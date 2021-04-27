#include "RunCommand.h"
#include "DependencyManager.h"
#include "XpcfXmlManager.h"
#include "tools/OsTools.h"
#include <boost/log/trivial.hpp>
#include "FileHandlerFactory.h"
#include <memory>

using namespace std;

RunCommand::RunCommand(const CmdOptions & options):AbstractCommand(RunCommand::NAME),m_options(options)
{
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
    std::vector<Dependency> deps;
    if (!m_options.getXpcfXmlFile().empty()) {
        XpcfXmlManager xpcfManager(m_options);
        try {
            fs::path xpcfConfigFilePath = DependencyManager::buildDependencyPath(m_options.getXpcfXmlFile());
            if ( xpcfConfigFilePath.extension() != ".xml") {
                return -1;
            }
            const std::map<std::string, fs::path> & modulesPathMap = xpcfManager.parseXpcfModulesConfiguration(xpcfConfigFilePath);

            for (auto & [name,modulePath] : modulesPathMap) {
                fs::path packageRootPath = XpcfXmlManager::findPackageRoot(modulePath);
                if (!fs::exists(packageRootPath)) {
                    BOOST_LOG_TRIVIAL(warning)<<"Unable to find root package path "<<packageRootPath<<" for module "<<name<<" path="<<modulePath;
                }
                if (fs::exists(packageRootPath/"packagedependencies.txt")) {
                    DependencyManager::parseRecurse(packageRootPath/"packagedependencies.txt",m_options,deps);
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

    if (!m_options.getDependenciesFile().empty()) {
        DependencyManager::parseRecurse(m_options.getDependenciesFile(), m_options, deps);
    }
   // std::cout<<"Exhaustive deps list:"<<std::endl;
    std::map<std::string,Dependency> depsMap;
    for (auto dependency : deps) {//filter redundant deps
        //std::cout<<dependency.toString()<<std::endl;
        depsMap.insert_or_assign(dependency.getName()+dependency.getVersion(), dependency);
    }

    if (m_options.environmentOnly()) {
        std::vector<fs::path> libPaths;
        for (auto & [name,dependency]: depsMap) {
             shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
             std::vector<fs::path> paths = fileRetriever->libPaths(dependency);
             libPaths.insert(libPaths.end(),paths.begin(), paths.end());
        }
    // display results
        std::cout<<OsTools::sharedLibraryPathEnvName(m_options.getOS())<<"=";
        for (auto path: libPaths) {
            fs::detail::utf8_codecvt_facet utf8;
            std::cout<<":"<<path.generic_string(utf8);
        }
        std::cout<<":$"<<OsTools::sharedLibraryPathEnvName(m_options.getOS())<<std::endl;
        return 0;
    }
    if (!m_options.getApplicationFile().empty()) {

    }

    return 0;
}
