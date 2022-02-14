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

static const std::map<Dependency::Type,std::string> type2prefixMap = {
    {Dependency::Type::BREW,"BREW"},
    {Dependency::Type::REMAKEN,"REMAKEN"},
    {Dependency::Type::CONAN,"CONAN"},
    {Dependency::Type::CHOCO,"CHOCO"},
    {Dependency::Type::SYSTEM,"SYSTEM"},
    {Dependency::Type::SCOOP,"SCOOP"},
    {Dependency::Type::VCPKG,"VCPKG"}
};

fs::path BazelGeneratorBackend::generate(const std::vector<Dependency> & deps, Dependency::Type depType)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (!mapContains(type2prefixMap,depType)) {
        throw std::runtime_error("Error dependency type " + std::to_string(static_cast<uint32_t>(depType)) + " no supported");
    }
    std::string prefix = type2prefixMap.at(depType);
    std::string filename = boost::to_lower_copy(prefix) + m_options.getGeneratorFilePath("buildinfo");
    fs::path filePath = DepUtils::getProjectBuildSubFolder(m_options)/filename;
    std::ofstream fos(filePath.generic_string(utf8),std::ios::out);
    std::string libdirs, libsStr, defines;
    for (auto & dep : deps ) {
        fos<<"def _setup_"<<dep.getName()<<"_impl(ctx):"<<std::endl;
        fos<<"    "<< dep.getName()<<"_root = \""<<dep.prefix()<<"\""<<std::endl;
        fos<<std::endl;
        fos<<"    ctx.symlink("<<dep.getName()<<"_root, \""<<dep.getName()<<"\")"<<std::endl;
        fos<<"    ctx.file(\"BUILD\", \"\"\""<<std::endl;
        fos<<"cc_library("<<std::endl;
        fos<<"    name = \""<<dep.getName()<<"\","<<std::endl;
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
                   // fos<<prefix<<"_INCLUDEPATH += \""<<cflag<<"\""<<std::endl;
                }
                else if (cflagPrefix == "D") {
                    // remove -D
                    cflag.erase(0,1);
                    boost::trim(cflag);
                    defines += " " + cflag;
                }
            }
        }
        //fos<<prefix<<"_DEFINES += "<<defines<<std::endl;
        fos<<"    visibility = [\"//visibility:public\"],"<<std::endl;
        fos<<"    linkopts = ["<<std::endl;
        fos<<"        \"-Wl,--start-group\","<<std::endl;
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
                    fos<<"        \"-L"<<option<<"\""<<std::endl;
                }
                //TODO : extract lib paths from libdefs and put quotes around libs path
                else {
                    boost::trim(option);
                    libsStr += " -" + option;
                    fos<<"        \"-"<<option<<"\""<<std::endl;
                }
            }
        }
        fos<<"        \"-Wl,--end-group\"],"<<std::endl;
        fos<<")"<<std::endl;
        fos<<"\"\"\".format("<<dep.getName()<<"_root = "<<dep.getName()<<"_root))"<<std::endl;
        fos<<std::endl;
        fos<<"_setup_"<<dep.getName()<<" = repository_rule("<<std::endl;
        fos<<"    implementation = _setup_"<<dep.getName()<<"_impl"<<std::endl;
        fos<<")"<<std::endl;
        fos<<std::endl;
        fos<<"def setup_"<<dep.getName()<<"():"<<std::endl;
        fos<<"    _setup_"<<dep.getName()<<"(name = \""<<dep.getName()<<"\")"<<std::endl;
    }
    fos.close();
    return filePath;
}

