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

#ifndef CONANSYSTEMTOOL_H
#define CONANSYSTEMTOOL_H

#include "SystemTools.h"

class ConanSystemTool : public BaseSystemTool
{
public:
/*    typedef enum {
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
    } GeneratorType;*/

    ConanSystemTool(const CmdOptions & options):BaseSystemTool(options, "conan") {
        m_conanVersion = conanVersion();
    }
    ~ConanSystemTool() override = default;
    void update() override;
    void bundle(const Dependency & dependency) override;
    void bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile) override {}
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;
    void search (const std::string & pkgName, const std::string & version) override;
    std::vector<fs::path> binPaths(const Dependency & dependency) override;
    std::vector<fs::path> libPaths(const Dependency & dependency) override;
    std::vector<fs::path> includePaths(const Dependency & dependency) override;
    std::string computeSourcePath( const Dependency &  dependency) override;
    std::pair<std::string, fs::path> invokeGenerator(std::vector<Dependency> & deps) override;
    void listRemotes() override;
    void addRemote(const std::string & remoteReference) override;
    void write_pkg_file(std::vector<Dependency> & deps) override;

private:
    void addRemoteImpl(const std::string & repositoryUrl);
    std::string retrieveInstallCommand(const Dependency & dependency) override;
    std::vector<fs::path> retrievePaths(const Dependency & dependency, BaseSystemTool::PathType conanNode, const fs::path & destination);
    fs::path createConanFile(const std::vector<Dependency> & deps);
    std::string computeToolRef( const Dependency &  dependency) override;
    std::string computeConanRef( const Dependency &  dependency, bool cliMode = false);
    void translateJsonToRemakenDep(std::vector<Dependency> & deps, const fs::path & conanJsonBuildInfo);
    std::vector<std::string> buildOptions(const Dependency & dep);
    int conanVersion();

// Members
private:
    int m_conanVersion;
};

#endif
