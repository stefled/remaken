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

#ifndef DEPTOOLS_H
#define DEPTOOLS_H

#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Dependency.h"
#include "CmdOptions.h"
#include "Cache.h"
#include "tinyxmlhelper.h"

namespace fs = boost::filesystem;

class DepTools
{
public:
    DepTools() = delete;
    ~DepTools() = delete;
    static fs::path buildDependencyPath(const std::string & filePath);
    static fs::path getProjectBuildSubFolder(const CmdOptions & options);
    static std::vector<fs::path> getChildrenDependencies(const fs::path & outputDirectory, const std::string & osPlatform, const std::string & filePrefix = "packagedependencies");
    static std::vector<Dependency> parse(const fs::path & dependenciesPath, const std::string & linkMode);
    // parseRecurse appends found dependencies to the deps vector - even duplicates.
    static void parseRecurse(const fs::path & dependenciesPath, const CmdOptions & options, std::vector<Dependency> & deps);
    static void readInfos(const fs::path &  dependenciesFile, const CmdOptions & options, uint32_t indentLevel = 0);
    static std::map<std::string,bool> parseConditionsFile(const fs::path & rootFolderPath);
    static std::vector<Dependency> filterConditionDependencies(const std::map<std::string,bool> & conditions, const std::vector<Dependency> & depCollection);
};

bool yesno_prompt(char const* prompt);

#endif // DEPTOOLS_H
