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
#include "utils/DepUtils.h"
#include "utils/OsUtils.h"
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
    return DepUtils::buildDependencyPath(m_options.getDependenciesFile());
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
    std::vector<Dependency> dependencies = DepUtils::parse(depPath, m_options.getMode());
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
    DepUtils::readInfos(dependenciesFile, m_options);
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
    if (m_options.invertRepositoryOrder() && dependency.getType() == Dependency::Type::REMAKEN) {// what about cache management in this case ?
        fileRetriever = FileHandlerFactory::instance()->getAlternateHandler(dependency.getType(),m_options);
        if (!fileRetriever) { // no alternate repository found
            BOOST_LOG_TRIVIAL(error)<<"==> No alternate repository defined for '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"'";
            throw std::runtime_error("No alternate repository defined for '" + dependency.getPackageName() +":" +dependency.getVersion() + "'");
        }
        dependency.changeBaseRepository(m_options.getAlternateRepoUrl());
    }
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
            catch (std::runtime_error & e) { // try alternate/primary repository
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getAlternateHandler(dependency.getType(),m_options);
                if (m_options.invertRepositoryOrder() && dependency.getType() == Dependency::Type::REMAKEN) {// what about cache management in this case ?
                    fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
                }
                if (!fileRetriever) { // no alternate repository found
                    BOOST_LOG_TRIVIAL(error)<<"==> Unable to find '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"' on "<<dependency.getRepositoryType()<<"('"<<dependency.getBaseRepository()<<"')";
                    throw std::runtime_error(e.what());
                }
                else {
                    dependency.changeBaseRepository(m_options.getAlternateRepoUrl());
                    if (m_options.invertRepositoryOrder() && dependency.getType() == Dependency::Type::REMAKEN) {
                        dependency.resetBaseRepository();
                    }
                    source = fileRetriever->computeSourcePath(dependency);
                    try {
                        std::cout<<"==> Trying to find '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"' on alternate repository "<<dependency.getBaseRepository()<<"('"<<source<<"')"<<std::endl;
                        outputDirectory = fileRetriever->installArtefact(dependency);
                    }
                    catch (std::runtime_error & e) {
                        BOOST_LOG_TRIVIAL(error)<<"==> Unable to find '"<<dependency.getPackageName()<<":"<<dependency.getVersion()<<"' on "<<dependency.getBaseRepository()<<"('"<<source<<"')";
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

void DependencyManager::retrieveDependencies(const fs::path &  dependenciesFile)
{
    std::vector<fs::path> dependenciesFileList = DepUtils::getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS());
    std::map<std::string,bool> conditionsMap = DepUtils::parseConditionsFile(dependenciesFile.parent_path());
    std::vector<Dependency> conditionsDependencies;
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = DepUtils::filterConditionDependencies(conditionsMap, DepUtils::parse(depsFile, m_options.getMode()) );
            for (auto dep : dependencies) {
                if (!dep.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }
#ifdef BOOST_OS_WINDOWS_AVAILABLE
                if (dep.needsPriviledgeElevation() && !OsUtils::isElevated()) {
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



