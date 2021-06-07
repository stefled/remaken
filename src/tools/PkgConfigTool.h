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
#include "CmdOptions.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class PkgConfigTool
{
public:
    PkgConfigTool();
    virtual ~PkgConfigTool() = default;
    void addPath(const fs::path & pkgConfigPath);
    std::string libs(const std::string & name);
    std::string cflags(const std::string & name);
    fs::path generateQmake(const std::vector<std::string>&  cflags, const std::vector<std::string>&  libs,
                           const std::string & prefix, const fs::path & destination);

protected:
    static std::string getPkgConfigToolIdentifier();
    fs::path m_pkgConfigToolPath;
    std::string m_pkgConfigPaths;
};

#endif // GITTOOL_H
