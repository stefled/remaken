#include "BrewSystemTool.h"
#include "utils/OsTools.h"

#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/predef.h>
#include <string>

namespace bp = boost::process;


std::string BrewSystemTool::computeToolRef (const Dependency &  dependency)
{
    std::string sourceURL = dependency.getPackageName();
    // package@version is not supported for all packages - not reliable !
    // sourceURL += "@" + dependency.getVersion();
    return sourceURL;
}

void BrewSystemTool::update ()
{
    int result = bp::system (m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating brew repositories");
    }
}

void BrewSystemTool::bundleLib(const std::string & libPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = bp::system(m_systemInstallerPath, "list", libPath.c_str(), bp::std_out > listOutputFut, ios);
    auto libsString = listOutputFut.get();
    std::vector<std::string> libsPath;
    boost::split(libsPath, libsString, [](char c){return c == '\n';});
    for (auto & lib : libsPath) {
        fs::path libPath (lib, utf8);
        if ( libPath.extension().generic_string((utf8)) == OsTools::sharedSuffix(m_options.getOS())) {
            std::cout<<lib<<std::endl;
        }
    }
}

void BrewSystemTool::bundle (const Dependency & dependency)
{
    std::string source = computeToolRef (dependency);
    bundleLib(source);
    boost::asio::io_context ios;
    std::future<std::string> depsOutputFut;
    int result = bp::system(m_systemInstallerPath, "deps", source.c_str(), bp::std_out > depsOutputFut, ios);
    auto depsString  = depsOutputFut.get();
    std::vector<std::string> deps;
    boost::split(deps, depsString, [](char c){return c == '\n';});
    for (auto & dep : deps) {
        if (!dep.empty()) {
            std::cout<<dep<<std::endl;
            bundleLib(dep);
        }
    }
}

void BrewSystemTool::install (const Dependency & dependency)
{
    if (installed (dependency)) {//TODO : version comparison and checking with range approach
        return;
    }
    std::string source = computeToolRef (dependency);
    int result = bp::system(m_systemInstallerPath, "install", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing brew dependency : " + source);
    }
}

bool BrewSystemTool::installed (const Dependency & dependency)
{
    std::string source = computeToolRef (dependency);
    int result = bp::system(m_systemInstallerPath, "ls","--versions", source.c_str());
    return (result == 0);
}

std::vector<fs::path> BrewSystemTool::binPaths ([[maybe_unused]] const Dependency & dependency)
{
    std::vector<fs::path> paths;
    fs::detail::utf8_codecvt_facet utf8;
    fs::path baseBrewPath = m_systemInstallerPath.parent_path().parent_path();
    baseBrewPath /= "bin";
    paths.push_back(baseBrewPath);
    return std::vector<fs::path>();
}

std::vector<fs::path> BrewSystemTool::libPaths ([[maybe_unused]] const Dependency & dependency)
{
    std::vector<fs::path> paths;
    fs::detail::utf8_codecvt_facet utf8;
    fs::path baseBrewPath = m_systemInstallerPath.parent_path().parent_path();
    baseBrewPath /= "lib";
    paths.push_back(baseBrewPath);
    return paths;
}
