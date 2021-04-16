#include "SystemTools.h"
#include "OsTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <map>

namespace bp = boost::process;
namespace nj = nlohmann;

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

#ifdef BOOST_OS_WINDOWS_AVAILABLE
#define OSSHAREDLIBNODEPATH "bin_paths"
#else
#define OSSHAREDLIBNODEPATH "lib_paths"
#endif

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
    std::string computeToolRef( const Dependency &  dependency) override;
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
    m_systemInstallerPath = bp::search_path(installer); //or get it from somewhere else.
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

std::map<std::string,std::vector<std::string>> supportedTools =
{
    {"linux",{"brew"}},
    {"windows",{"scoop"}},
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

std::shared_ptr<BaseSystemTool> SystemTools::createTool(const CmdOptions & options, std::optional<std::reference_wrapper<const Dependency>> dependencyOpt)
{

    if (dependencyOpt.has_value()) {
        const Dependency & dependency = dependencyOpt.value().get();
        std::string explicitToolName = dependency.getIdentifier();
        if (explicitToolName == "system") {
            // get tool identifier for current OS
            explicitToolName = getToolIdentifier();
        }
        boost::filesystem::path p = bp::search_path(explicitToolName);
        if (!p.empty()) {
#ifdef BOOST_OS_ANDROID_AVAILABLE
            return nullptr;// or conan as default? but implies to set conan options
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
            return nullptr;// or conan as default? but implies to set conan options
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
#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
            if (explicitToolName == "brew") {
                return std::make_shared<BrewSystemTool>(options);
            }
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
            if (explicitToolName == "choco") {
                return std::make_shared<ChocoSystemTool>(options);
            }
            if (explicitToolName == "scoop") {
                return std::make_shared<ZypperSystemTool>(options);
            }
#endif

#ifdef BOOST_OS_BSD_FREE_AVAILABLE
            if (explicitToolName == "pkg") {
                return std::make_shared<PkgToolSystemTool>(options);
            }
#endif
            if (explicitToolName == "vcpkg") {
                return std::make_shared<VCPKGSystemTool>(options);
            }
        }
        throw std::runtime_error("Error: unable to find " + explicitToolName + " tool for dependency [" +dependency.getPackageName() + ":" + dependency.getVersion() +  "]. Please check your configuration and environment.");
    }
    return nullptr;
}


static const std::map<std::string,std::string> conanArchTranslationMap ={{"x86_64", "x86_64"},
                                                                         {"i386", "x86"},
                                                                         {"arm", "armv7"},
                                                                         {"arm64", "armv8"},
                                                                         {"armv6", "armv6"},
                                                                         {"armv7", "armv7"},
                                                                         {"armv7hf", "armv7hf"},
                                                                         {"armv8", "armv8"}
                                                                        };


void ConanSystemTool::update()
{
}

void ConanSystemTool::bundle(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    std::string buildType = "build_type=Debug";
    fs::path destination = m_options.getDestinationRoot();
    destination /= ".conan";

    if (m_options.getConfig() == "release") {
        buildType = "build_type=Release";
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;
    std::vector<std::string> options;
    std::vector<std::string> optionsArgs;
    std::vector<std::string> settingsArgs;
    if (mapContains(conanArchTranslationMap, m_options.getArchitecture())) {
        settingsArgs.push_back("-s");
        settingsArgs.push_back("arch=" + conanArchTranslationMap.at(m_options.getArchitecture()));
    }
    if (dependency.hasOptions()) {
        boost::split(options, dependency.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            optionsArgs.push_back("-o " + option);
        }
    }
    if (dependency.getMode() == "na") {
        result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-if", destination, bp::args(optionsArgs), "-g", "json", source.c_str());
    }
    else {
        std::string buildMode = dependency.getName() + ":";
        if (dependency.getMode() == "static") {
            buildMode += "shared=False";
        }
        else {
            buildMode += "shared=True";
        }
        result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-if", destination, bp::args(optionsArgs), "-g", "json", source.c_str());
    }
    if (result != 0) {
        throw std::runtime_error("Error bundling conan dependency : " + source);
    }
    fs::detail::utf8_codecvt_facet utf8;
    std::string fileName = dependency.getPackageName() + "_conanbuildinfo.json";
    fs::path conanBuildInfoJson = destination/fileName;
    fs::copy_file(destination/"conanbuildinfo.json", conanBuildInfoJson, fs::copy_options::overwrite_existing);
    fs::remove(destination/"conanbuildinfo.json");
    if (fs::exists(conanBuildInfoJson)) {
        std::ifstream ifs1{ conanBuildInfoJson.generic_string(utf8) };
        nj::json conanBuildData = nj::json::parse(ifs1);
        for (auto dep : conanBuildData["dependencies"]) {
            auto root = dep["rootpath"];
            auto lib_paths = dep[OSSHAREDLIBNODEPATH];
            for (auto lib_path : lib_paths) {
                boost::filesystem::path libPath = std::string(lib_path);
                if (boost::filesystem::exists(libPath)) {
                    OsTools::copySharedLibraries(libPath, m_options);
                }
            }
        }
    }
}

void ConanSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    std::string buildType = "build_type=Debug";

    if (m_options.getConfig() == "release") {
        buildType = "build_type=Release";
    }
    std::vector<std::string> options;
    std::vector<std::string> optionsArgs;
    std::vector<std::string> settingsArgs;
    if (mapContains(conanArchTranslationMap, m_options.getArchitecture())) {
        settingsArgs.push_back("-s");
        settingsArgs.push_back("arch=" + conanArchTranslationMap.at(m_options.getArchitecture()));
    }
    if (dependency.hasOptions()) {
        boost::split(options, dependency.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            optionsArgs.push_back("-o " + option);
        }
    }
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        std::string profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
        optionsArgs.push_back("--profile " + profileName);
    }
    else {
        optionsArgs.push_back("--profile " + m_options.getConanProfile());
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;
    if (dependency.getMode() == "na") {
        result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), bp::args(optionsArgs),"--build=missing", source.c_str());
    }
    else {
        std::string buildMode = "shared=True";
        if (dependency.getMode() == "static") {
            buildMode = "shared=False";
        }
        result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), bp::args(optionsArgs),"--build=missing", source.c_str());
    }
    if (result != 0) {
        throw std::runtime_error("Error installing conan dependency : " + source);
    }
}

