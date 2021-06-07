#include "BrewSystemTool.h"
#include "utils/OsTools.h"

#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <boost/predef.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>

namespace fs = boost::filesystem;
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
        if ( libPath.extension().generic_string((utf8)) == OsTools::sharedSuffix(m_options.getOS())) {
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

fs::path BrewSystemTool::generateQmake(const std::vector<std::string>&  cflags, const std::vector<std::string>&  libs)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path filePath = DepTools::getProjectBuildSubFolder(m_options)/ "brewbuildinfo.pri";
    std::ofstream fos(filePath.generic_string(utf8),std::ios::out);
    std::string libdirs, libsStr;
    for (auto & cflagInfos : cflags) {
        std::vector<std::string> cflagsVect;
        boost::split(cflagsVect, cflagInfos, [&](char c){return c == ' ';});
        for (auto & cflag: cflagsVect) {
            // remove -I
            cflag.erase(0,2);
            boost::trim(cflag);
            fos<<"BREW_INCLUDEPATH += \""<<cflag<<"\""<<std::endl;
        }
    }
    for (auto & libInfos : libs) {
        std::vector<std::string> optionsVect;
        boost::split(optionsVect, libInfos, [&](char c){return c == ' ';});
        for (auto & option: optionsVect) {
            std::string prefix = option.substr(0,2);
            if (prefix == "-L") {
                option.erase(0,2);
                libdirs += " -L\"" + option + "\"";
            }
            //TODO : extract lib paths from libdefs and put quotes around libs path
            else {
                libsStr += " " + option;
            }
        }
    }
    fos<<"BREW_LIBS +="<<libsStr<<std::endl;
    fos<<"BREW_SYSTEMLIBS += "<<std::endl;
    fos<<"BREW_FRAMEWORKS += "<<std::endl;
    fos<<"BREW_FRAMEWORKS_PATH += "<<std::endl;
    fos<<"BREW_LIBDIRS +="<<libdirs<<std::endl;
    fos<<"BREW_BINDIRS +="<<std::endl;
    fos<<"BREW_DEFINES +="<<std::endl;
    fos<<"BREW_DEFINES +="<<std::endl;
    fos<<"BREW_QMAKE_CXXFLAGS +="<<std::endl;
    fos<<"BREW_QMAKE_CFLAGS +="<<std::endl;
    fos<<"BREW_QMAKE_LFLAGS +="<<std::endl;
    fos<<std::endl;
    fos<<"INCLUDEPATH += $$BREW_INCLUDEPATH"<<std::endl;
    fos<<"LIBS += $$BREW_LIBS"<<std::endl;
    fos<<"LIBS += $$BREW_LIBDIRS"<<std::endl;
    fos<<"BINDIRS += $$BREW_BINDIRS"<<std::endl;
    fos<<"DEFINES += $$BREW_DEFINES"<<std::endl;
    fos<<"LIBS += $$BREW_FRAMEWORKS"<<std::endl;
    fos<<"LIBS += $$BREW_FRAMEWORK_PATHS"<<std::endl;
    fos<<"QMAKE_CXXFLAGS += $$BREW_QMAKE_CXXFLAGS"<<std::endl;
    fos<<"QMAKE_CFLAGS += $$BREW_QMAKE_CFLAGS"<<std::endl;
    fos<<"QMAKE_LFLAGS += $$BREW_QMAKE_LFLAGS"<<std::endl;
    fos.close();
    return filePath;
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
    boost::filesystem::path pkgConfigToolPath = bp::search_path("pkg-config");
    if (pkgConfigToolPath.empty()) {
        throw std::runtime_error("Error: pkg-config tool not available: please install pkg-config !");
    }
    fs::detail::utf8_codecvt_facet utf8;
    fs::path globalBrewPkgConfigPath(brewRootPathMap.at(m_options.getOS()), utf8);
    globalBrewPkgConfigPath /= "lib";
    globalBrewPkgConfigPath /= "pkgconfig";
    auto env = boost::this_process::environment();
    // add to PKG_CONFIG_PATH default pkg-config path for brew
    std::string brewPkgconfigPaths = globalBrewPkgConfigPath.generic_string(utf8);

    for ( auto & dep : deps) {
        std::cout<<"===> Adding '"<<dep.getName()<<":"<<dep.getVersion()<<"' dependency"<<std::endl;
        // retrieve brew sub-dependencies for 'dep'
        std::vector<std::string> depsList = split( run ("deps", dep.getPackageName()) );
        // add base dependency to check for keg-only
        depsList.push_back(dep.getPackageName());
        // search for keg-only formulae
        for (auto & subDep : depsList) {
            std::string jsonInfos = run ("info", subDep, {"--json=v1"});
            //todo : ignore keg-only on linux
            if (jsonInfos.find("\"keg_only\":true") != std::string::npos) { // found keg-only formulae
                std::vector<std::string> filesList = split( run ("list", subDep) );
                fs::path localPkgConfigPath;
                for (auto & file : filesList) {
                    if (file.find("pkgconfig") != std::string::npos) { // found pkgconfig path for keg-only
                        localPkgConfigPath = file;
                    }
                }
                if (!localPkgConfigPath.empty()) { // add found pkgconfig to pkgConfigPath variable
                    brewPkgconfigPaths += ":" + localPkgConfigPath.parent_path().generic_string(utf8);
                }
            }
        }
    }

    // call pkg-config on dep and populate libs and cflags variables
    env["PKG_CONFIG_PATH"] = brewPkgconfigPaths;
    std::vector<std::string> cflags, libs;
    for ( auto & dep : deps) {
        boost::asio::io_context ios;
        std::future<std::string> listOutputFut;
        int result = bp::system(pkgConfigToolPath, "--cflags", env,  dep.getName(), bp::std_out > listOutputFut, ios);
        if (result != 0) {
            throw std::runtime_error("Error running pkg-config --cflags on '" + dep.getName() + "'");
        }
        cflags.push_back(listOutputFut.get());
        ios.reset();
        result = bp::system(pkgConfigToolPath, "--libs", env,  dep.getName(), bp::std_out > listOutputFut, ios);
        if (result != 0) {
            throw std::runtime_error("Error running pkg-config --libs on '" + dep.getName() + "'");
        }
        libs.push_back(listOutputFut.get());
    }

    // format CFLAGS and LIBS results
    if (generator == GeneratorType::qmake) {
        return generateQmake(cflags, libs);
    }
    else {
        throw std::runtime_error("Only qmake generator is supported, other generators' support coming in future releases");
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
