#include "DependencyManager.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <list>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/dll.hpp>
#include <boost/algorithm/string.hpp>
//#include <zipper/unzipper.h>
#include <future>
#include "tools/SystemTools.h"
#include "utils/DepTools.h"
#include "utils/OsTools.h"
#include <boost/log/trivial.hpp>
#include "utils/PathBuilder.h"
#include <regex>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

DependencyManager::DependencyManager(const CmdOptions & options):m_options(options),m_cache(options)
{
}

fs::path DependencyManager::buildDependencyPath()
{
    return DepTools::buildDependencyPath(m_options.getDependenciesFile());
}

int DependencyManager::retrieve()
{
    try {
        fs::path rootPath = buildDependencyPath();
        if (m_options.projectModeEnabled()) {
            m_options.setProjectRootPath(rootPath.parent_path());
        }
        retrieveDependencies(rootPath);
        std::cout<<std::endl;
        std::cout<<"--------- Installation status ---------"<<std::endl;
        for (auto & [depType,retriever] : FileHandlerFactory::instance()->getHandlers()) {
            std::cout<<"=> '"<<depType<<"' dependencies installed:"<<std::endl;
            for (auto & dependency : retriever->installedDependencies()) {
                std::cout<<"===> "<<dependency.toString()<<std::endl;
            }
            std::cout<<std::endl;
        }
    }
    catch (const std::runtime_error & e) {
        BOOST_LOG_TRIVIAL(error)<<e.what();
        return -1;
    }
    return 0;
}

int DependencyManager::parse()
{
    bool bValid = true;
    bool bNeedElevation = false;
    fs::path depPath = buildDependencyPath();
    std::vector<Dependency> dependencies = DepTools::parse(depPath, m_options.getMode());
    for (auto dep : dependencies) {
        if (!dep.validate()) {
            bValid = false;
        }
#ifdef BOOST_OS_WINDOWS_AVAILABLE
        if (dep.needsPriviledgeElevation()) {
            bNeedElevation = true;
        }
#endif
    }
    if (!bValid) {
        BOOST_LOG_TRIVIAL(error)<<"Semantic error parsing file "<<depPath<<" please fix the above dependency(ies) declaration error(s)";
        return -1;
    }
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    if (bNeedElevation) {
        BOOST_LOG_TRIVIAL(warning)<<"Information of parsing file : remaken needs elevated privileges to install system Windows (choco) dependencies ";
        return -2;
    }
#endif
    BOOST_LOG_TRIVIAL(info)<<"File "<<depPath<<" successfully parsed";
    return 0;
}

bool yesno_prompt(char const* prompt) {
    using namespace std;
    while (true) {
        cout << prompt << " [y/n] ";
        string line;
        if (!getline(cin, line)) {
            throw std::runtime_error("unexpected input error");
        }
        else if (line.size() == 1 && line.find_first_of("YyNn") != line.npos) {
            return line == "Y" || line == "y";
        }
    }
}

int DependencyManager::clean()
{
    try {
        std::cout<<"WARNING: all remaken installed packages will be removed (note: system, conan ... dependencies are kept)"<<std::endl;
        if (yesno_prompt("are you sure ?")) {
            fs::remove_all(m_options.getRemakenRoot());
        }
    }
    catch (const std::runtime_error & e) {
        BOOST_LOG_TRIVIAL(error)<<e.what();
        return -1;
    }
    return 0;
}

void DependencyManager::readInfos(const fs::path &  dependenciesFile)
{
    DepTools::readInfos(dependenciesFile, m_options);
}


int DependencyManager::info()
{
    try {
        readInfos(buildDependencyPath());
    }
    catch (const std::runtime_error & e) {
        BOOST_LOG_TRIVIAL(error)<<e.what();
        return -1;
    }
    return 0;
}

