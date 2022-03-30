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

class GitTool
{
public:
    GitTool(bool override= false);
    virtual ~GitTool() = default;
    int clone(const std::string & url, const fs::path & destinationRootFolder, bool recurseSubModule = false);
    int clone(const std::string & url, const fs::path & destinationRootFolder, const std::string & tag, bool recurseSubModule = false);

protected:
    static std::string getGitToolIdentifier();
    fs::path m_gitToolPath;
    bool m_override;
};

#endif // GITTOOL_H
