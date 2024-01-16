#include "NativeSystemTools.h"
#include "BrewSystemTool.h"
#include "VCPKGSystemTool.h"
#include "ConanSystemTool.h"
#include "PkgConfigTool.h"
#include "utils/OsUtils.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace bp = boost::process;
namespace nj = nlohmann;



BaseSystemTool::BaseSystemTool(const CmdOptions & options, const std::string & installer):m_options(options)
{
    m_systemInstallerPath = SystemTools::getToolPath(options, installer);

#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
    m_sudoCmd = bp::search_path("sudo"); //or get it from somewhere else.
    if (m_sudoCmd.empty()) {
        throw std::runtime_error("Error : sudo command not found on the system. Please check your environment first.");
    }
#endif
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

std::vector<fs::path> BaseSystemTool::includePaths([[maybe_unused]] const Dependency & dependency)
{
    return std::vector<fs::path>();
}

std::pair<std::string, fs::path> BaseSystemTool::invokeGenerator([[maybe_unused]] std::vector<Dependency> & deps)
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
        pkgConfig.cflags(dep,cflags);
        pkgConfig.libs(dep,libs);
    }

    // format CFLAGS and LIBS results
    return pkgConfig.generate(deps,Dependency::Type::SYSTEM);
}

void BaseSystemTool::write_pkg_file([[maybe_unused]] std::vector<Dependency> & deps)
{
    // TODO
}

std::vector<std::string> BaseSystemTool::split(const std::string & str, char splitChar)
{
    std::vector<std::string> outVect;
    std::string strNoCarriageReturn = str;

    boost::erase_all(strNoCarriageReturn, "\r");
    boost::split(outVect, strNoCarriageReturn, [&](char c) {return c == splitChar;});
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

std::string BaseSystemTool::run(const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue)
{
    return SystemTools::run(m_systemInstallerPath, command, options, cmdValue);
}

std::string BaseSystemTool::run(const std::string & command, const std::vector<std::string> & options)
{
    return SystemTools::run(m_systemInstallerPath, command, options);
}

std::string BaseSystemTool::run(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue)
{
    return SystemTools::run(m_systemInstallerPath, command, subCommand, options, cmdValue);
}

std::string BaseSystemTool::run(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options)
{
    return SystemTools::run(m_systemInstallerPath, command, subCommand, options);
}

std::string BaseSystemTool::runAsRoot(const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue)
{
    return SystemTools::runAsRoot(m_sudoCmd, m_systemInstallerPath, command, options, cmdValue);
}

std::string BaseSystemTool::runAsRoot(const std::string & command, const std::vector<std::string> & options)
{
    return SystemTools::runAsRoot(m_sudoCmd, m_systemInstallerPath, command, options);
}

std::string BaseSystemTool::runAsRoot(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue)
{
    return SystemTools::runAsRoot(m_sudoCmd, m_systemInstallerPath, command, subCommand, options, cmdValue);
}

std::string BaseSystemTool::runAsRoot(const std::string & command, const std::string & subCommand, const std::vector<std::string> & options)
{
    return SystemTools::runAsRoot(m_sudoCmd, m_systemInstallerPath, command, subCommand, options);
}

fs::path SystemTools::getToolPath(const CmdOptions & options, const std::string & installer)
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
    fs::path systemInstallerPath = bp::search_path(installer, envPath); //or get it from somewhere else.
    if (systemInstallerPath.empty()) {
        throw std::runtime_error("Error : " + installer + " command not found on the system. Please install it first.");
    }
    return systemInstallerPath;
}

