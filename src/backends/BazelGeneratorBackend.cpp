/**
 * @copyright Copyright (c) 2019 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Loïc Touraine
 *
 * @file
 * @brief description of file
 * @date 2019-11-15
 */
#include "backends/BazelGeneratorBackend.h"
#include "utils/OsUtils.h"
#include <regex>

static const std::map<Dependency::Type,std::string> type2prefixMap = {
    {Dependency::Type::BREW,"BREW"},
    {Dependency::Type::REMAKEN,"REMAKEN"},
    {Dependency::Type::CONAN,"CONAN"},
    {Dependency::Type::CHOCO,"CHOCO"},
    {Dependency::Type::SYSTEM,"SYSTEM"},
    {Dependency::Type::SCOOP,"SCOOP"},
    {Dependency::Type::VCPKG,"VCPKG"}
};

BazelGeneratorBackend::BazelGeneratorBackend(const CmdOptions & options):AbstractGeneratorBackend(options,".bzl") {
    fs::detail::utf8_codecvt_facet utf8;
    fs::path filePath = DepUtils::getProjectBuildSubFolder(m_options)/"BUILD";
    std::ofstream buildFile(filePath.generic_string(utf8),std::ios::out);
    buildFile.close();
}

std::string getValidName(const std::string & originalName)
{
    std::string validName = originalName;
    if (originalName.find("+")) {
        boost::replace_all(validName,"+","p");
    }
    return validName;
}

std::pair<std::string, fs::path> BazelGeneratorBackend::generate(const std::vector<Dependency> & deps, Dependency::Type depType)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (!mapContains(type2prefixMap,depType)) {
        throw std::runtime_error("Error dependency type " + std::to_string(static_cast<uint32_t>(depType)) + " no supported");
    }
    std::string prefix = type2prefixMap.at(depType);
    std::string filename = boost::to_lower_copy(prefix) + getGeneratorFileName("buildinfo");
    fs::path filePath = DepUtils::getProjectBuildSubFolder(m_options)/filename;
    std::ofstream fos(filePath.generic_string(utf8),std::ios::out);
    std::string defines;
    std::vector<std::string> setupFuncVect;
    for (auto dep : deps ) {
        std::string bazelPkgName = getValidName(dep.getName());

        std::map<std::string,bool> incPaths, defines;
        std::vector<std::string> libsVect;
        for (auto & cflagInfos : dep.cflags()) {
            std::vector<std::string> cflagsVect;
            boost::split_regex(cflagsVect, cflagInfos, boost::regex( " -" ));
            for (auto & cflag: cflagsVect) {
                if (cflag[0] == '-') {
                    cflag.erase(0,1);
                }
                std::string cflagPrefix = cflag.substr(0,1);
                if (cflagPrefix == "I") {
                    // remove -I
                    cflag.erase(0,1);
                    boost::trim(cflag);
                    fs::path incPath = OsUtils::extractPath(dep.prefix(),cflag);
                    fs::path symlinkFolder(bazelPkgName, utf8);
                    if (!mapContains(incPaths,(symlinkFolder/incPath).generic_string(utf8))) {
                        incPaths.insert({(symlinkFolder/incPath).generic_string(utf8),true});
                    }
                }
                else if (cflagPrefix == "D") {
                    // remove -D
                    cflag.erase(0,1);
                    boost::trim(cflag);
                    if (!mapContains(defines,cflag)) {
                        defines.insert({cflag,true});
                    }
                }
            }
        }
        for (auto & libInfos : dep.libs()) {
            std::vector<std::string> optionsVect;
            boost::split_regex(optionsVect, libInfos, boost::regex( " -" ));
            for (auto & option: optionsVect) {
                if (option[0] == '-') {
                    option.erase(0,1);
                }
                std::string optionPrefix = option.substr(0,1);
                if (optionPrefix == "L") {
                    option.erase(0,1);
                    dep.libdirs().push_back(option);
                }
                //TODO : extract lib paths from libdefs and put quotes around libs path
                else {
                    boost::trim(option);
                    libsVect.push_back(option);
                }
            }
        }

        fos<<"def _setup_"<<bazelPkgName<<"_impl(ctx):"<<std::endl;
        fos<<"    "<< bazelPkgName<<"_root = \""<<dep.prefix()<<"\""<<std::endl;
        fos<<std::endl;
        fos<<"    ctx.symlink("<<bazelPkgName<<"_root, \""<<bazelPkgName<<"\")"<<std::endl;
        fos<<"    ctx.file(\"BUILD\", \"\"\""<<std::endl;
        fos<<"cc_library("<<std::endl;
        fos<<"    name = \""<<bazelPkgName<<"\","<<std::endl;
        fos<<"    hdrs = glob([";
        bool start = true;
        for (auto & [incP,b] : incPaths) {
            if (!start) {
                fos<<", ";
            }
            fos<<"\""<<incP<<"/**/*.h\", \""<<incP<<"/**/*.hpp\", \""<<incP<<"/**/*.ipp\"";
            start = false;
        }
        fos<<"]),"<<std::endl;
        start = true;
        fos<<"    includes = [";
        for (auto & [incP,b]: incPaths) {
            if (!start) {
                fos<<", ";
            }
            fos<<"\""<<incP<<"\"";
            start = false;
        }
        fos<<"],"<<std::endl;
        start = true;
        fos<<"    defines = [";
        for (auto & [def,b] : defines) {
            if (!start) {
                fos<<", ";
            }
            std::string define = boost::replace_all_copy(def,"\"","\\\"");
            fos<<"\""<<define<<"\"";
            start = false;
        }
        fos<<"],"<<std::endl;
        fos<<"    visibility = [\"//visibility:public\"],"<<std::endl;
        fos<<"    linkopts = ["<<std::endl;
        fos<<"        \"-Wl,--start-group\","<<std::endl;
        for (auto & libFolder : dep.libdirs()) {
            fs::path libPath = OsUtils::extractPath(dep.prefix(),libFolder);
            fs::path symlinkFolder(bazelPkgName, utf8);
            fos<<"        \"-L"<<(symlinkFolder/libPath).generic_string(utf8)<<"\","<<std::endl;
        }
        for (auto & lib : libsVect) {
            fos<<"        \"-"<<lib<<"\","<<std::endl;
        }
        fos<<"        \"-Wl,--end-group\"],"<<std::endl;
        fos<<")"<<std::endl;
        fos<<"\"\"\")"<<std::endl;
        fos<<std::endl;
        fos<<"_setup_"<<bazelPkgName<<" = repository_rule("<<std::endl;
        fos<<"    implementation = _setup_"<<bazelPkgName<<"_impl"<<std::endl;
        fos<<")"<<std::endl;
        fos<<std::endl;
        setupFuncVect.push_back("setup_"+ bazelPkgName + "()");
        fos<<"def setup_"<<bazelPkgName<<"():"<<std::endl;
        fos<<"    print(\"--> Setup "<<dep.getName()<<" dependency\")"<<std::endl;
        fos<<"    _setup_"<<bazelPkgName<<"(name = \""<<bazelPkgName<<"\")"<<std::endl;
        fos<<std::endl;
    }
    std::string setupFunc = "setup_" + boost::to_lower_copy(prefix) + "_deps";
    fos<<"def "<<setupFunc<<"():"<<std::endl;
    fos<<"    print(\"--> Setup "<<boost::to_lower_copy(prefix)<<" dependencies\")"<<std::endl;
    for (auto & setupFunc : setupFuncVect) {
        fos<<"    "<<setupFunc<<std::endl;
    }
    fos.close();
    return {setupFunc,filePath};
}

