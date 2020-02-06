#include "DependencyManager.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <list>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
//#include <zipper/unzipper.h>
#include <future>
#include "SystemTools.h"
#include "OsTools.h"
#include <boost/log/trivial.hpp>

using namespace std;

bool isSystemDependency(const Dependency & dep)
{
    return (dep.getRepositoryType() == "system");
}

bool isSpecificSystemToolDependency(const Dependency & dep)
{
    return isSystemDependency(dep) && (dep.getIdentifier() == SystemTools::getToolIdentifier());
}

bool isGenericSystemDependency(const Dependency & dep)
{
    return isSystemDependency(dep) && (dep.getIdentifier() == "system");
}

bool isSystemNeededElevation(const Dependency & dep)
{
#ifdef BOOST_OS_WINDOWS_AVAILABLE
     return (isSystemDependency(dep) && SystemTools::getToolIdentifier() == "choco");
#else
     return false;
#endif
}

DependencyManager::DependencyManager(const CmdOptions & options):m_options(options),m_cache(options)
{
}

fs::path DependencyManager::buildDependencyPath()
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path currentPath(fs::current_path().generic_string(utf8));

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
        retrieveDependencies(buildDependencyPath());
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
    std::vector<Dependency> dependencies = parse(depPath);
    for (auto dep : dependencies) {
        if (!dep.validate()) {
            bValid = false;
        }

        if (isSystemNeededElevation(dep)) {
            bNeedElevation = true;
        }
    }
    if (!bValid) {
        BOOST_LOG_TRIVIAL(error)<<"Semantic error parsing file "<<depPath<<" please fix the above dependency(ies) declaration error(s)";
        return -1;
    }
    if (bNeedElevation) {
        BOOST_LOG_TRIVIAL(warning)<<"Information of parsing file : remaken needs elevated privileges to install system Windows (choco) dependencies ";
        return -2;
    }
    BOOST_LOG_TRIVIAL(info)<<"File "<<depPath<<" successfully parsed";
    return 0;
}

int DependencyManager::bundle()
{
    try {
        bundleDependencies(buildDependencyPath());
    }
    catch (const std::runtime_error & e) {
        BOOST_LOG_TRIVIAL(error)<<e.what();
        return -1;
    }
    BOOST_LOG_TRIVIAL(warning)<<"bundle command under implementation : conan dependencies not yet supported, rpath not reinterpreted after copy";
    return 0;
}

static const std::map<const std::string_view,const  std::string_view> os2sharedSuffix = {
    {"mac",".dylib"},
    {"win",".dll"},
    {"unix",".so"},
    {"android",".so"},
    {"ios",".dylib"},
    {"linux",".so"}
};

const std::string_view & sharedSuffix(const std::string_view & osStr)
{
    if (os2sharedSuffix.find(osStr) == os2sharedSuffix.end()) {
        return os2sharedSuffix.at("unix");
    }
    return os2sharedSuffix.at(osStr);
}


