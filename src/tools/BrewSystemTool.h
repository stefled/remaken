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

#ifndef BREWSYSTEMTOOL_H
#define BREWSYSTEMTOOL_H

#include "SystemTools.h"

class BrewSystemTool : public BaseSystemTool
{
public:
    BrewSystemTool(const CmdOptions & options):BaseSystemTool(options, "brew") {}
    ~BrewSystemTool() override = default;
    void update() override;    
    void bundle(const Dependency & dependency) override;
    void bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile) override {}
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;
    void search (const std::string & pkgName, const std::string & version) override;
    fs::path invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator) override;
    std::vector<fs::path> binPaths([[maybe_unused]] const Dependency & dependency) override;
    std::vector<fs::path> libPaths([[maybe_unused]] const Dependency & dependency) override;
    void listRemotes() override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
    void tap(const std::string & repositoryUrl);
    void bundleLib(const std::string & libPath);
    std::string computeToolRef( const Dependency &  dependency) override;
};

#endif