bool DependencyManager::installDep(Dependency &  dependency, const std::string & source,
                                   const fs::path & outputDirectory, const fs::path & libDirectory, const fs::path & binDirectory)
{
    // backward compatibility values
    bool withBinDir = false;
    bool withLibDir = true;
    bool withHeaders = true;

    if (fs::exists(outputDirectory/Constants::PKGINFO_FOLDER)) {
        // use existing package informations
        withBinDir = fs::exists(outputDirectory/Constants::PKGINFO_FOLDER/".bin");
        withLibDir = fs::exists(outputDirectory/Constants::PKGINFO_FOLDER/".lib");
        withHeaders = fs::exists(outputDirectory/Constants::PKGINFO_FOLDER/".headers");
    }

    if (dependency.getType() != Dependency::Type::REMAKEN) {
        if (m_options.useCache()) {
            if (!m_cache.contains(source)) {
                return true;
            }
            return false;
        }
        return true;
    }

    if (m_options.useCache()) {
        if (dependency.getMode() == "na" || (withHeaders && !withBinDir && !withLibDir)) {
            if (!fs::exists(outputDirectory)) {
                return true;
            }
            return false;
        }
        if (withLibDir && !fs::exists(libDirectory) || withBinDir && !fs::exists(binDirectory)) {
            return true;
        }
        else {
            return false;
        }
        if (!m_cache.contains(source)) {
            return true;
        }
        return false;
    }

    if (dependency.getMode() != "na") {
        if (withLibDir && !fs::exists(libDirectory) || withBinDir && !fs::exists(binDirectory)) {
            return true;
        }
        return false;
    }

    if (fs::exists(outputDirectory)) {
        return false;
    }
    return true;
}

void DependencyManager::retrieveDependency(Dependency &  dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
    std::string source = fileRetriever->computeSourcePath(dependency);
    fs::path outputDirectory = fileRetriever->computeLocalDependencyRootDir(dependency);
    fs::path libDirectory = fileRetriever->computeRootLibDir(dependency);
    fs::path binDirectory = fileRetriever->computeRootBinDir(dependency);
    if (installDep(dependency, source, outputDirectory, libDirectory, binDirectory) || m_options.force()) {
        try {
            std::cout<<"=> Installing "<<dependency.getRepositoryType()<<"::"<<source<<std::endl;
            try {
                outputDirectory = fileRetriever->installArtefact(dependency);
            }
            catch (std::runtime_error & e) { // try alternate repository
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getAlternateHandler(dependency,m_options);
                if (!fileRetriever) { // no alternate repository found
                    BOOST_LOG_TRIVIAL(error)<<"Unable to find '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"' on "<<dependency.getRepositoryType()<<"('"<<dependency.getBaseRepository()<<"')";
                    throw std::runtime_error(e.what());
                }
                else {
                    dependency.changeBaseRepository(m_options.getAlternateRepoUrl());
                    source = fileRetriever->computeSourcePath(dependency);
                    try {
                        outputDirectory = fileRetriever->installArtefact(dependency);
                    }
                    catch (std::runtime_error & e) {
                        BOOST_LOG_TRIVIAL(error)<<"Unable to find '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"' on "<<dependency.getRepositoryType()<<"('"<<dependency.getBaseRepository()<<"')";
                        throw std::runtime_error(e.what());
                    }
                }
            }
            std::cout<<"===> "<<dependency.getName()<<" installed in "<<outputDirectory<<std::endl;
            if (m_options.useCache()) {
                m_cache.add(source);
            }
        }
        catch (const std::runtime_error & e) {
            throw std::runtime_error(e.what());
        }
    }
    else {
        if (m_cache.contains(source) && m_options.useCache()) {
            std::cout<<"===> "<<dependency.getRepositoryType()<<"::"<<dependency.getName()<<"-"<<dependency.getVersion()<<" found in cache : already installed"<<std::endl;
        }
        else {
            std::cout<<"===> "<<dependency.getRepositoryType()<<"::"<<dependency.getName()<<"-"<<dependency.getVersion()<<" already installed in folder : "<<outputDirectory<<std::endl;
        }
    }
    this->retrieveDependencies(outputDirectory/"packagedependencies.txt");
}

