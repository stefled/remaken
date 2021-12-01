#include "ConanSystemTool.h"
#include "utils/OsUtils.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>

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


static const std::map<BaseSystemTool::PathType,std::string> conanNodeMap ={{BaseSystemTool::PathType::BIN_PATHS, "bin_paths"},                                                                            
                                                                            {BaseSystemTool::PathType::LIB_PATHS, "lib_paths"}
                                                                            };

void ConanSystemTool::update()
{
}

void ConanSystemTool::bundle(const Dependency & dependency)
{
    fs::path destination = m_options.getDestinationRoot();
    destination /= ".conan";

    std::vector<fs::path> libPaths = retrievePaths(dependency, BaseSystemTool::PathType::LIB_PATHS, destination);

    for (auto & libPath : libPaths) {
        if (boost::filesystem::exists(libPath)) {
            OsUtils::copySharedLibraries(libPath, m_options);
        }
    }
}

void ConanSystemTool::addRemoteImpl(const std::string & repositoryUrl)
{
    if (repositoryUrl.empty()) {
        return;
    }
    std::string remoteList = run ("remote","list");
    auto repoParts = split(repositoryUrl,'#');
    std::string repoId = repositoryUrl;
    std::vector<std::string> options;
    if (repoParts.size() < 2) {
        BOOST_LOG_TRIVIAL(warning)<<"Unable to add conan remote. Remote format must follow remoteAlias#remoteURL[#position]. Missing one of remoteAlias or remoteURL: " + repositoryUrl;
        return;
    }
    if (repoParts.size() >= 2) {
        repoId = repoParts.at(0);
        options.push_back(repoId);
        options.push_back(repoParts.at(1));
    }
    std::cout<<"Adding conan remote: "<<repoId;
    if (repoParts.size() == 3) {
        std::cout<<" position: "<<repoParts.at(2);
        options.push_back("--insert");
        options.push_back(repoParts.at(2));
    }
    std::cout<<std::endl;
    if (remoteList.find(repoId) == std::string::npos) {
        std::string result = run ("remote","add",options);
    }
}

void ConanSystemTool::addRemote(const std::string & remoteReference)
{
    addRemoteImpl(remoteReference);
}


void ConanSystemTool::listRemotes()
{
    std::vector<std::string> remoteList = split( run ("remote", "list") );
    std::cout<<"Conan remotes:"<<std::endl;
    for (const auto & remote: remoteList) {
         std::cout<<"=> "<<remote<<std::endl;
    }
}

void ConanSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    addRemote(dependency.getBaseRepository());
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
        profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
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

void ConanSystemTool::search(const std::string & pkgName, const std::string & version)
{
    std::string package = pkgName;
       if (!version.empty()) {
           package += "/" + version;
       }
       std::vector<std::string> foundDeps = split( run ("search", {"-r","all"}, package) );
       std::cout<<"Conan::search results:"<<std::endl;
       std::string currentRemote;
       for (auto & dep : foundDeps) {
           if (dep.find("Remote") != std::string::npos) {
               std::vector<std::string> remoteDetails = split(dep,'\'');
               currentRemote = remoteDetails.at(1);
           }
           std::string pkg = dep;
           std::string user, channel;
           if (dep.find('@') != std::string::npos) {
               std::vector<std::string> depInfos = split(dep,'@');
               pkg = depInfos.at(0);
               if (depInfos.size() == 2) {
                   std::vector<std::string> infoDetails = split(depInfos.at(1),'/');
                   user = infoDetails.at(0) + "@";
                   if (depInfos.size() == 2) {
                       channel = "#" + infoDetails.at(1);
                   }
               }

           }
           if (pkg.find('/') != std::string::npos) {
               std::vector<std::string> depDetails = split(pkg,'/');
               std::string name, version;
               name = depDetails.at(0);
               if (depDetails.size() == 2) {
                   version = depDetails.at(1);
               }
               std::cout<<dep<<"\t"<<name<<channel<<"|"<<version<<"|"<<name<<"|"<<user<<"conan|"<<currentRemote<<"|"<<std::endl;
           }
       }
}

std::string ConanSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = m_systemInstallerPath.generic_string(utf8);
    installCmd += " install ";
    if (dependency.getMode() != "na") {
            std::string buildMode = "shared=True";
            if (dependency.getMode() == "static") {
                buildMode = "shared=False";
            }
            installCmd += " -o " + buildMode;
    }
    std::string buildType = "build_type=Debug";
    if (m_options.getConfig() == "release") {
        buildType = "build_type=Release";
    }
    if (mapContains(conanArchTranslationMap, m_options.getArchitecture())) {
        installCmd += " -s arch="+ conanArchTranslationMap.at(m_options.getArchitecture());
    }
    installCmd += " -s " + buildType;
    installCmd += " -s compiler.cppstd=" + m_options.getCppVersion();
    std::string profileName = m_options.getConanProfile();
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
    }
    installCmd += " -pr " + profileName;

    if (dependency.hasOptions()) {
        std::vector<std::string> options;
        boost::split(options, dependency.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            installCmd += " -o " + option;
        }
    }
    return installCmd;
}

