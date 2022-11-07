#include "RunCommand.h"
#include "utils/DepUtils.h"
#include "managers/XpcfXmlManager.h"
#include "utils/OsUtils.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>
#include "FileHandlerFactory.h"
#include <memory>


namespace bp = boost::process;

using namespace std;

RunCommand::RunCommand(const CmdOptions & options):AbstractCommand(RunCommand::NAME),m_options(options)
{
    if (!options.getXpcfXmlFile().empty()) {
        m_xpcfXmlFile = options.getXpcfXmlFile();
    }
    if (!options.getDependenciesFile().empty()) {
        m_depsFile = options.getDependenciesFile();
    }
    if (!options.getApplicationName().empty()) {
        m_applicationName = options.getApplicationName();
    }
    else if (!options.getApplicationFile().empty()) {
        m_applicationFile = options.getApplicationFile();
    }
}

void RunCommand::findBinary(const std::string & pkgName, const std::string & pkgVersion)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string appName = pkgName;
    if (!m_applicationName.empty()) {
        appName = m_applicationName.generic_string(utf8);
    }
    fs::path pkgPath = DepUtils::findPackageFolder(m_options, pkgName, pkgVersion);
    if (!pkgPath.empty()) {
        if (!fs::exists(pkgPath/"bin")) {
            // there is no application
            return;
        }
    }
    for (fs::directory_entry& pathElt : fs::recursive_directory_iterator(pkgPath/"bin"/m_options.getArchitecture())) {
        if (fs::is_regular_file(pathElt.path())) {
            if (pathElt.path().extension() == ".xml") {
                if (m_xpcfXmlFile.empty()) {
                    m_xpcfXmlFile = pathElt.path();
                }
            }
            if (pathElt.path().extension().empty() ||
                    ((m_options.getOS() == "win") && (pathElt.path().extension().generic_string(utf8) == ".exe"))) {
                if (pathElt.path().filename().stem().generic_string(utf8) == appName) {
                    if (pathElt.path().parent_path().generic_string(utf8).find(m_options.getConfig()) != std::string::npos) {
                        m_applicationFile = pathElt.path();
                    }
                }
            }
        }
    }
    for (fs::directory_entry& pathElt : fs::directory_iterator(pkgPath)) {
        if (fs::is_regular_file(pathElt.path())) {
            if (pathElt.path().extension() == ".xml") {
                if (m_xpcfXmlFile.empty()) {
                    m_xpcfXmlFile = pathElt.path();
                }
            }
            if (pathElt.path().filename() == "packagedependencies.txt") {
                if (m_depsFile.empty() || m_depsFile == "packagedependencies.txt") {
                    m_depsFile = pathElt.path();
                }
            }
        }
    }

}

int RunCommand::execute()
{

    // supports debug/release config : will be reflected in constructed paths
    // parse pkgdeps recursively if any
    // parse xml
    // gather deps by types
    // conan -> generate json
    // brew, vcpkg, choco, conan -> get libPaths
    // remaken -> get path for each remaken deps (from xml or from pkgdeps parsing)
    fs::detail::utf8_codecvt_facet utf8;
    std::vector<std::string> results;
    boost::split(results, m_options.getRemakenPkgRef(), [](char c){return c == ':';});
    if (results.size() == 2) {
        std::string pkgName, pkgVersion;
        pkgName = results[0];
        pkgVersion = results[1];
        findBinary(pkgName, pkgVersion);
    }

    std::vector<Dependency> deps;
    std::vector<fs::path> libPaths;
    if (!m_xpcfXmlFile.empty()) {
        XpcfXmlManager xpcfManager(m_options);
        try {
            fs::path xpcfConfigFilePath = DepUtils::buildDependencyPath(m_xpcfXmlFile.generic_string(utf8));
            if ( xpcfConfigFilePath.extension() != ".xml") {
                return -1;
            }
            const std::map<std::string, fs::path> & modulesPathMap = xpcfManager.parseXpcfModulesConfiguration(xpcfConfigFilePath);

            for (auto & [name,modulePath] : modulesPathMap) {
                fs::path packageRootPath = XpcfXmlManager::findPackageRoot(modulePath, m_options.getVerbose());
                if (!fs::exists(packageRootPath)) {
                    BOOST_LOG_TRIVIAL(warning)<<"Unable to find root package path "<<packageRootPath<<" for module "<<name<<" path="<<modulePath;
                }
                // The following line should not be needed mandatory as xpcf loads dynamically the modules
                // libPaths.push_back(modulePath);
                if (fs::exists(packageRootPath/"packagedependencies.txt")) {
                    DepUtils::parseRecurse(packageRootPath/"packagedependencies.txt",m_options,deps);
                }
                else {
                    BOOST_LOG_TRIVIAL(warning)<<"Unable to find packagedependencies.txt file in package path"<<packageRootPath<<" for module "<<name;
                }
            }
        }
        catch (const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(error)<<e.what();
            return -1;
        }
    }

    if (!m_depsFile.empty()) {
        DepUtils::parseRecurse(m_depsFile, m_options, deps);
    }
    // std::cout<<"Exhaustive deps list:"<<std::endl;
    std::map<std::string,Dependency> depsMap;
    for (auto dependency : deps) {//filter redundant deps
        //std::cout<<dependency.toString()<<std::endl;
        depsMap.insert_or_assign(dependency.getName()+dependency.getVersion(), dependency);
    }
    for (auto & [name,dependency]: depsMap) {
        shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
        std::vector<fs::path> paths = fileRetriever->libPaths(dependency);
        libPaths.insert(libPaths.end(),paths.begin(), paths.end());
    }

    std::string SharedLibraryPathEnvName(OsUtils::sharedLibraryPathEnvName(m_options.getOS()));

    if (m_options.environmentOnly()) {
        // display results
        std::cout<<SharedLibraryPathEnvName<<"=";
        for (auto path: libPaths) {
            fs::detail::utf8_codecvt_facet utf8;
            std::cout<<":"<<path.generic_string(utf8);
        }
        std::cout<<":$"<<SharedLibraryPathEnvName<<std::endl;
        return 0;
    }

    int result = 0;
    if (!m_applicationFile.empty()) {
        if (!fs::exists(m_applicationFile)) {
            BOOST_LOG_TRIVIAL(error)<<"Unable to find application file "<<m_options.getApplicationFile();
            return -1;
        }
        auto env = boost::this_process::environment();
        bp::environment runEnv = env;
        for (auto & path: libPaths) {
            fs::detail::utf8_codecvt_facet utf8;
            runEnv[SharedLibraryPathEnvName] += path.generic_string(utf8);
        }
        std::vector<std::string> parsedArgs;
        for (auto & arg : m_options.getApplicationArguments()) {
            std::string argument = arg;
            if (arg.length() >= 2) {
                if ((arg[0] == '\\') && (arg[1]   == '-')) {
                    argument = arg.substr(1);
                }
            }
            parsedArgs.push_back(argument);
        }

        result = bp::system(m_applicationFile, bp::args(parsedArgs), runEnv);
    }

    return result;
}
