#include "VCPKGSystemTool.h"
#include "OsTools.h"

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

static const std::map<BaseSystemTool::PathType,std::string> vcpkgPathNodeMap ={{BaseSystemTool::PathType::BIN_PATHS, "bin"},
                                                                             #ifdef BOOST_OS_WINDOWS_AVAILABLE
                                                                             {BaseSystemTool::PathType::LIB_PATHS, "bin"}
                                                                             #else
                                                                             {BaseSystemTool::PathType::LIB_PATHS, "lib"}
                                                                             #endif
                                                                            };

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

fs::path VCPKGSystemTool::computeLocalDependencyRootDir( const Dependency &  dependency)
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
    mode = options2vcpkg.at(mode);
    fs::path baseVcpkgPath = m_systemInstallerPath.parent_path();
    baseVcpkgPath /= "packages";
    std::string sourceURL = dependency.getPackageName();

    sourceURL += "_" + arch;
    sourceURL += "-" + os;
    sourceURL += "-" + mode;
    return baseVcpkgPath/sourceURL;
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
    mode = options2vcpkg.at(mode);
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

std::vector<fs::path> VCPKGSystemTool::retrievePaths(const Dependency & dependency, BaseSystemTool::PathType pathType)
{
    std::vector<fs::path> paths;
    fs::detail::utf8_codecvt_facet utf8;
    fs::path depPath = computeLocalDependencyRootDir(dependency);
    depPath /= vcpkgPathNodeMap.at(pathType);
    if (!fs::exists(depPath)) {
        BOOST_LOG_TRIVIAL(warning)<<"VCPKG path doesn't exist : "<<depPath;
        return paths;
    }
    paths.push_back(depPath);
    return paths;
}

std::vector<fs::path> VCPKGSystemTool::binPaths(const Dependency & dependency)
{
    std::vector<fs::path> binPaths = retrievePaths(dependency, BaseSystemTool::PathType::BIN_PATHS);
    return binPaths;
}

std::vector<fs::path> VCPKGSystemTool::libPaths(const Dependency & dependency)
{
    std::vector<fs::path> libPaths = retrievePaths(dependency, BaseSystemTool::PathType::LIB_PATHS);
    return libPaths;
}

