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

#ifndef SYSTEMFILERETRIEVER_H
#define SYSTEMFILERETRIEVER_H
#include <string>
#include "Constants.h"
#include "CmdOptions.h"
#include "AbstractFileRetriever.h"
#include "tools/SystemTools.h"
#include <optional>

class SystemFileRetriever : public AbstractFileRetriever
{
public:
    SystemFileRetriever(const CmdOptions & options, Dependency::Type dependencyType = Dependency::Type::SYSTEM);
    ~SystemFileRetriever() override = default;


    fs::path bundleArtefact(const Dependency & dependency) override;
    fs::path installArtefactImpl(const Dependency & dependency) override;
    fs::path retrieveArtefact(const Dependency & dependency) override;
    void addArtefactRemoteImpl(const Dependency & dependency) override;
    std::vector<fs::path> binPaths(const Dependency & dependency) override;
    std::vector<fs::path> libPaths(const Dependency & dependency) override;
    std::vector<fs::path> includePaths(const Dependency & dependency) override;
    std::string computeSourcePath( const Dependency &  dependency) override;
    std::pair<std::string, fs::path> invokeGenerator(std::vector<Dependency> & deps) override;
    void write_pkg_file(std::vector<Dependency> & deps) override;


protected:
    std::shared_ptr<BaseSystemTool> m_tool;
    fs::path m_scriptFilePath;
};

#endif // SYSTEMFILERETRIEVER_H
