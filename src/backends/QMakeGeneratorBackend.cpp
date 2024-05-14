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

#include "backends/QMakeGeneratorBackend.h"
#include <fstream>
#include <ios>
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

std::pair<std::string, fs::path> QMakeGeneratorBackend::generate(const std::vector<Dependency> & deps, Dependency::Type depType)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (!mapContains(type2prefixMap,depType)) {
        throw std::runtime_error("Error dependency type " + std::to_string(static_cast<uint32_t>(depType)) + " no supported");
    }
    std::string prefix = type2prefixMap.at(depType);
    std::string filename = boost::to_lower_copy(prefix) + getGeneratorFileName("buildinfo");
    fs::path filePath = DepUtils::getProjectBuildSubFolder(m_options)/filename;
    std::ofstream fos(filePath.generic_string(utf8),std::ios::out);
    std::string libdirs, libsStr, defines, cflags;
    for (auto & dep : deps ) {
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
                    cflags += " \"" + cflag + "\"";
                }
                else if (cflagPrefix == "D") {
                    // remove -D
                    cflag.erase(0,1);
                    boost::trim(cflag);
                    defines += " " + cflag;
                }
            }
        }
        for (auto & define : dep.defines()) {
            defines += " " + define;
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
                    libdirs += " -L\"" + option + "\"";
                }
                //TODO : extract lib paths from libdefs and put quotes around libs path
                else {
                    boost::trim(option);
                    libsStr += " -" + option;
                }
            }
        }
        for (auto & libdir : dep.libdirs()) {
            libdirs += " -L\"" + libdir + "\"";
        }
    }
    fos<<prefix<<"_INCLUDEPATH +="<<cflags<<std::endl;
    fos<<prefix<<"_LIBS +="<<libsStr<<std::endl;
    fos<<prefix<<"_SYSTEMLIBS += "<<std::endl;
    fos<<prefix<<"_FRAMEWORKS += "<<std::endl;
    fos<<prefix<<"_FRAMEWORKS_PATH += "<<std::endl;
    fos<<prefix<<"_LIBDIRS +="<<libdirs<<std::endl;
    fos<<prefix<<"_BINDIRS +="<<std::endl;

    fos<<prefix<<"_DEFINES += "<<defines<<std::endl;
    fos<<prefix<<"_QMAKE_CXXFLAGS +="<<std::endl;
    fos<<prefix<<"_QMAKE_CFLAGS +="<<std::endl;
    fos<<prefix<<"_QMAKE_LFLAGS +="<<std::endl;
    fos<<std::endl;
    fos<<"INCLUDEPATH += $$"<<prefix<<"_INCLUDEPATH"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_LIBS"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_LIBDIRS"<<std::endl;
    fos<<"BINDIRS += $$"<<prefix<<"_BINDIRS"<<std::endl;
    fos<<"DEFINES += $$"<<prefix<<"_DEFINES"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_FRAMEWORKS"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_FRAMEWORK_PATHS"<<std::endl;
    fos<<"QMAKE_CXXFLAGS += $$"<<prefix<<"_QMAKE_CXXFLAGS"<<std::endl;
    fos<<"QMAKE_CFLAGS += $$"<<prefix<<"_QMAKE_CFLAGS"<<std::endl;
    fos<<"QMAKE_LFLAGS += $$"<<prefix<<"_QMAKE_LFLAGS"<<std::endl;
    fos.close();
    return {boost::to_lower_copy(prefix)+"_basic_setup", filePath};
}


void QMakeGeneratorBackend::generateIndex(std::map<std::string,fs::path> setupInfos)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path buildProjectSubFolderPath = DepUtils::getProjectBuildSubFolder(m_options);
    fs::path depsInfoFilePath = buildProjectSubFolderPath / getGeneratorFileName("dependenciesBuildInfo"); // extension should later depend on generator type
    std::ofstream depsOstream(depsInfoFilePath.generic_string(utf8),std::ios::out);
    for (auto & kv : setupInfos) {
        depsOstream<<"CONFIG += "<<kv.first<<std::endl;
    }
    fs::path  buildSubFolderPath = DepUtils::getBuildSubFolder(m_options);
    for (auto & kv : setupInfos) {
        fs::path setupFilePath = kv.second.filename();
        if (!setupFilePath.empty()) {
            depsOstream<<"include($$_PRO_FILE_PWD_/"<<buildSubFolderPath.generic_string(utf8)<<"/"<<setupFilePath.filename().generic_string(utf8)<<")\n";
        }
    }
    depsOstream.close();
}

void QMakeGeneratorBackend::generateConfigureConditionsFile(const fs::path &  rootFolderPath, const std::vector<Dependency> & deps)
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
    for (auto & dep : deps) {
        for (auto & condition : dep.getConditions()) {
            configureFile << "DEFINES += " << condition;
            configureFile << "\n";
        }
    }
    configureFile.close();
}

void QMakeGeneratorBackend::parseConditionsFile(const fs::path &  rootFolderPath, std::map<std::string,bool> & conditionsMap)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path configureFilePath = rootFolderPath / getGeneratorFileName("configure_conditions");
    //std::map<std::string,bool> conditionsMap;

    if (!fs::exists(configureFilePath)) {
        return;
    }

    std::ifstream configureFile(configureFilePath.generic_string(utf8).c_str(), std::ios::in);
    while (!configureFile.eof()) {
        std::vector<std::string> results;
        std::string curStr;
        getline(configureFile,curStr);
        //std::string formatRegexStr = "^[\t\s]*DEFINES[\t\s]*+=[\t\s]*[a-zA-Z0-9_-]*";
        //std::regex formatRegexr(formatRegexStr, std::regex_constants::extended);
        std::smatch sm;
        //check string format is ^[\t\s]*DEFINES[\t\s]*+=[\t\s]*[a-zA-Z0-9_-]*
        // if (std::regex_search(curStr, sm, formatRegexr, std::regex_constants::match_any)) {
        boost::split(results, curStr, [](char c){return c == '=';});
        if (results.size() == 2) {
            std::string definesValue = results[0];
            std::string conditionValue = results[1];
            boost::algorithm::erase_all(definesValue, " ");
            boost::trim(conditionValue);
            if (definesValue == "DEFINES+") {
                conditionsMap.insert_or_assign(conditionValue, true);
            }
            if (definesValue == "DEFINES-") {
                conditionsMap.insert_or_assign(conditionValue, false);
            }
        }
        // }
    }
    configureFile.close();
}
