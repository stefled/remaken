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

#ifndef VCPKGSYSTEMTOOL_H
#define VCPKGSYSTEMTOOL_H

#include "SystemTools.h"

class VCPKGSystemTool : public BaseSystemTool
{
public:
    fs::detail::utf8_codecvt_facet utf8;
    VCPKGSystemTool(const CmdOptions & options):BaseSystemTool(options, "vcpkg") {}
    ~VCPKGSystemTool() override = default;
    void update() override;
    void bundle(const Dependency & dependency) override;
    void bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile) override {}
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;
    void search (const std::string & pkgName, const std::string & version) override;
    std::vector<fs::path> binPaths(const Dependency & dependency) override;
    std::vector<fs::path> libPaths(const Dependency & dependency) override;
    std::vector<fs::path> includePaths(const Dependency & dependency) override;
    void listRemotes() override { BOOST_LOG_TRIVIAL(info)<<"remote() is unique for "<<m_systemInstallerPath; }
    void addRemote([[maybe_unused]] const std::string & remoteReference) override {
        BOOST_LOG_TRIVIAL(warning)<<"addRemote() not implemented yet for tool "<<m_systemInstallerPath;
    }


protected:
    void bundleLib(const std::string & libPath);
    fs::path computeLocalDependencyRootDir( const Dependency &  dependency);
    std::vector<fs::path> retrievePaths(const Dependency & dependency, BaseSystemTool::PathType pathType);
    std::string computeToolRef( const Dependency &  dependency) override;


private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};


#endif
