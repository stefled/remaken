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

#ifndef OSUTILS_H
#define OSUTILS_H

#include "CmdOptions.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class OsUtils
{
public:
    OsUtils() = delete;
    ~OsUtils() = delete;
    static bool isElevated();
    static void copySharedLibraries(const fs::path & sourceRootFolder, const CmdOptions & options);
    static void copyStaticLibraries(const fs::path & sourceRootFolder, const CmdOptions & options);
    static void copyLibraries(const fs::path & sourceRootFolder, const CmdOptions & options, std::function<const std::string_view &(const std::string_view &)> suffixFunction);
    static void copyLibraries(const fs::path & sourceRootFolder, const fs::path & destinationFolderPath, const std::string_view & suffix);
    static const std::string_view & sharedSuffix(const std::string_view & osStr);
    static const std::string_view & staticSuffix(const std::string_view & osStr);
    static const std::string_view & sharedLibraryPathEnvName(const std::string_view & osStr);
    static fs::path computeRemakenRootPackageDir(const CmdOptions & options);
    static void copyFolder(const fs::path & srcFolderPath, const fs::path & dstFolderPath, bool bRecurse);


    static fs::path acquireTempFolderPath();
    static void releaseTempFolderPath(const fs::path & tmpDir);
};

#endif // OSUTILS_H