std::vector<std::string> ConanSystemTool::buildOptions(const Dependency & dep)
{
    std::vector<std::string> options;
    std::vector<std::string> results;
    if (dep.getMode() != "na") {
        std::string modeValue = "False";
        if (dep.getMode() == "shared") {
            modeValue = "True";
        }
        results.push_back(dep.getPackageName() + ":" + dep.getMode() + "=" + modeValue);
    }
    if (dep.hasOptions()) {
        boost::split(options, dep.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            results.push_back(dep.getPackageName() + ":" + option);
        }
    }
    return results;
}

fs::path ConanSystemTool::createConanFile(const std::vector<Dependency> & deps)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path conanFilePath = DepUtils::getProjectBuildSubFolder(m_options)/ "conanfile.txt";
    std::ofstream fos(conanFilePath.generic_string(utf8),std::ios::out);
    fos<<"[requires]"<<'\n';
    for (auto & dependency : deps) {
        std::string sourceURL = computeConanRef(dependency);
        std::cout<<"===> Adding '"<<sourceURL<<"' dependency"<<std::endl;
        fos<<sourceURL<<'\n';
    }
    fos<<'\n';
    fos<<"[generators]"<<'\n';
    fos<<"qmake"<<'\n';
    fos<<"\n";
    fos<<"[options]"<<'\n';
    for (auto & dependency : deps) {
        for (auto & option : buildOptions(dependency)) {
            fos<<option<<'\n';
        }
    }
    fos.close();
    return conanFilePath;
}

fs::path ConanSystemTool::invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path destination = DepUtils::getProjectBuildSubFolder(m_options);
    fs::path conanFilePath = createConanFile(deps);
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
    std::string profileName = m_options.getConanProfile();
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        std::string profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;
    result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(),  "-if", destination, "-g", "qmake", conanFilePath);
        //result = bp::system(m_systemInstallerPath, "install", conanFilePath.generic_string(utf8).c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-if",  targetPath.generic_string(utf8).c_str());

   if (result != 0) {
        throw std::runtime_error("Error installing conan dependencies");
    }
    return destination/"conanbuildinfo.pri";
}



std::vector<fs::path> ConanSystemTool::retrievePaths(const Dependency & dependency, BaseSystemTool::PathType conanNode, const fs::path & destination)
{
    std::vector<fs::path> conanPaths;
    std::string source = computeToolRef(dependency);
    std::string buildType = "build_type=Debug";

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
            auto conan_paths = dep[conanNodeMap.at(conanNode)];
            for (auto & conan_path : conan_paths) {
                boost::filesystem::path conanPath = std::string(conan_path);
                if (boost::filesystem::exists(conanPath)) {
                    conanPaths.push_back(conanPath);
                }
            }
        }
    }
    return conanPaths;
}

bool ConanSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string ConanSystemTool::computeConanRef( const Dependency &  dependency, bool cliMode )
{
    std::string sourceURL = dependency.getPackageName();
    sourceURL += "/" + dependency.getVersion();
    if (cliMode) {
        sourceURL += "@";
    }
    // decorate url for remotes other than conan-center index
    if ((dependency.getBaseRepository() != "conan-center")
        && (dependency.getBaseRepository() != "conancenter")) {
        if (!cliMode) {
            sourceURL += "@";
        }
        sourceURL +=  dependency.getIdentifier();
        sourceURL += "/" + dependency.getChannel();
    }
    return sourceURL;
}

std::string ConanSystemTool::computeToolRef(const Dependency &  dependency)
{
    return computeConanRef(dependency, true);
}

std::string ConanSystemTool::computeSourcePath(const Dependency &  dependency)
{
    std::string sourceURL = computeToolRef(dependency);
    sourceURL += "|" + m_options.getBuildConfig();
    sourceURL += "|" + dependency.getToolOptions();
    return sourceURL;
}

std::vector<fs::path> ConanSystemTool::binPaths(const Dependency & dependency)
{
    fs::path workingDirectory = OsUtils::acquireTempFolderPath();
    std::vector<fs::path> binPaths = retrievePaths(dependency, BaseSystemTool::PathType::BIN_PATHS, workingDirectory);
    OsUtils::releaseTempFolderPath(workingDirectory);
    return binPaths;
}

std::vector<fs::path> ConanSystemTool::libPaths(const Dependency & dependency)
{
    fs::path workingDirectory = OsUtils::acquireTempFolderPath();
    BaseSystemTool::PathType libPathNode = BaseSystemTool::PathType::LIB_PATHS;
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    if (m_options.crossCompiling() && m_options.getOS() != "win") {
        libPathNode = BaseSystemTool::PathType::LIB_PATHS;

    }
    else {
        libPathNode = BaseSystemTool::PathType::BIN_PATHS;

    }
#endif
    std::vector<fs::path> libPaths = retrievePaths(dependency, libPathNode, workingDirectory);
    OsUtils::releaseTempFolderPath(workingDirectory);
    return libPaths;
}

