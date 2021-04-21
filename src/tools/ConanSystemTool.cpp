#include "ConanSystemTool.h"
#include "OsTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <map>

namespace bp = boost::process;
namespace nj = nlohmann;

static const std::map<std::string,std::string> conanArchTranslationMap ={{"x86_64", "x86_64"},
                                                                         {"i386", "x86"},
                                                                         {"arm", "armv7"},
                                                                         {"arm64", "armv8"},
                                                                         {"armv6", "armv6"},
                                                                         {"armv7", "armv7"},
                                                                         {"armv7hf", "armv7hf"},
                                                                         {"armv8", "armv8"}
                                                                        };


#ifdef BOOST_OS_WINDOWS_AVAILABLE
#define OSSHAREDLIBNODEPATH "bin_paths"
#else
#define OSSHAREDLIBNODEPATH "lib_paths"
#endif

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
    std::string profileName = m_options.getConanProfile();
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        std::string profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;
    if (dependency.getMode() == "na") {
        result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), bp::args(optionsArgs),"--build=missing", source.c_str());
    }
    else {
        std::string buildMode = "shared=True";
        if (dependency.getMode() == "static") {
            buildMode = "shared=False";
        }
        result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), bp::args(optionsArgs),"--build=missing", source.c_str());
    }
    if (result != 0) {
        throw std::runtime_error("Error installing conan dependency : " + source);
    }
}

void ConanSystemTool::invokeGenerator(const fs::path & conanFilePath, ConanSystemTool::GeneratorType generator)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path targetPath = m_options.getProjectRootPath() / "build" / m_options.getConfig();
    std::string buildType = "build_type=Debug";

    if (m_options.getConfig() == "release") {
        buildType = "build_type=Release";
    }
    std::vector<std::string> settingsArgs;
    if (mapContains(conanArchTranslationMap, m_options.getArchitecture())) {
        settingsArgs.push_back("-s");
        settingsArgs.push_back("arch=" + conanArchTranslationMap.at(m_options.getArchitecture()));
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();

    int result = -1;
    result = bp::system(m_systemInstallerPath, "install", conanFilePath.generic_string(utf8).c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-if",  targetPath.generic_string(utf8).c_str());
    if (result != 0) {
        throw std::runtime_error("Error calling conan generator : ");
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

std::vector<std::string> ConanSystemTool::binPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}

std::vector<std::string> ConanSystemTool::libPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}

