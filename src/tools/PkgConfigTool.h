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

#ifndef GITTOOL_H
#define GITTOOL_H
#include <string>
#include "Constants.h"
#include "CmdOptions.h"
#include "Dependency.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class PkgConfigTool
{
public:
    PkgConfigTool(const CmdOptions & options);
    virtual ~PkgConfigTool() = default;
    void addPath(const fs::path & pkgConfigPath);

    void libs(Dependency & dep, const std::vector<std::string> & options = {});
    void cflags(Dependency & dep, const std::vector<std::string> & options = {});
    std::pair<std::string, fs::path> generate(const std::vector<Dependency> & deps, Dependency::Type depType);


protected:
    static std::string getPkgConfigToolIdentifier();
    fs::path m_pkgConfigToolPath;
    std::string m_pkgConfigPaths;
    const CmdOptions & m_options;
};

#endif // GITTOOL_H
