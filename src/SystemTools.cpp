#include "SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include <map>

namespace bp = boost::process;

static const std::map<std::string,std::string> options2vcpkg ={{"mac","osx"},
                                                     {"win","windows"},
                                                     {"linux","linux"},
                                                     {"android","android"},
                                                     {"x86_64","x64"},
                                                     {"i386","x86"},
                                                     {"arm","arm"},
                                                     {"arm64","arm64"},
                                                     {"shared","dynamic"},
                                                     {"static","static"},
                                                     {"unix","linux"}};



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

class BrewSystemTool : public BaseSystemTool
{
public:
    BrewSystemTool(const CmdOptions & options):BaseSystemTool(options, "brew") {}
    ~BrewSystemTool() override = default;
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
    m_systemInstallerPath = bp::search_path(installer); //or get it from somewhere else.
    m_sudoCmd = bp::search_path("sudo"); //or get it from somewhere else.
    if (m_systemInstallerPath.empty()) {
        throw std::runtime_error("Error : " + installer + " command not found on the system. Please install it first.");
    }
}


std::string BaseSystemTool::computeSourcePath( const Dependency &  dependency)
{
    return dependency.getPackageName();
}

std::string SystemTools::getToolIdentifier()
{
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

std::shared_ptr<BaseSystemTool> SystemTools::createTool(const CmdOptions & options)
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return nullptr;// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return nullptr;// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    // need to figure out the location of apt, yum ...
    boost::filesystem::path p = bp::search_path("apt-get");
    if (!p.empty()) {
        return std::make_shared<AptSystemTool>(options);
    }

    p = bp::search_path("yum");
    if (!p.empty()) {
        return std::make_shared<YumSystemTool>(options);
    }

    p = bp::search_path("pacman");
    if (!p.empty()) {
        return std::make_shared<PacManSystemTool>(options);
    }

    p = bp::search_path("zypper");
    if (!p.empty()) {
        return std::make_shared<ZypperSystemTool>(options);
    }
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return std::make_shared<PkgUtilSystemTool>(options);
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return std::make_shared<BrewSystemTool>(options);
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return std::make_shared<ChocoSystemTool>(options);
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
    return std::make_shared<PkgToolSystemTool>(options);
#endif
    return nullptr;
}

void ConanSystemTool::update()
{
}

void ConanSystemTool::install(const Dependency & dependency)
{
    std::string source = computeSourcePath(dependency);
    std::string buildType = "build_type=Debug";

    if (m_options.getConfig() == "release") {
        buildType = "build_type=Release";
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;
    if (dependency.getMode() == "na") {
        if (dependency.getBaseRepository().empty()) {
            result = bp::system(m_systemInstallerPath, "install", "-s", buildType.c_str(), "-s", cppStd.c_str(),"--build=missing", source.c_str());
        }
        else {
            result = bp::system(m_systemInstallerPath, "install", "-s", buildType.c_str(), "-s", cppStd.c_str(),"--build=missing", "-r", dependency.getBaseRepository().c_str(), source.c_str());
        }
    }
    else {
        std::string buildMode = "shared=True";
        if (dependency.getMode() == "static") {
            buildMode = "shared=False";
        }

        if (dependency.getBaseRepository().empty()) {
            result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), "-s", buildType.c_str(), "-s", cppStd.c_str(),"--build=missing", source.c_str());
        }
        else {
            result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), "-s", buildType.c_str(), "-s", cppStd.c_str(),"--build=missing", "-r", dependency.getBaseRepository().c_str(), source.c_str());
        }
    }
    if (result != 0) {
        throw std::runtime_error("Error installing conan dependency : " + source);
    }
}

bool ConanSystemTool::installed(const Dependency & dependency)
{
    return false;
}


std::string ConanSystemTool::computeSourcePath( const Dependency &  dependency)
{
    std::string sourceURL = dependency.getPackageName();
    sourceURL += "/" + dependency.getVersion();
    sourceURL += "@" + dependency.getIdentifier();
    sourceURL += "/" + dependency.getChannel();
    return sourceURL;
}


void VCPKGSystemTool::update()
{
}

void VCPKGSystemTool::install(const Dependency & dependency)
{
    std::string source = this->computeSourcePath(dependency);
    int result = bp::system(m_systemInstallerPath, "install", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing vcpkg dependency : " + source);
    }
}

bool VCPKGSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string VCPKGSystemTool::computeSourcePath( const Dependency &  dependency)
{
    std::string mode = dependency.getMode();
    std::string os =  m_options.getOS();
    std::string arch =  m_options.getArchitecture();
    if (options2vcpkg.find(mode) == options2vcpkg.end()) {
        throw std::runtime_error("Error installing vcpkg dependency : unknown mode " + mode);
    }
    if (options2vcpkg.find(os) == options2vcpkg.end()) {
        throw std::runtime_error("Error installing vcpkg dependency : unsupported operating system " + os);
    }
    if (options2vcpkg.find(arch) == options2vcpkg.end()) {
        throw std::runtime_error("Error installing vcpkg dependency : unsupported architecture " + arch);
    }
    os = options2vcpkg.at(os);
    arch = options2vcpkg.at(arch);
    std::string sourceURL = dependency.getPackageName();
    sourceURL += ":" + arch;
    sourceURL += "-" + os;
    return sourceURL;
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
    std::string source = computeSourcePath(dependency);

    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());

    if (result != 0) {
        throw std::runtime_error("Error installing apt dependency : " + source);
    }
}

bool AptSystemTool::installed(const Dependency & dependency)
{
    fs::path dpkg = bp::search_path("dpkg-query");
    std::string source = computeSourcePath(dependency);
    int result = bp::system(dpkg, "-W","-f='${Status}'", source.c_str(),"|","grep -q \"ok installed\"");
    return result == 0;
}



void BrewSystemTool::update()
{
    int result = bp::system(m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating brew repositories");
    }
}

void BrewSystemTool::install(const Dependency & dependency)
{
    if (installed(dependency)) {//TODO : version comparison and checking with range approach
        return;
    }
    std::string source = computeSourcePath(dependency);
    int result = bp::system(m_systemInstallerPath, "install", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing brew dependency : " + source);
    }
}

bool BrewSystemTool::installed(const Dependency & dependency)
{
    std::string source = computeSourcePath(dependency);
    int result = bp::system(m_systemInstallerPath, "ls","--versions", source.c_str());
    return (result == 0);
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
    std::string source = computeSourcePath(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing yum dependency : " + source);
    }
}

bool YumSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeSourcePath(dependency);
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
    std::string source = computeSourcePath(dependency);
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
    std::string source = computeSourcePath(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install", "-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing vcpkg dependency : " + source);
    }
}

bool PkgToolSystemTool::installed(const Dependency & dependency)
{
    std::string source = computeSourcePath(dependency);
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
    std::string source = computeSourcePath(dependency);
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
    std::string source = computeSourcePath(dependency);
    int result = bp::system(m_systemInstallerPath, "install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing choco dependency : " + source);
    }
}

bool ChocoSystemTool::installed(const Dependency & dependency)
{
    return false;
}

void ZypperSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","ref");
    if (result != 0) {
        throw std::runtime_error("Error updating choco repositories");
    }
}

void ZypperSystemTool::install(const Dependency & dependency)
{
    std::string source = computeSourcePath(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","in", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing choco dependency : " + source);
    }
}

bool ZypperSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeSourcePath(dependency);
    int result = bp::system(rpm, "-q",source.c_str());
    return result == 0;
}






