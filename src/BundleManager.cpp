#include "BundleManager.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <list>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/dll.hpp>
#include <boost/algorithm/string.hpp>
//#include <zipper/unzipper.h>
#include <future>
#include "SystemTools.h"
#include "OsTools.h"
#include <boost/log/trivial.hpp>
#include "PathBuilder.h"
#include "DependencyManager.h"
#include <regex>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

BundleManager::BundleManager(const CmdOptions & options):m_options(options)
{
}

fs::path BundleManager::buildDependencyPath()
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path currentPath(boost::filesystem::initial_path().generic_string(utf8));

    fs::path dependenciesFile (m_options.getDependenciesFile(),utf8);

    if (!dependenciesFile.is_absolute()){
        dependenciesFile = currentPath /dependenciesFile;
    }

    if (!fs::exists(dependenciesFile)) {
        throw std::runtime_error("The file does not exists " + dependenciesFile.generic_string(utf8));
    }
    parseIgnoreInstall(dependenciesFile);
    return dependenciesFile;
}

void BundleManager::parseIgnoreInstall(const fs::path &  filepath)
{
    std::vector<fs::path> ignoreFileList = DependencyManager::getChildrenDependencies(filepath.parent_path(), m_options.getOS(), "packageignoreinstall");
    for (fs::path ignoreFile : ignoreFileList) {
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

fs::path findPackageRoot(fs::path moduleLibPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string versionRegex = "[0-9]+\.[0-9]+\.[0-9]+";
    fs::path currentFilename = moduleLibPath.filename();
    bool bFoundVersion = false;
    std::smatch sm;
    while (!bFoundVersion) {
        std::regex tmplRegex(versionRegex, std::regex_constants::extended);
        std::string currentFilenameStr = currentFilename.string(utf8);
        if (std::regex_search(currentFilenameStr, sm, tmplRegex, std::regex_constants::match_any)) {
            std::string matchStr = sm.str(0);
            BOOST_LOG_TRIVIAL(warning)<<"Found "<< matchStr<<" version for modulepath "<<moduleLibPath;
            std::cout<<"Found "<< matchStr<<" version "<<std::endl;
            return moduleLibPath;
        }
        else {
            moduleLibPath = moduleLibPath.parent_path();
            currentFilename = moduleLibPath.filename();
        }
    }
    // no path found : return empty path
    return fs::path();
}

int BundleManager::bundleXpcf()
{
    try {
        if (!fs::exists(m_options.getDestinationRoot()/m_options.getModulesSubfolder())) {
            fs::create_directories(m_options.getDestinationRoot()/m_options.getModulesSubfolder());
        }
        fs::path xpcfConfigFilePath = buildDependencyPath();
        if ( xpcfConfigFilePath.extension() != ".xml") {
            return -1;
        }
        fs::copy_file(xpcfConfigFilePath , m_options.getDestinationRoot()/xpcfConfigFilePath.filename(), fs::copy_option::overwrite_if_exists);

        parseXpcfModulesConfiguration(xpcfConfigFilePath);
        updateXpcfModulesPath(m_options.getDestinationRoot()/xpcfConfigFilePath.filename());
        for (auto & [name,modulePath] : m_modulesPathMap) {
            OsTools::copySharedLibraries(modulePath,m_options);
        }

        for (auto & [name,modulePath] : m_modulesPathMap) {
            OsTools::copySharedLibraries(modulePath,m_options);
            fs::path packageRootPath = findPackageRoot(modulePath);
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

void BundleManager::updateModuleNode(tinyxml2::XMLElement * xmlModuleElt)
{
    fs::detail::utf8_codecvt_facet utf8;
    xmlModuleElt->SetAttribute("path",m_options.getModulesSubfolder().string(utf8).c_str());
}

int BundleManager::updateXpcfModulesPath(const fs::path & configurationFilePath)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    tinyxml2::XMLDocument xmlDoc;
    enum tinyxml2::XMLError loadOkay = xmlDoc.LoadFile(configurationFilePath.string(utf8).c_str());
    if (loadOkay == 0) {
        try {
            //TODO : check each element exists before using it !
            // a check should be performed upon announced module uuid and inner module uuid
            // check xml node is xpcf-registry first !
            tinyxml2::XMLElement * rootElt = xmlDoc.RootElement();
            string rootName = rootElt->Value();
            if (rootName != "xpcf-registry" && rootName != "xpcf-configuration") {
                return -1;
            }
            result = 0;

            processXmlNode(rootElt, "module", std::bind(&BundleManager::updateModuleNode, this, _1));
            xmlDoc.SaveFile(configurationFilePath.string(utf8).c_str());
        }
        catch (const std::runtime_error & e) {
            return -1;
        }
    }
    return result;
}

void BundleManager::declareModule(tinyxml2::XMLElement * xmlModuleElt)
{
    std::string moduleName = xmlModuleElt->Attribute("name");
    std::string moduleDescription = "";
    if (xmlModuleElt->Attribute("description") != nullptr) {
        moduleDescription = xmlModuleElt->Attribute("description");
    }
    std::string moduleUuid =  xmlModuleElt->Attribute("uuid");
    fs::path modulePath = PathBuilder::buildModuleFolderPath(xmlModuleElt->Attribute("path"), m_options.getConfig());
    if (! mapContains(m_modulesUUiDMap, moduleName)) {
        m_modulesUUiDMap[moduleName] = moduleUuid;
    }
    else {
        std::string previousModuleUUID = m_modulesUUiDMap.at(moduleName);
        if (moduleUuid != previousModuleUUID) {
            BOOST_LOG_TRIVIAL(warning)<<"Already found a module named "<<moduleName<<" with a different UUID: first UUID found ="<<previousModuleUUID<<" last UUID = "<<moduleUuid;
        }
    }

    if (! mapContains(m_modulesPathMap, moduleName)) {
        m_modulesPathMap[moduleName] = modulePath;
    }
}


int BundleManager::parseXpcfModulesConfiguration(const fs::path & configurationFilePath)
{
    int result = -1;
    tinyxml2::XMLDocument doc;
    m_modulesPathMap.clear();
    enum tinyxml2::XMLError loadOkay = doc.LoadFile(configurationFilePath.string().c_str());
    if (loadOkay == 0) {
        try {
            //TODO : check each element exists before using it !
            // a check should be performed upon announced module uuid and inner module uuid
            // check xml node is xpcf-registry first !
            tinyxml2::XMLElement * rootElt = doc.RootElement();
            string rootName = rootElt->Value();
            if (rootName != "xpcf-registry" && rootName != "xpcf-configuration") {
                return -1;
            }
            result = 0;

            processXmlNode(rootElt, "module", std::bind(&BundleManager::declareModule, this, _1));
        }
        catch (const std::runtime_error & e) {
            return -1;
        }
    }
    return result;
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
    std::vector<fs::path> dependenciesFileList = DependencyManager::getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS());
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = DependencyManager::parse(depsFile, m_options.getMode());
            for (auto dependency : dependencies) {
                if (!dependency.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }
            }
            std::vector<std::shared_ptr<std::thread>> thread_group;
            for (Dependency const & dependency : dependencies) {
                if (dependency.getType() == Dependency::Type::REMAKEN || dependency.getType() == Dependency::Type::CONAN) {
                    if (!mapContains(m_ignoredPackages, dependency.getPackageName())) {
                        bundleDependency(dependency);
                    }
                }
            }
        }
    }
}




