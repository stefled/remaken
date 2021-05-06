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

#ifndef ABSTRACTFILERETRIEVER_H
#define ABSTRACTFILERETRIEVER_H

#include "IFileRetriever.h"
#include "CmdOptions.h"
#include "tools/ZipTool.h"

class AbstractFileRetriever : public IFileRetriever
{
public:
    AbstractFileRetriever(const CmdOptions & options);
    virtual ~AbstractFileRetriever() override;
    virtual fs::path installArtefact(const Dependency & dependency) override final;
    virtual fs::path bundleArtefact(const Dependency & dependency) override;
    virtual std::vector<fs::path> binPaths(const Dependency & dependency) override;
    virtual std::vector<fs::path> libPaths(const Dependency & dependency) override;
    virtual std::string computeSourcePath( const Dependency &  dependency) override;
    virtual fs::path computeRootBinDir( const Dependency & dependency) override;
    virtual fs::path computeRootLibDir( const Dependency & dependency) override;
    virtual fs::path computeLocalDependencyRootDir( const Dependency &  dependency) override;
    virtual const std::vector<Dependency> & installedDependencies() const override final { return m_installedDeps; }

protected:
    virtual fs::path installArtefactImpl(const Dependency & dependency);
    virtual void processPostInstallActions();
    void copySharedLibraries(const fs::path & sourceRootFolder);
    fs::path m_workingDirectory;
    const CmdOptions & m_options;
    std::shared_ptr<ZipTool> m_zipTool;
    std::vector<Dependency> m_installedDeps;

};

#endif // ABSTRACTCOMMAND_H
