#include "DepTools.h"
#include "Constants.h"
#include "FileHandlerFactory.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/log/trivial.hpp>
#include <regex>
#include <boost/algorithm/string.hpp>

using namespace std;

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

fs::path DepTools::getProjectBuildSubFolder(const CmdOptions & options)
{
    fs::path targetPath = options.getProjectRootPath() / "build" / options.getConfig();
    return targetPath;
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

std::map<std::string,bool>  DepTools::parseConditionsFile(const fs::path &  rootFolderPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path configureFilePath = rootFolderPath/"configure_conditions.pri";
    std::map<std::string,bool> conditionsMap;

    if (!fs::exists(configureFilePath)) {
        return conditionsMap;
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
                conditionsMap.insert({conditionValue,true});
            }
       // }
    }
    configureFile.close();
    return conditionsMap;
}

std::vector<Dependency> DepTools::filterConditionDependencies(const std::map<std::string,bool> & conditions, const std::vector<Dependency> & depCollection)
{
    std::vector<Dependency> filteredDepCollection;
    for (auto const & dep : depCollection) {
        bool conditionsFullfilled = true;
        if (dep.hasConditions()) {
            for (auto & condition : dep.getConditions()) {
                if (!mapContains(conditions, condition)) {
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


