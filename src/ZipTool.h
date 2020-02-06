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

#ifndef ZIPTOOL_H
#define ZIPTOOL_H
#include <string>
#include "CmdOptions.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ZipTool
{
public:
    ZipTool( const std::string & tool, bool quiet = true);
    virtual ~ZipTool() = default;
    virtual int uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder) = 0;
    static std::shared_ptr<ZipTool> createZipTool(const CmdOptions & options);
     static std::string getZipToolIdentifier();
protected:
    fs::path m_zipToolPath;
    bool m_quiet;
};

#endif // ZIPTOOL_H
