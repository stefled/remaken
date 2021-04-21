#include "BrewSystemTool.h"
#include "OsTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>

namespace bp = boost::process;


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

std::vector<std::string> BrewSystemTool::binPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}

std::vector<std::string> BrewSystemTool::libPaths(const Dependency & dependency)
{
    return std::vector<std::string>();
}
