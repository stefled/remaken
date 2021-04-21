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

std::vector<std::string> VCPKGSystemTool::binPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}

std::vector<std::string> VCPKGSystemTool::libPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}

