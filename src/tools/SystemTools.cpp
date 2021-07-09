#include "SystemTools.h"
#include "BrewSystemTool.h"
#include "VCPKGSystemTool.h"
#include "ConanSystemTool.h"
#include "PkgConfigTool.h"
#include "utils/OsUtils.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <map>

namespace bp = boost::process;
namespace nj = nlohmann;

class AptSystemTool : public BaseSystemTool
{
public:
    AptSystemTool(const CmdOptions & options):BaseSystemTool(options, "apt-get") {}
    ~AptSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class YumSystemTool : public BaseSystemTool
{
public:
    YumSystemTool(const CmdOptions & options):BaseSystemTool(options, "yum") {}
    ~YumSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class PacManSystemTool : public BaseSystemTool
{
public:
    PacManSystemTool(const CmdOptions & options):BaseSystemTool(options, "pacman") {}
    ~PacManSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class PkgToolSystemTool : public BaseSystemTool
{
public:
    PkgToolSystemTool(const CmdOptions & options):BaseSystemTool(options, "pkg") {}
    ~PkgToolSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class PkgUtilSystemTool : public BaseSystemTool
{
public:
    PkgUtilSystemTool(const CmdOptions & options):BaseSystemTool(options, "pkgutil") {}
    ~PkgUtilSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class ChocoSystemTool : public BaseSystemTool
{
public:
    ChocoSystemTool(const CmdOptions & options):BaseSystemTool(options, "choco") {}
    ~ChocoSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class ScoopSystemTool : public BaseSystemTool
{
public:
    ScoopSystemTool(const CmdOptions & options):BaseSystemTool(options, "scoop") {}
    ~ScoopSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

class ZypperSystemTool : public BaseSystemTool
{
public:
    ZypperSystemTool(const CmdOptions & options):BaseSystemTool(options, "zypper") {}
    ~ZypperSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;


private:
};

BaseSystemTool::BaseSystemTool(const CmdOptions & options, const std::string & installer):m_options(options)
{
    fs::path p = options.getRemakenRoot() / "vcpkg";
    std::vector<fs::path> envPath = boost::this_process::path();
    envPath.push_back(p);
    // TODO must add brew search path here, not /usr/local/bin as it is the brew path on mac only
#ifdef BOOST_OS_MACOS_AVAILABLE
    if (fs::exists("/usr/local/bin")) {
        envPath.push_back("/usr/local/bin");
    }
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    if (fs::exists("/home/linuxbrew/.linuxbrew/bin")) {
        envPath.push_back("/home/linuxbrew/.linuxbrew/bin");
    }
#endif
    m_systemInstallerPath = bp::search_path(installer, envPath); //or get it from somewhere else.
    m_sudoCmd = bp::search_path("sudo"); //or get it from somewhere else.
    if (m_systemInstallerPath.empty()) {
        throw std::runtime_error("Error : " + installer + " command not found on the system. Please install it first.");
    }
}


std::string BaseSystemTool::computeToolRef( const Dependency &  dependency)
{
    return dependency.getPackageName();
}

std::string BaseSystemTool::computeSourcePath( const Dependency &  dependency)
{
    return computeToolRef(dependency);
}

std::vector<fs::path> BaseSystemTool::binPaths([[maybe_unused]] const Dependency & dependency)
{
    return std::vector<fs::path>();
}

std::vector<fs::path> BaseSystemTool::libPaths([[maybe_unused]] const Dependency & dependency)
{
    return std::vector<fs::path>();
}

fs::path BaseSystemTool::invokeGenerator([[maybe_unused]] const std::vector<Dependency> & deps, [[maybe_unused]] GeneratorType generator)
{
    PkgConfigTool pkgConfig(m_options);
    // call pkg-config on dep and populate libs and cflags variables
    // TODO:check pkg exists from pkgconfig ?
    std::vector<std::string> cflags, libs;
    for ( auto & dep : deps) {
        pkgConfig.cflags(dep.getName(),cflags);
        pkgConfig.libs(dep.getName(),libs);
    }

    // format CFLAGS and LIBS results
    return pkgConfig.generate(generator,cflags,libs,Dependency::Type::SYSTEM);
}

const std::map<Dependency::Type,std::string> typeToStringMap =
{
    {Dependency::Type::BREW,"brew"},
    {Dependency::Type::CHOCO,"choco"},
    {Dependency::Type::CONAN,"conan"},
    {Dependency::Type::SCOOP,"scoop"},
    {Dependency::Type::VCPKG,"vcpkg"}
};

std::string SystemTools::getToolIdentifier(Dependency::Type type)
{
    if (type != Dependency::Type::SYSTEM) {
        if (!mapContains(typeToStringMap, type)) {
            throw std::runtime_error("Error: unable to find tool identifier for type " + std::to_string(static_cast<uint32_t>(type)));
        }
        return typeToStringMap.at(type);
    }
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return string();// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return string();// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    // need to figure out the location of apt, yum ...
    std::string tool = "apt-get";
    boost::filesystem::path p = bp::search_path(tool);
    if (!p.empty()) {
        return tool;
    }
    tool = "yum";
    p = bp::search_path(tool);
    if (!p.empty()) {
        return tool;
    }

    tool = "pacman";
    p = bp::search_path(tool);
    if (!p.empty()) {
        return tool;
    }

    tool = "zypper";
    p = bp::search_path(tool);
    if (!p.empty()) {
        return tool;
    }
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "pkgutil";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "brew";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "choco";
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
    return "pkg";
#endif
    return "";
}

const std::map<std::string,std::vector<std::string>> supportedTools =
{
    {"linux",{"brew","vcpkg"}},
    {"mac",{"vcpkg"}},
    {"windows",{"scoop","vcpkg"}},
};

bool SystemTools::isToolSupported(const std::string & tool)
{
    const std::string & systemTool = getToolIdentifier();
    if (tool == systemTool) {
        return true;
    }

#ifdef BOOST_OS_LINUX_AVAILABLE
    for (auto & supportedTool : supportedTools.at("linux")) {
        if (supportedTool == tool) {
            return true;
        }
    }
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "pkgutil";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    for (auto & supportedTool : supportedTools.at("mac")) {
        if (supportedTool == tool) {
            return true;
        }
    }
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    for (auto & supportedTool : supportedTools.at("windows")) {
        if (supportedTool == tool) {
            return true;
        }
    }
#endif
    return false;
}

std::shared_ptr<BaseSystemTool> SystemTools::createTool(const CmdOptions & options, Dependency::Type dependencyType)
{
    fs::path p = options.getRemakenRoot() / "vcpkg";
    std::vector<fs::path> envPath = boost::this_process::path();
    envPath.push_back(p);
#ifdef BOOST_OS_MACOS_AVAILABLE
    if (fs::exists("/usr/local/bin")) {
        envPath.push_back("/usr/local/bin");
    }
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    if (fs::exists("/home/linuxbrew/.linuxbrew/bin")) {
        envPath.push_back("/home/linuxbrew/.linuxbrew/bin");
    }
#endif
    std::string explicitToolName = getToolIdentifier(dependencyType);
    p = bp::search_path(explicitToolName, envPath);
    if (!p.empty()) {
#ifdef BOOST_OS_ANDROID_AVAILABLE
        return nullptr;// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
        return nullptr;// or conan as default? but implies to set conan options
#endif
#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
        if (dependencyType == Dependency::Type::BREW) {
            return std::make_shared<BrewSystemTool>(options);
        }
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
        if (explicitToolName == "apt-get") {
            return std::make_shared<AptSystemTool>(options);
        }
        if (explicitToolName == "yum") {
            return std::make_shared<YumSystemTool>(options);
        }
        if (explicitToolName == "pacman") {
            return std::make_shared<PacManSystemTool>(options);
        }
        if (explicitToolName == "zypper") {
            return std::make_shared<ZypperSystemTool>(options);
        }
        if (explicitToolName == "pkgutil") {
            return std::make_shared<PkgUtilSystemTool>(options);
        }
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
        if (dependencyType == Dependency::Type::SCOOP) {
            return std::make_shared<ScoopSystemTool>(options);
        }
        if ((dependencyType == Dependency::Type::CHOCO)
            || (dependencyType == Dependency::Type::SYSTEM)) {
            return std::make_shared<ChocoSystemTool>(options);
        }
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
        if (explicitToolName == "pkg") {
            return std::make_shared<PkgToolSystemTool>(options);
        }
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
        if (explicitToolName == "pkgutil") {
            return std::make_shared<PkgUtilSystemTool>(options);
        }
#endif
        if (dependencyType == Dependency::Type::VCPKG) {
            return std::make_shared<VCPKGSystemTool>(options);
        }
        if (dependencyType == Dependency::Type::CONAN) {
            return std::make_shared<ConanSystemTool>(options);
        }
    }
    throw std::runtime_error("Error: unable to find " + explicitToolName + " tool. Please check your configuration and environment.");
}

void AptSystemTool::update()
{
    int result = bp::system(sudo(),m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating apt repositories");
    }
}

void AptSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);

    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());

    if (result != 0) {
        throw std::runtime_error("Error installing apt dependency : " + source);
    }
}

bool AptSystemTool::installed(const Dependency & dependency)
{
    fs::path dpkg = bp::search_path("dpkg-query");
    std::string source = computeToolRef(dependency);
    int result = bp::system(dpkg, "-W","-f='${Status}'", source.c_str(),"|","grep -q \"ok installed\"");
    return result == 0;
}

void YumSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "update","-y");
    if (result != 0) {
        throw std::runtime_error("Error updating yum repositories");
    }
}

void YumSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing yum dependency : " + source);
    }
}

