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

#ifndef NATIVESYSTEMTOOLS_H
#define NATIVESYSTEMTOOLS_H

#include "tools/SystemTools.h"

class NativeSystemTool : public BaseSystemTool
{
public:
    NativeSystemTool(const CmdOptions & options, const std::string & installer):BaseSystemTool(options, installer) {}
    virtual ~NativeSystemTool() = default;
    virtual void bundle ([[maybe_unused]] const Dependency & dependency) override {
        BOOST_LOG_TRIVIAL(warning)<<"bundle() not implemented yet for tool "<<m_systemInstallerPath;
    }
    virtual void search (const std::string & pkgName, const std::string & version) override {
        BOOST_LOG_TRIVIAL(warning)<<"search() not implemented yet for tool "<<m_systemInstallerPath;
    }
    virtual void bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile) override;

protected:
    virtual std::string retrieveInstallCommand(const Dependency & dependency) override = 0;
};

class AptSystemTool : public NativeSystemTool
{
public:
    AptSystemTool(const CmdOptions & options):NativeSystemTool(options, "apt-get") {}
    ~AptSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class YumSystemTool : public NativeSystemTool
{
public:
    YumSystemTool(const CmdOptions & options):NativeSystemTool(options, "yum") {}
    ~YumSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class PacManSystemTool : public NativeSystemTool
{
public:
    PacManSystemTool(const CmdOptions & options):NativeSystemTool(options, "pacman") {}
    ~PacManSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class PkgToolSystemTool : public NativeSystemTool
{
public:
    PkgToolSystemTool(const CmdOptions & options):NativeSystemTool(options, "pkg") {}
    ~PkgToolSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class PkgUtilSystemTool : public NativeSystemTool
{
public:
    PkgUtilSystemTool(const CmdOptions & options):NativeSystemTool(options, "pkgutil") {}
    ~PkgUtilSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class ChocoSystemTool : public NativeSystemTool
{
public:
    ChocoSystemTool(const CmdOptions & options):NativeSystemTool(options, "choco") {}
    ~ChocoSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class ScoopSystemTool : public NativeSystemTool
{
public:
    ScoopSystemTool(const CmdOptions & options):NativeSystemTool(options, "scoop") {}
    ~ScoopSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

class ZypperSystemTool : public NativeSystemTool
{
public:
    ZypperSystemTool(const CmdOptions & options):NativeSystemTool(options, "zypper") {}
    ~ZypperSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;

private:
    std::string retrieveInstallCommand(const Dependency & dependency) override;
};

#endif // NATIVESYSTEMTOOLS_H
