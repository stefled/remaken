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
#include "tools/ConanSystemTool.h"

class ConanFileRetriever : public SystemFileRetriever
{
public:
    ConanFileRetriever(const CmdOptions & options);
    ~ConanFileRetriever() override = default;
    fs::path bundleArtefact(const Dependency & dependency) override;
    fs::path createConanFile(const fs::path & projectFolderPath);
    fs::path invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator) override;

protected:
    std::vector<std::string> buildOptions(const Dependency & dep);

};

#endif // CONANFILERETRIEVER_H
