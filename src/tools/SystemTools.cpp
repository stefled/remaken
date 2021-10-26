#include "NativeSystemTools.h"
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

namespace bp = boost::process;
namespace nj = nlohmann;



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
#ifdef BOOST_OS_LINUX_AVAILABLE
    if (fs::exists("/usr/lib/x86_64-linux-gnu/pkgconfig/")) {
        m_options.verboseMessage("==> Adding pkgconfig path: '/usr/lib/x86_64-linux-gnu/pkgconfig/'");
        pkgConfig.addPath("/usr/lib/x86_64-linux-gnu/pkgconfig/");
    }
#endif
    std::vector<std::string> cflags, libs;
    for ( auto & dep : deps) {
        pkgConfig.cflags(dep.getName(),cflags);
        pkgConfig.libs(dep.getName(),libs);
    }

    // format CFLAGS and LIBS results
    return pkgConfig.generate(generator,cflags,libs,Dependency::Type::SYSTEM);
}

std::vector<std::string> BaseSystemTool::split(const std::string & str, char splitChar)
{
    std::vector<std::string> outVect;
    boost::split(outVect, str, [&](char c){return c == splitChar;});
    if (outVect.size() == 1) {
        if (outVect.at(0).empty()) {
            outVect.clear();
        }
    }
    else {
        outVect.erase(std::remove_if(outVect.begin(), outVect.end(),[](std::string s) { return s.empty(); }), outVect.end());
    }
    return outVect;
}

std::string BaseSystemTool::run(const std::string & command, const std::string & cmdValue, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(m_systemInstallerPath, command, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else {
        result = bp::system(m_systemInstallerPath, command, bp::args(options),  cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (result != 0) {
        throw std::runtime_error("Error running " + m_systemInstallerPath.generic_string(utf8) +" command '" + command + "' for '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}


std::string BaseSystemTool::run(const std::string & command, const std::vector<std::string> & options)
{
    return run(command,"",options);
}

std::string SystemTools::getToolIdentifier(Dependency::Type type)
{
    if (type != Dependency::Type::SYSTEM) {
        return to_string(type);
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
#if defined(BOOST_OS_MACOS_AVAILABLE)
        if ((dependencyType == Dependency::Type::BREW) ||
            (dependencyType == Dependency::Type::SYSTEM)) {
            return std::make_shared<BrewSystemTool>(options);
        }
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
        if (dependencyType == Dependency::Type::BREW) {
            return std::make_shared<BrewSystemTool>(options);
        }
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

std::vector<std::shared_ptr<BaseSystemTool>> SystemTools::retrieveTools (const CmdOptions & options)
{
    std::vector<std::shared_ptr<BaseSystemTool>> toolList;
    std::shared_ptr<BaseSystemTool> tool = createTool(options,Dependency::Type::SYSTEM);
    if (tool) {
        toolList.push_back(tool);
    }
    tool = createTool(options,Dependency::Type::CONAN);
    if (tool) {
        toolList.push_back(tool);
    }
    tool = createTool(options,Dependency::Type::VCPKG);
    if (tool) {
        toolList.push_back(tool);
    }
#if defined(BOOST_OS_LINUX_AVAILABLE)
    tool = createTool(options,Dependency::Type::BREW);
    if (tool) {
        toolList.push_back(tool);
    }
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    tool = createTool(options,Dependency::Type::CHOCO);
    if (tool) {
        toolList.push_back(tool);
    }
    tool = createTool(options,Dependency::Type::SCOOP);
    if (tool) {
        toolList.push_back(tool);
    }
#endif
    return toolList;
}