void DependencyManager::generateConfigureFile(const fs::path &  rootFolderPath, const std::vector<Dependency> & deps)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path buildFolderPath = rootFolderPath/"build";
    fs::path configureFilePath = buildFolderPath/"configure_conditions.pri";
    if (deps.empty()) {
        return;
    }
    if (fs::exists(configureFilePath) ) {
        fs::remove(configureFilePath);
    }
    if (!fs::exists(buildFolderPath)) {
        fs::create_directories(buildFolderPath);
    }

    std::ofstream configureFile(configureFilePath.generic_string(utf8).c_str(), std::ios::out);
    for (auto & dep : deps) {
        for (auto & condition : dep.getConditions()) {
            configureFile << "DEFINES += " << condition;
            configureFile << "\n";
        }
    }
    configureFile.close();
}

void DependencyManager::parseConditionsFile(const fs::path &  rootFolderPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path configureFilePath = rootFolderPath/"configure_conditions.pri";
    if (!fs::exists(configureFilePath)) {
        return ;
    }

    std::ifstream configureFile(configureFilePath.generic_string(utf8).c_str(), std::ios::in);
    while (!configureFile.eof()) {
        std::vector<std::string> results;
        string curStr;
        getline(configureFile,curStr);
        std::string formatRegexStr = "^[\t\s]*DEFINES[\t\s]*+=[\t\s]*[a-zA-Z0-9_-]*";
        //std::regex formatRegexr(formatRegexStr, std::regex_constants::extended);
        std::smatch sm;
        //check string format is ^[\t\s]*DEFINES[\t\s]*+=[\t\s]*[a-zA-Z0-9_-]*
       // if (std::regex_search(curStr, sm, formatRegexr, std::regex_constants::match_any)) {
            boost::split(results, curStr, [](char c){return c == '=';});
            if (results.size() == 2) {
                std::string conditionValue = results[1];
                boost::trim(conditionValue);
                m_defaultConditionsMap.insert({conditionValue,true});
            }
       // }
    }
    configureFile.close();
}

std::vector<Dependency> DependencyManager::filterConditionDependencies(const std::vector<Dependency> & depCollection)
{
    std::vector<Dependency> filteredDepCollection;
    for (auto const & dep : depCollection) {
        bool conditionsFullfilled = true;
        if (dep.hasConditions()) {
            for (auto & condition : dep.getConditions()) {
                if (!mapContains(m_defaultConditionsMap, condition)) {
                    std::string msg = "configure project with [" + condition + "] (=> " + dep.getPackageName() +":"+dep.getVersion() +") ?";
                    if (!yesno_prompt(msg.c_str()))  {
                        conditionsFullfilled = false;
                        std::cout<<"Dependency not configured "<<dep.toString()<<std::endl;
                    }
                }
                else {
                    std::cout<<"Configuring project with [" + condition + "] (=> " + dep.getPackageName() +":"+dep.getVersion() +")"<<std::endl;
                }
            }
        }
        if (conditionsFullfilled) {
            filteredDepCollection.push_back(dep);
        }
    }
    return filteredDepCollection;
}

void DependencyManager::retrieveDependencies(const fs::path &  dependenciesFile)
{
    std::vector<fs::path> dependenciesFileList = DepTools::getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS());
    parseConditionsFile(dependenciesFile.parent_path());
    std::vector<Dependency> conditionsDependencies;
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = filterConditionDependencies( DepTools::parse(depsFile, m_options.getMode()) );
            for (auto dep : dependencies) {
                if (!dep.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }
#ifdef BOOST_OS_WINDOWS_AVAILABLE
                if (dep.needsPriviledgeElevation() && !OsTools::isElevated()) {
                    throw std::runtime_error("Remaken needs elevated privileges to install system Windows " + SystemTools::getToolIdentifier() + " dependencies");
                }
#endif
            }
            std::vector<std::shared_ptr<std::thread>> thread_group;
            for (Dependency & dependency : dependencies) {
#ifdef REMAKEN_USE_THREADS
                thread_group.push_back(std::make_shared<std::thread>(&DependencyManager::retrieveDependency,this, dependency));
#else
                retrieveDependency(dependency);
                if (dependency.hasConditions()) {
                    conditionsDependencies.push_back(dependency);
                }
#endif
            }
            for (auto threadPtr : thread_group) {
                threadPtr->join();
            }
        }
    }
    generateConfigureFile(dependenciesFile.parent_path(), conditionsDependencies);
}



