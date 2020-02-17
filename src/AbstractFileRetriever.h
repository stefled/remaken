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
#include "ZipTool.h"

class AbstractFileRetriever : public IFileRetriever
{
public:
    AbstractFileRetriever(const CmdOptions & options);
    virtual ~AbstractFileRetriever() override;
    virtual fs::path installArtefact(const Dependency & dependency) override;
    virtual fs::path bundleArtefact(const Dependency & dependency) override;
    virtual std::string computeSourcePath( const Dependency &  dependency) override;
    virtual fs::path computeRootLibDir( const Dependency & dependency) override;
    virtual fs::path computeLocalDependencyRootDir( const Dependency &  dependency) override;
    virtual fs::path computeRemakenRootDir( const Dependency &  dependency) override;

protected:
    void copySharedLibraries(const fs::path & sourceRootFolder);
    void cleanUpWorkingDirectory();
    fs::path m_workingDirectory;
    CmdOptions m_options;
    std::shared_ptr<ZipTool> m_zipTool;

};

#endif // ABSTRACTCOMMAND_H