void DependencyManager::bundleDependency(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
    fs::path rootLibDir = fileRetriever->computeRootLibDir(dependency);
    if (!fs::exists(rootLibDir)) { // ignoring header only dependencies
        BOOST_LOG_TRIVIAL(warning)<<"Ignoring "<<dependency.getName()<<" dependency: no shared library found from "<<rootLibDir;
        return;
    }
    for (fs::directory_entry& x : fs::directory_iterator(rootLibDir)) {
        fs::path filepath = x.path();
        if (fs::is_symlink(x.path())) {
            if (fs::exists(m_options.getDestinationRoot()/filepath.filename())) {
                fs::remove(m_options.getDestinationRoot()/filepath.filename());
            }
            fs::copy_symlink(x.path(),m_options.getDestinationRoot()/filepath.filename());
        }
        else if (is_regular_file(filepath)) {
            if (filepath.extension().string(utf8) == sharedSuffix(m_options.getOS())) {
                fs::copy_file(filepath , m_options.getDestinationRoot()/filepath.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
    }
    fs::path outputDirectory = fileRetriever->computeLocalDepencyRootDir(dependency);
    std::vector<fs::path> childrenDependencies = getChildrenDependencies(dependency,outputDirectory);
    for (fs::path childDependency : childrenDependencies) {
        if (fs::exists(childDependency)) {
            this->retrieveDependencies(childDependency);
        }
    }
}

void DependencyManager::bundleDependencies(const fs::path &  dependenciesFile)
{
    if (fs::exists(dependenciesFile)){
        std::vector<Dependency> dependencies = parse(dependenciesFile);
        for (auto dependency : dependencies) {
            if (dependency.getType() == Dependency::Type::REMAKEN) {
                if (!dependency.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                }

                if (isSystemNeededElevation(dependency) && !OsTools::isElevated()) {
                    throw std::runtime_error("Remaken needs elevated privileges to install system Windows " + SystemTools::getToolIdentifier() + " dependencies");
                }
            }
        }
        std::vector<std::shared_ptr<std::thread>> thread_group;
        for (Dependency const & dependency : dependencies) {
            if (dependency.getType() == Dependency::Type::REMAKEN) {
#ifdef REMAKEN_USE_THREADS
                thread_group.push_back(std::make_shared<std::thread>(&DependencyManager::bundleDependency,this, dependency));
#else
                bundleDependency(dependency);
#endif
            }
        }
        for (auto threadPtr : thread_group) {
            threadPtr->join();
        }
    }
}

std::vector<Dependency> removeRedundantDependencies(const std::multimap<std::string,Dependency> & dependencies)
{
    std::multimap<std::string,std::pair<std::size_t,Dependency>> dependencyMap;
    std::vector<Dependency> depVector;
    for (auto const & node : dependencies) {
        if (!isSystemDependency(node.second)) {
            depVector.push_back(std::move(node.second));
        }
        else {// System dependency
            if (dependencies.count(node.first) == 1) {
                // only one system dependency was found : generic or dedicated tool dependency is added to the dependencies vector
                depVector.push_back(std::move(node.second));
            }
            else {
                if (isSpecificSystemToolDependency(node.second)) {
                    // there must be one dedicated tool when the dependency appears twice : add only the dedicated dependency
                    depVector.push_back(std::move(node.second));
                }
            }
        }
    }
    return depVector;
}

std::vector<Dependency> DependencyManager::parse(const fs::path &  dependenciesPath)
{
    std::multimap<std::string,Dependency> libraries;
    if (fs::exists(dependenciesPath)) {
        ifstream fis(dependenciesPath.generic_string(),ios::in);
        while (!fis.eof()) {
            string curStr;
            getline(fis,curStr);
            if (!curStr.empty()) {
                Dependency dep(curStr, m_options.getMode());
                if (isGenericSystemDependency(dep)||isSpecificSystemToolDependency(dep)||!isSystemDependency(dep)) {
                    // only add "generic" system or tool@system or other deps
                    libraries.insert(std::make_pair(dep.getName(), std::move(dep)));
                }
            }
        }
        fis.close();
    }
    return removeRedundantDependencies(libraries);
}


void DependencyManager::retrieveDependency(Dependency &  dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
    std::string source = fileRetriever->computeSourcePath(dependency);
    if (!m_cache.contains(source) || !m_options.useCache()){
        try {
            std::cout<<"=> Installing "<<dependency.getRepositoryType()<<"::"<<source<<std::endl;
            fs::path outputDirectory;
            try {
                outputDirectory = fileRetriever->installArtefact(dependency);
            }
            catch (std::runtime_error & e) { // try alternate repository
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(m_options,true);
                dependency.changeBaseRepository(m_options.getAlternateRepoUrl());
                source = fileRetriever->computeSourcePath(dependency);
                fileRetriever->installArtefact(dependency);
            }

            if (m_options.useCache()) {
                m_cache.add(source);
            }
            std::vector<fs::path> childrenDependencies = getChildrenDependencies(dependency,outputDirectory);
            for (fs::path childDependency : childrenDependencies) {
                if (fs::exists(childDependency)) {
                    this->retrieveDependencies(childDependency);
                }
            }
        }
        catch (const std::runtime_error & e) {
            throw std::runtime_error(e.what());
        }
    }
    else {
        std::cout<<"=> Dependency "<<source<<" found in cache : already installed"<<std::endl;
    }
}

void DependencyManager::retrieveDependencies(const fs::path &  dependenciesFile)
{
    if (fs::exists(dependenciesFile)){
        std::vector<Dependency> dependencies = parse(dependenciesFile);
        for (auto dep : dependencies) {
            if (!dep.validate()) {
                throw std::runtime_error("Error parsing dependency file : invalid format ");
            }

            if (isSystemNeededElevation(dep) && !OsTools::isElevated()) {
                throw std::runtime_error("Remaken needs elevated privileges to install system Windows " + SystemTools::getToolIdentifier() + " dependencies");
            }
        }
        std::vector<std::shared_ptr<std::thread>> thread_group;
        for (Dependency & dependency : dependencies) {
#ifdef REMAKEN_USE_THREADS
            thread_group.push_back(std::make_shared<std::thread>(&DependencyManager::retrieveDependency,this, dependency));
#else
            retrieveDependency(dependency);
#endif
        }
        for (auto threadPtr : thread_group) {
            threadPtr->join();
        }
    }
}

std::vector<fs::path> DependencyManager::getChildrenDependencies(const Dependency &  dependency, const fs::path &  outputDirectory)
{
    auto childrenDependencies = list<std::string>{"packagedependencies.txt"};
    childrenDependencies.push_back("packagedependencies-"+ m_options.getOS() + ".txt");
    std::vector<fs::path> subDepsPath;
    for (std::string childDependency : childrenDependencies) {
        fs::path chidrenPackageDependenciesPath = outputDirectory / dependency.getPackageName() / dependency.getVersion() / childDependency;
        if (fs::exists(chidrenPackageDependenciesPath)) {
            subDepsPath.push_back(fs::absolute(chidrenPackageDependenciesPath));
        }
    }
    return subDepsPath;
}



