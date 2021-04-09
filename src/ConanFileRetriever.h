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

#ifndef CONANFILERETRIEVER_H
#define CONANFILERETRIEVER_H

#include "SystemFileRetriever.h"

class ConanFileRetriever : public SystemFileRetriever
{
public:
    typedef enum {
        cmake = 0x01,
        cmake_multi = 0x02,
        cmake_paths = 0x04,
        cmake_find_package = 0x08,
        cmake_find_package_multi = 0x0F,
        compiler_args = 0x10,
        qmake = 0x20,
        pkg_config = 0x40,
        txt = 0x80,
        json = 0xF0,
        make = 0x100,
        markdown = 0x200,
        deploy = 0x400
    } GeneratorType;
    ConanFileRetriever(const CmdOptions & options);
    ~ConanFileRetriever() override = default;
    fs::path bundleArtefact(const Dependency & dependency) override;
    fs::path createConanFile(const fs::path & projectFolderPath, const std::vector<Dependency> & conanDeps);
    void invokeGenerator(const fs::path & conanFilePath, const fs::path & projectFolderPath, GeneratorType generator = GeneratorType::qmake);

};

#endif // CONANFILERETRIEVER_H
