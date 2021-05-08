#include "DepTools.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/log/trivial.hpp>
#include <regex>

using namespace std;

fs::path DepTools::buildDependencyPath(const std::string & filePath)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path currentPath(boost::filesystem::initial_path().generic_string(utf8));

    fs::path dependenciesFile (filePath, utf8);

    if (!dependenciesFile.is_absolute()){
        dependenciesFile = currentPath /dependenciesFile;
    }

    if (!fs::exists(dependenciesFile)) {
        throw std::runtime_error("The file does not exists " + dependenciesFile.generic_string(utf8));
    }
    return dependenciesFile;
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

std::vector<Dependency> DepTools::parse(const fs::path &  dependenciesPath, const std::string & linkMode)
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

void DepTools::parseRecurse(const fs::path &  dependenciesPath, const CmdOptions & options, std::vector<Dependency> & deps)
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

std::vector<fs::path> DepTools::getChildrenDependencies(const fs::path &  outputDirectory, const std::string & osPlatform, const std::string & filePrefix)
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

void DepTools::readInfos(const fs::path &  dependenciesFile, const CmdOptions & options, uint32_t indentLevel)
{
    indentLevel ++;
    std::vector<fs::path> dependenciesFileList = DepTools::getChildrenDependencies(dependenciesFile.parent_path(), options.getOS());
    for (fs::path depsFile : dependenciesFileList) {
        if (fs::exists(depsFile)) {
            std::vector<Dependency> dependencies = DepTools::parse(depsFile, options.getMode());
            for (auto dep : dependencies) {
                if (!dep.validate()) {
                    throw std::runtime_error("Error parsing dependency file : invalid format ");
                    indentLevel --;
                }
            }
            for (Dependency & dependency : dependencies) {
                displayInfo(dependency, indentLevel);
                fs::detail::utf8_codecvt_facet utf8;
                shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, options);
                fs::path outputDirectory = fileRetriever->computeLocalDependencyRootDir(dependency);
                readInfos(outputDirectory/"packagedependencies.txt", options, indentLevel);
            }
        }
    }
    indentLevel --;
}