bool YumSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeToolRef(dependency);
    int result = bp::system(rpm, "-q",source.c_str());
    return result == 0;
}



void PacManSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "-Syyu","--noconfirm");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PacManSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "-S","--noconfirm", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing pacman dependency : " + source);
    }
}

bool PacManSystemTool::installed(const Dependency & dependency)
{
    return false;
}

void PkgToolSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PkgToolSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install", "-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing vcpkg dependency : " + source);
    }
}

bool PkgToolSystemTool::installed(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "info", source.c_str());
    return result == 0;
}


void PkgUtilSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "--catalog");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PkgUtilSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "--install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing pacman dependency : " + source);
    }
}

bool PkgUtilSystemTool::installed(const Dependency & dependency)
{
    return false;
}


void ChocoSystemTool::update()
{
    int result = bp::system(m_systemInstallerPath, "outdated");
    if (result != 0) {
        throw std::runtime_error("Error updating choco repositories");
    }
}

void ChocoSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing choco dependency : " + source);
    }
}

bool ChocoSystemTool::installed(const Dependency & dependency)
{
    return false;
}

void ScoopSystemTool::update()
{
    int result = bp::system(m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating scoop repositories");
    }
}

void ScoopSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing scoop dependency : " + source);
    }
}

bool ScoopSystemTool::installed(const Dependency & dependency)
{
    return false;
}

void ZypperSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","ref");
    if (result != 0) {
        throw std::runtime_error("Error updating zypper repositories");
    }
}

void ZypperSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","in", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing zypper dependency : " + source);
    }
}

bool ZypperSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeToolRef(dependency);
    int result = bp::system(rpm, "-q",source.c_str());
    return result == 0;
}






