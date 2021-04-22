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
#include "Tools/OsTools.h"
#include <boost/log/trivial.hpp>
#include "PathBuilder.h"
#include <regex>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

DependencyManager::DependencyManager(const CmdOptions & options):m_options(options),m_cache(options)
{
}

fs::path DependencyManager::buildDependencyPath()
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
    return dependenciesFile;
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
    std::vector<Dependency> dependencies = parse(depPath, m_options.getMode());
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

const std::string indentStr = "    ";

std::string indent(uint32_t indentLevel) {
    std::string ind;
    for (uint32_t i = 0; i<indentLevel; i++) {
        ind += indentStr;
    }
    return ind;
}

void displayInfo(const Dependency& d, uint32_t indentLevel)
{

    std::cout<<indent(indentLevel)<<"`-- "<<d.getName()<<":"<<d.getVersion()<<"  "<<d.getRepositoryType()<<" ("<<d.getIdentifier()<<")"<<std::endl;
}

void DependencyManager::readInfos(const fs::path &  dependenciesFile)
{
    m_indentLevel ++;
    std::vector<fs::path> dependenciesFileList = getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS());
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = parse(depsFile, m_options.getMode());
            for (auto dep : dependencies) {
                if (!dep.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                     m_indentLevel --;
                }
            }
            for (Dependency & dependency : dependencies) {
                displayInfo(dependency, m_indentLevel);
                fs::detail::utf8_codecvt_facet utf8;
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
                fs::path outputDirectory = fileRetriever->computeLocalDependencyRootDir(dependency);
                readInfos(outputDirectory/"packagedependencies.txt");
            }
        }
    }
    m_indentLevel --;
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


std::vector<Dependency> removeRedundantDependencies(const std::multimap<std::string,Dependency> & dependencies)
{
    std::multimap<std::string,std::pair<std::size_t,Dependency>> dependencyMap;
    std::vector<Dependency> depVector;
    for (auto const & node : dependencies) {
        if (!node.second.isSystemDependency()) {
            depVector.push_back(std::move(node.second));
        }
        else {// System dependency
            if (dependencies.count(node.first) == 1) {
                // only one system dependency was found : generic or dedicated tool dependency is added to the dependencies vector
                depVector.push_back(std::move(node.second));
            }
            else {
                if (node.second.isSpecificSystemToolDependency()) {
                    // there must be one dedicated tool when the dependency appears twice : add only the dedicated dependency
                    depVector.push_back(std::move(node.second));
                }
            }
        }
    }
    return depVector;
}

std::vector<Dependency> DependencyManager::parse(const fs::path &  dependenciesPath, const std::string & linkMode)
{
    std::multimap<std::string,Dependency> libraries;
    if (fs::exists(dependenciesPath)) {
        ifstream fis(dependenciesPath.generic_string(),ios::in);
        while (!fis.eof()) {
            string curStr;
            getline(fis,curStr);
            if (!curStr.empty()) {
                std::string commentRegexStr = "^[ \t]*//";
                std::regex commentRegex(commentRegexStr, std::regex_constants::extended);
                std::smatch sm;
                // parsing finds commented lines
                if (!std::regex_search(curStr, sm, commentRegex, std::regex_constants::match_any)) {
                    // Dependency line is not commented: parsing the dependency
                    Dependency dep(curStr, linkMode);

                    if (dep.isGenericSystemDependency()
                        ||dep.isSpecificSystemToolDependency()
                        ||!dep.isSystemDependency()) {
                        // only add "generic" system or tool@system or other deps
                        libraries.insert(std::make_pair(dep.getName(), std::move(dep)));
                    }
                }
                else {
                    std::cout<<"[IGNORED]: Dependency line '"<<curStr<<"' is commented !"<<std::endl;
                }
            }
        }
        fis.close();
    }
    return removeRedundantDependencies(libraries);
}

void DependencyManager::parseRecurse(const fs::path &  dependenciesPath, const CmdOptions & options, std::vector<Dependency> & deps)
{
    std::vector<fs::path> dependenciesFileList = getChildrenDependencies(dependenciesPath.parent_path(), options.getOS());
    for (fs::path & depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = parse(depsFile, options.getMode());
            deps.insert(std::end(deps), std::begin(dependencies), std::end(dependencies));
            for (auto dep : dependencies) {
                if (!dep.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }
                fs::detail::utf8_codecvt_facet utf8;
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dep, options);
                fs::path outputDirectory = fileRetriever->computeLocalDependencyRootDir(dep);
                parseRecurse(outputDirectory/"packagedependencies.txt", options,deps);
            }
        }
    }
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
    std::vector<fs::path> dependenciesFileList = getChildrenDependencies(dependenciesFile.parent_path(), m_options.getOS());
    parseConditionsFile(dependenciesFile.parent_path());
    std::vector<Dependency> conditionsDependencies;
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = filterConditionDependencies( parse(depsFile, m_options.getMode()) );
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

std::vector<fs::path> DependencyManager::getChildrenDependencies(const fs::path &  outputDirectory, const std::string & osPlatform, const std::string & filePrefix)
{
    auto platformFiles = list<std::string>{filePrefix + ".txt"};
    platformFiles.push_back(filePrefix + "-"+ osPlatform + ".txt");
    std::vector<fs::path> filePaths;
    for (std::string file : platformFiles) {
        fs::path chidrenPackageDependenciesPath = outputDirectory / file;
        if (fs::exists(chidrenPackageDependenciesPath)) {
            filePaths.push_back(fs::absolute(chidrenPackageDependenciesPath));
        }
    }
    return filePaths;
}



