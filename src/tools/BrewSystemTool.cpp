#include "BrewSystemTool.h"
#include "utils/OsUtils.h"
#include "tools/PkgConfigTool.h"
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/predef.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace nj = nlohmann;

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

std::vector<std::string> BrewSystemTool::split(const std::string & str, char splitChar)
{
    std::vector<std::string> outVect;
    boost::split(outVect, str, [&](char c){return c == splitChar;});
    outVect.erase(std::remove_if(outVect.begin(), outVect.end(),[](std::string s) { return s.empty(); }));
    return outVect;
}


std::string BrewSystemTool::run(const std::string & command, const std::string & depName, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    int result = bp::system(m_systemInstallerPath, command, bp::args(options),  depName, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running brew command '" + command + "' for '" + depName + "'");
    }
    auto libsString = listOutputFut.get();
    std::vector<std::string> libsPath;
    boost::split(libsPath, libsString, [](char c){return c == '\n';});
    return  libsString;
}

void BrewSystemTool::bundleLib(const std::string & libPath)
{
    std::vector<std::string> libsPath = split( run ("list",libPath) );
    for (auto & lib : libsPath) {
        fs::detail::utf8_codecvt_facet utf8;
        fs::path libPath (lib, utf8);
        if ( libPath.extension().generic_string((utf8)) == OsUtils::sharedSuffix(m_options.getOS())) {
            std::cout<<lib<<std::endl;
        }
    }
}

void BrewSystemTool::bundle (const Dependency & dependency)
{
    std::string source = computeToolRef (dependency);
    bundleLib(source);
    std::vector<std::string> deps = split( run ("deps", source) );
    for (auto & dep : deps) {
        std::cout<<dep<<std::endl;
        bundleLib(dep);
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

std::string BrewSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = m_systemInstallerPath.generic_string(utf8);
    installCmd += " install " + source;
    return installCmd;
}

fs::path BrewSystemTool::invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator)
{
    static const std::map<std::string,std::string> brewRootPathMap = {
        {"mac","/usr/local"},
        {"linux","/home/linuxbrew/.linuxbrew"}
    };

    if (!mapContains(brewRootPathMap, m_options.getOS())) {
        throw std::runtime_error("Error: brew not supported for OS : " + m_options.getOS());
    }

    fs::detail::utf8_codecvt_facet utf8;
    fs::path globalBrewPkgConfigPath(brewRootPathMap.at(m_options.getOS()), utf8);
    globalBrewPkgConfigPath /= "lib";
    globalBrewPkgConfigPath /= "pkgconfig";

    // add to PKG_CONFIG_PATH default pkg-config path for brew
    PkgConfigTool pkgConfig(m_options);
    pkgConfig.addPath(globalBrewPkgConfigPath);
    for ( auto & dep : deps) {
        std::cout<<"==> Adding '"<<dep.getName()<<":"<<dep.getVersion()<<"' dependency"<<std::endl;
        // retrieve brew sub-dependencies for 'dep'
        std::vector<std::string> depsList = split( run ("deps", dep.getPackageName()) );
        // add base dependency to check for keg-only
        depsList.push_back(dep.getPackageName());
        // search for keg-only formulae
        for (auto & subDep : depsList) {
            m_options.verboseMessage("===> Analysing sub dependency: " + subDep);
            std::string jsonInfos = run ("info", subDep, {"--json=v1"});
            nj::json brewJsonInfos = nj::json::parse(jsonInfos);
            //todo : ignore keg-only on linux and search keg-only
            if (!brewJsonInfos.is_array()) {
                throw std::runtime_error("Error: expecting a json array but brew json info is not an array");
            }
            if (brewJsonInfos.empty()) {
                throw std::runtime_error("Error: brew json info array is empty");
            }
            if (brewJsonInfos[0].contains("keg_only")) {
                if (brewJsonInfos[0]["keg_only"].get<bool>() == true) {
                    // found keg-only formulae
                    m_options.verboseMessage("   |==> " + subDep + " is 'keg-only': parsing files ...");
                    std::vector<std::string> filesList = split( run ("list", subDep) );
                    fs::path localPkgConfigPath;
                    for (auto & file : filesList) {
                        if (file.find("pkgconfig") != std::string::npos) { // found pkgconfig path for keg-only
                            localPkgConfigPath = file;
                        }
                    }
                    if (!localPkgConfigPath.empty()) { // add found pkgconfig to pkgConfigPath variable
                        m_options.verboseMessage("   |==> Adding keg-only pkgconfig path: " + localPkgConfigPath.parent_path().generic_string(utf8));
                        pkgConfig.addPath(localPkgConfigPath.parent_path());
                    }
                }
            }
        }
    }

    // call pkg-config on dep and populate libs and cflags variables
    std::vector<std::string> cflags, libs;
    for ( auto & dep : deps) {
        pkgConfig.cflags(dep.getName(),cflags);
        pkgConfig.libs(dep.getName(),libs);
    }

    return pkgConfig.generate(generator,cflags,libs,Dependency::Type::BREW);
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
