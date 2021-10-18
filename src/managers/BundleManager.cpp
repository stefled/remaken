#include "BundleManager.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <list>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/dll.hpp>
#include <boost/algorithm/string.hpp>
//#include <zipper/unzipper.h>
#include <future>
#include "tools/SystemTools.h"
#include "utils/DepUtils.h"
#include "utils/OsUtils.h"
#include <boost/log/trivial.hpp>
#include <regex>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

BundleManager::BundleManager(const CmdOptions & options):m_xpcfManager(options),m_options(options)
{
}

fs::path BundleManager::buildDependencyPath()
{
    fs::path dependenciesFile = DepUtils::buildDependencyPath(m_options.getDependenciesFile());
    parseIgnoreInstall(dependenciesFile);
    return dependenciesFile;
}

void BundleManager::parseIgnoreInstall(const fs::path &  filepath)
{
    std::vector<fs::path> ignoreFileList = DepUtils::getChildrenDependencies(filepath.parent_path(), m_options.getOS(), "packageignoreinstall");
    for (fs::path & ignoreFile : ignoreFileList) {
        if (fs::exists(ignoreFile)) {
            ifstream fis(ignoreFile.generic_string(),ios::in);
            while (!fis.eof()) {
                string curStr;
                getline(fis,curStr);
                if (!curStr.empty()) {
                    std::string commentRegexStr = "^[ \t]*//";
                    std::regex commentRegex(commentRegexStr, std::regex_constants::extended);
                    std::smatch sm;
                    // parsing finds commented lines
                    if (!std::regex_search(curStr, sm, commentRegex, std::regex_constants::match_any)) {
                        // line is not commented: parsing the package name
                        boost::trim(curStr);
                        m_ignoredPackages[curStr] = true;
                    }
                    else {
                        std::cout<<"[IGNORED]: line '"<<curStr<<"' is commented !"<<std::endl;
                    }
                }
            }
            fis.close();
        }
    }
}

int BundleManager::bundle()
{
    try {
        if (!fs::exists(m_options.getDestinationRoot())) {
            fs::create_directory(m_options.getDestinationRoot());
        }
        bundleDependencies(buildDependencyPath());
    }
    catch (const std::runtime_error & e) {
        BOOST_LOG_TRIVIAL(error)<<e.what();
        return -1;
    }

#ifndef BOOST_OS_WINDOWS_AVAILABLE
    BOOST_LOG_TRIVIAL(warning)<<"bundle command under implementation : rpath not reinterpreted after copy";
#endif
    return 0;
}

int BundleManager::bundleXpcf()
{
    try {
        if (!fs::exists(m_options.getDestinationRoot()/m_options.getModulesSubfolder())) {
            fs::create_directories(m_options.getDestinationRoot()/m_options.getModulesSubfolder());
        }
        fs::path xpcfConfigFilePath = buildDependencyPath();
        if ( xpcfConfigFilePath.extension() != ".xml") {
            BOOST_LOG_TRIVIAL(error)<<" the xpcf configuration file must be an xml file with the correct .xml extension, file provided is "<<xpcfConfigFilePath;
            return -1;
        }
        fs::copy_file(xpcfConfigFilePath , m_options.getDestinationRoot()/xpcfConfigFilePath.filename(), fs::copy_option::overwrite_if_exists);

        const std::map<std::string, fs::path> & modulesPathMap = m_xpcfManager.parseXpcfModulesConfiguration(xpcfConfigFilePath);
        m_xpcfManager.updateXpcfModulesPath(m_options.getDestinationRoot()/xpcfConfigFilePath.filename());
        for (auto & [name,modulePath] : modulesPathMap) {
            OsUtils::copySharedLibraries(modulePath,m_options);
        }

        for (auto & [name,modulePath] : modulesPathMap) {
            fs::path packageRootPath = XpcfXmlManager::findPackageRoot(modulePath);
            if (!fs::exists(packageRootPath)) {
                BOOST_LOG_TRIVIAL(warning)<<"Unable to find root package path "<<packageRootPath<<" for module "<<name;
            }
            if (fs::exists(packageRootPath/"packagedependencies.txt")) {
                bundleDependencies(packageRootPath/"packagedependencies.txt");
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
#ifndef BOOST_OS_WINDOWS_AVAILABLE
    BOOST_LOG_TRIVIAL(warning)<<"bundleXpcf command under implementation : rpath not reinterpreted after copy";
#endif
    return 0;
}


void BundleManager::bundleDependency(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
    fs::path outputDirectory = fileRetriever->bundleArtefact(dependency);
    if (!outputDirectory.empty() && dependency.getType() == Dependency::Type::REMAKEN && m_options.recurse()) {
        this->bundleDependencies(outputDirectory/"packagedependencies.txt");
    }
}

void BundleManager::bundleDependencies(const fs::path &  dependenciesFile)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::vector<fs::path> dependenciesFileList = DepUtils::getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS(),dependenciesFile.stem().generic_string(utf8));
    for (fs::path const & depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = DepUtils::parse(depsFile, m_options.getMode());
            for (Dependency const & dependency : dependencies) {
                if (!dependency.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }
                if (dependency.getType() == Dependency::Type::REMAKEN
                        || dependency.getType() == Dependency::Type::CONAN
                        || dependency.getType() == Dependency::Type::BREW
                        || dependency.getType() == Dependency::Type::VCPKG
                        || dependency.getType() == Dependency::Type::SYSTEM) {
                    if (!mapContains(m_ignoredPackages, dependency.getPackageName())) {
                        bundleDependency(dependency);
                    }
                }
            }
        }
    }
}