std::string SystemTools::run(const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(tool, command, subCommand, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else{
        result = bp::system(tool, command, subCommand, bp::args(options), cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (result != 0) {
        throw std::runtime_error("Error running " + tool.generic_string(utf8) +" command '" + command + " sub-command '" + subCommand + "' with value '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}

std::string SystemTools::run(const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options)
{
    return run(tool, command, subCommand, options,"");
}

std::string SystemTools::run(const fs::path & tool, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(tool, command, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else{
        result = bp::system(tool, command, bp::args(options), cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (result != 0) {
        throw std::runtime_error("Error running " + tool.generic_string(utf8) +" command '" + command + "' with value '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}

std::string SystemTools::run(const fs::path & tool, const std::string & command, const std::vector<std::string> & options)
{
    return run(tool, command, options, "");
}

std::string SystemTools::runAsRoot(const fs::path & sudoTool, const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options, const std::string & cmdValue)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(sudoTool, tool, command, subCommand, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else {
        result = bp::system(sudoTool, tool, command, subCommand, bp::args(options),  cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (result != 0) {
        throw std::runtime_error("Error running "  + sudoTool.generic_string(utf8) + " "+ tool.generic_string(utf8) +" command '" + command  + " sub-command '" + subCommand + "' with value '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}


std::string SystemTools::runAsRoot(const fs::path & sudoTool, const fs::path & tool, const std::string & command, const std::string & subCommand, const std::vector<std::string> & options)
{
    return runAsRoot(sudoTool, tool, command, subCommand, options, "");
}

std::string SystemTools::runAsRoot(const fs::path & sudoTool, const fs::path & tool, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(sudoTool, tool, command, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else {
        result = bp::system(sudoTool, tool, command, bp::args(options),  cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (result != 0) {
        throw std::runtime_error("Error running "  + sudoTool.generic_string(utf8) + " "+ tool.generic_string(utf8) +" command '" + command + "' for '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}


std::string SystemTools::runAsRoot(const fs::path & sudoTool, const fs::path & tool, const std::string & command, const std::vector<std::string> & options)
{
    return runAsRoot(sudoTool, tool, command, "", options);
}

std::string SystemTools::runShellCommand(const std::string & builtinCommand, const std::vector<std::string> & options, const std::string & cmdValue, const std::vector<int> & validResults)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    fs::path builtinPath = bp::search_path(builtinCommand);
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(builtinPath, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else{
        result = bp::system(builtinPath, bp::args(options), cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (std::find(std::begin(validResults), std::end(validResults), result) == std::end(validResults)) {
        throw std::runtime_error("Error running " + builtinPath.generic_string(utf8) + "' with value '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
}

std::string SystemTools::runShellCommand(const std::string & builtinCommand, const std::string & command, const std::vector<std::string> & options, const std::string & cmdValue, const std::vector<int> & validResults)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    fs::path builtinPath = bp::search_path(builtinCommand);
    int result = -1;
    if (cmdValue.empty()) {
        result = bp::system(builtinPath, command, bp::args(options), bp::std_out > listOutputFut, ios);
    }
    else{
        result = bp::system(builtinPath, command, bp::args(options), cmdValue, bp::std_out > listOutputFut, ios);
    }
    if (std::find(std::begin(validResults), std::end(validResults), result) == std::end(validResults)) {
        throw std::runtime_error("Error running " + builtinPath.generic_string(utf8) +" command '" + command + "' with value '" + cmdValue + "'");
    }
    auto resultStringList = listOutputFut.get();
    return resultStringList;
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

std::shared_ptr<BaseSystemTool> SystemTools::createTool(const CmdOptions & options, Dependency::Type dependencyType, bool dontThrowOnMissing)
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
    if (dontThrowOnMissing) {
        return nullptr;
    }
    throw std::runtime_error("Error: unable to find " + explicitToolName + " tool. Please check your configuration and environment.");
}

std::vector<std::shared_ptr<BaseSystemTool>> SystemTools::retrieveTools (const CmdOptions & options)
{
    std::vector<std::shared_ptr<BaseSystemTool>> toolList;
    std::shared_ptr<BaseSystemTool> tool;
    tool = createTool(options, Dependency::Type::SYSTEM, true);
    if (tool) {
        toolList.push_back(tool);
    }
    tool = createTool(options, Dependency::Type::CONAN, true);
    if (tool) {
        toolList.push_back(tool);
    }
    tool = createTool(options, Dependency::Type::VCPKG, true);
    if (tool) {
        toolList.push_back(tool);
    }
#if defined(BOOST_OS_LINUX_AVAILABLE)
    tool = createTool(options, Dependency::Type::BREW, true);
    if (tool) {
        toolList.push_back(tool);
    }
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    tool = createTool(options,Dependency::Type::SCOOP, true);
    if (tool) {
        toolList.push_back(tool);
    }
#endif
    return toolList;
}