bool ConanSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string ConanSystemTool::computeToolRef(const Dependency &  dependency)
{
    std::string sourceURL = dependency.getPackageName();
    sourceURL += "/" + dependency.getVersion();
    sourceURL += "@";
    // decorate url for remotes other than conan-center index
    if (dependency.getBaseRepository() != "conan-center") {
        sourceURL +=  dependency.getIdentifier();
        sourceURL += "/" + dependency.getChannel();
    }
    return sourceURL;
}

std::string ConanSystemTool::computeSourcePath(const Dependency &  dependency)
{
    std::string sourceURL = computeToolRef(dependency);
    sourceURL += "|" + m_options.getBuildConfig();
    sourceURL += "|" + dependency.getToolOptions();
    return sourceURL;
}

void VCPKGSystemTool::update()
{
}

void VCPKGSystemTool::install(const Dependency & dependency)
{
    std::string source = this->computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "install", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing vcpkg dependency : " + source);
    }
}

bool VCPKGSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string VCPKGSystemTool::computeToolRef( const Dependency &  dependency)
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
    std::vector<std::string> options;
    std::string optionStr;

    os = options2vcpkg.at(os);
    arch = options2vcpkg.at(arch);
    std::string sourceURL = dependency.getPackageName();

    if (dependency.hasOptions()) {
        sourceURL += "[";
        boost::split(options, dependency.getToolOptions(), [](char c){return c == '#';});
        for (auto option: options) {
            if (!optionStr.empty()) {
                optionStr += ",";
            }
            boost::trim(option);
            optionStr += option;
        }
        sourceURL += optionStr;
        sourceURL += "]";
    }

    sourceURL += ":" + arch;
    sourceURL += "-" + os;
    sourceURL += "-" + mode;
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

std::string BrewSystemTool::computeToolRef(const Dependency &  dependency)
{
    std::string sourceURL = dependency.getPackageName();
    // package@version is not supported for all packages - not reliable !
    // sourceURL += "@" + dependency.getVersion();
    return sourceURL;
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
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "install", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing brew dependency : " + source);
    }
}

bool BrewSystemTool::installed(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
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






