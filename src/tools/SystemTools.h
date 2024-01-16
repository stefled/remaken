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

#ifndef SYSTEMTOOLS_H
#define SYSTEMTOOLS_H

#include "Constants.h"
#include "CmdOptions.h"
#include "Dependency.h"
#include <list>
#include <optional>
#include <boost/log/trivial.hpp>
#include "utils/DepUtils.h"

class BaseSystemTool
{
public:
    typedef enum {
       LIB_PATHS,
       BIN_PATHS,
       INCLUDE_PATHS
    } PathType;
    BaseSystemTool(const CmdOptions & options, const std::string & installer);
    virtual ~BaseSystemTool() = default;
    virtual void update() = 0;
    virtual void bundle ([[maybe_unused]] const Dependency & dependency) = 0;
    virtual void bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile) = 0;
    virtual void install (const Dependency & dependency) = 0;
    virtual void search (const std::string & pkgName, [[maybe_unused]] const std::string & version = "") = 0;
    virtual bool installed (const Dependency & dependency) = 0;
    virtual std::vector<fs::path> binPaths ([[maybe_unused]] const Dependency & dependency);
    virtual std::vector<fs::path> libPaths ([[maybe_unused]] const Dependency & dependency);
    virtual std::vector<fs::path> includePaths ([[maybe_unused]] const Dependency & dependency);
    virtual std::string computeSourcePath (const Dependency &  dependency);
    virtual fs::path sudo () { return m_sudoCmd; }
    bool bundleScripted() { return m_bundleScripted; }
    virtual std::pair<std::string, fs::path> invokeGenerator([[maybe_unused]] std::vector<Dependency> & deps);
    virtual void listRemotes() = 0;
    virtual void addRemote(const std::string & remoteReference) = 0;
    virtual void write_pkg_file(std::vector<Dependency> & deps);

protected:
    std::string run(const std::string & command, const std::vector<std::string> & options = {});
    std::string run(const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue);
    std::string run(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options = {});
    std::string run(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue);
    std::string runAsRoot(const std::string & command, const std::vector<std::string> & options = {});
    std::string runAsRoot(const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue);
    std::string runAsRoot(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options = {});
    std::string runAsRoot(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue );
    std::vector<std::string> split(const std::string & str, char splitChar = '\n');
    virtual std::string retrieveInstallCommand(const Dependency & dependency) = 0;
    virtual std::string computeToolRef ( const Dependency &  dependency);
    fs::path m_systemInstallerPath;
    fs::path m_sudoCmd;
    const CmdOptions & m_options;
    bool m_bundleScripted = false;
};

class SystemTools
{
public:
    SystemTools (const CmdOptions & options) = delete;
    ~SystemTools () = delete;
    static std::string getToolIdentifier (Dependency::Type type = Dependency::Type::SYSTEM);
    static bool isToolSupported (const std::string & tool);
    static std::shared_ptr<BaseSystemTool> createTool (const CmdOptions & options, Dependency::Type dependencyType=Dependency::Type::SYSTEM, bool dontThrowOnMissing = false);
    static std::vector<std::shared_ptr<BaseSystemTool>> retrieveTools (const CmdOptions & options);
    static fs::path getToolPath(const CmdOptions & options, const std::string & installer);
    static std::string run(const fs::path & tool, const std::string & command, const std::vector<std::string> & options = {});
    static std::string run(const fs::path & tool, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue);
    static std::string run(const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options = {});
    static std::string run(const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue);
    static std::string runAsRoot(const fs::path & sudoTool,const fs::path & tool, const std::string & command, const std::vector<std::string> & options = {});
    static std::string runAsRoot(const fs::path & sudoTool,const fs::path & tool, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue);
    static std::string runAsRoot(const fs::path & sudoTool,const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options = {});
    static std::string runAsRoot(const fs::path & sudoTool,const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue);
    static std::string runShellCommand(const std::string & builtinCommand, const std::vector<std::string> & options, const std::string & cmdValue, const std::vector<int> & validResults = {0});
    static std::string runShellCommand(const std::string & builtinCommand, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue, const std::vector<int> & validResults = {0});
};


#endif // SYSTEMTOOLS_H