void BazelGeneratorBackend::generateIndex(std::map<std::string,fs::path> setupInfos)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path buildProjectSubFolderPath = DepUtils::getProjectBuildSubFolder(m_options);
    fs::path depsInfoFilePath = buildProjectSubFolderPath / getGeneratorFileName("dependenciesBuildInfo"); // extension should later depend on generator type
    std::ofstream depsOstream(depsInfoFilePath.generic_string(utf8),std::ios::out);

    fs::path  buildSubFolderPath = DepUtils::getBuildSubFolder(m_options);
    for (auto & kv : setupInfos) {
        fs::path setupFilePath = kv.second.filename();
        if (!setupFilePath.empty()) {
            depsOstream<<"load(\"//"<<buildSubFolderPath.generic_string(utf8)<<":"<<setupFilePath.filename().generic_string(utf8)<<"\",\""<<kv.first<<"\")"<<std::endl;
        }
    }

    depsOstream<<"def setup_all_remaken_deps():"<<std::endl;
    depsOstream<<"    print(\"Setup all remaken dependencies\")"<<std::endl;
    for (auto & kv : setupInfos) {
        depsOstream<<"    "<<kv.first<<"()"<<std::endl;
    }
    depsOstream.close();
}

void BazelGeneratorBackend::generateConfigureConditionsFile(const fs::path &  rootFolderPath, const std::vector<Dependency> & deps)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path buildFolderPath = rootFolderPath/DepUtils::getBuildPlatformFolder(m_options);
    fs::path configureFilePath = buildFolderPath / getGeneratorFileName("configure_conditions");
    if (fs::exists(configureFilePath) ) {
        fs::remove(configureFilePath);
    }

    if (deps.empty()) {
        return;
    }

    if (!fs::exists(buildFolderPath)) {
        fs::create_directories(buildFolderPath);
    }
    std::ofstream configureFile(configureFilePath.generic_string(utf8).c_str(), std::ios::out);
    configureFile << "REMAKENDEFINES = [";
    bool start = true;
    for (auto & dep : deps) {
        for (auto & condition : dep.getConditions()) {
            if (!start) {
                configureFile <<" ,";
            }
            configureFile << condition;
            start = false;

        }
    }
    configureFile << "]\n";
    configureFile.close();
}

void BazelGeneratorBackend::parseConditionsFile(const fs::path &  rootFolderPath, std::map<std::string,bool> & conditionsMap)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path configureFilePath = rootFolderPath / getGeneratorFileName("configure_conditions");
    if (!fs::exists(configureFilePath)) {
        return;
    }

    std::ifstream configureFile(configureFilePath.generic_string(utf8).c_str(), std::ios::in);
    while (!configureFile.eof()) {
        std::vector<std::string> results;
        std::string curStr;
        getline(configureFile,curStr);
        std::string formatRegexStr = "^[\t ]*REMAKENDEFINES[\t ]*=[\t ]*[\[]+([a-zA-Z0-9_, ]*)[\]]+";
        std::regex formatRegexr(formatRegexStr, std::regex_constants::extended);
        std::smatch sm;
        if (std::regex_search(curStr, sm, formatRegexr, std::regex_constants::match_any)) {
            if (sm.size() >= 2) {
                std::string defines = sm[1];
                boost::split(results, defines, [](char c){return c == ',';});
                for (auto & result: results) {
                    std::string conditionValue = result;
                    boost::trim(conditionValue);
                    conditionsMap.insert_or_assign(conditionValue, true);
                }
            }
        }
    }
    configureFile.close();
}
