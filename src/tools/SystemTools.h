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
#include <boost/log/trivial.hpp>

class BaseSystemTool
{
public:
    BaseSystemTool(const CmdOptions & options, const std::string & installer);
    virtual ~BaseSystemTool() = default;
    virtual void update() = 0;
    virtual void bundle(const Dependency & dependency) {
        BOOST_LOG_TRIVIAL(warning)<<"bundle() not implemented yet for tool "<<m_systemInstallerPath;
    }
    virtual void install(const Dependency & dependency) = 0;
    virtual bool installed(const Dependency & dependency) = 0;
    virtual std::string computeSourcePath( const Dependency &  dependency);
    virtual fs::path sudo() { return m_sudoCmd; }


protected:
    virtual std::string computeToolRef( const Dependency &  dependency);
    fs::path m_systemInstallerPath;
    fs::path m_sudoCmd;
    const CmdOptions & m_options;
};

class SystemTools
{
public:
    SystemTools(const CmdOptions & options) = delete;
    ~SystemTools() = delete;
    static std::string getToolIdentifier();
    static bool isToolSupported(const std::string & tool);
    static std::shared_ptr<BaseSystemTool> createTool(const CmdOptions & options, std::optional<std::reference_wrapper<const Dependency>> dependencyOpt=std::nullopt);
};

class VCPKGSystemTool : public BaseSystemTool
{
public:
    fs::detail::utf8_codecvt_facet utf8;
    VCPKGSystemTool(const CmdOptions & options):BaseSystemTool(options, options.getRemakenRoot().generic_string(utf8) + "/vcpkg/vcpkg") {}
    ~VCPKGSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

protected:
    std::string computeToolRef( const Dependency &  dependency) override;
};



#endif // SYSTEMTOOLS_H
