#include "ConanSystemTool.h"
#include "utils/OsUtils.h"
#include "PkgConfigTool.h"
#include <boost/process.hpp>
#include <boost/predef.h>
#include <boost/algorithm/string.hpp>
#include "boost/lexical_cast.hpp"
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
                                                                           {BaseSystemTool::PathType::LIB_PATHS, "lib_paths"},
                                                                           {BaseSystemTool::PathType::INCLUDE_PATHS, "include_paths"}
                                                                          };


static const std::map<BaseSystemTool::PathType,std::string> conanV2NodeMap ={{BaseSystemTool::PathType::BIN_PATHS, "bindirs"},
                                                                             {BaseSystemTool::PathType::LIB_PATHS, "libdirs"},
                                                                             {BaseSystemTool::PathType::INCLUDE_PATHS, "includedirs"}
                                                                            };

static const std::map<GeneratorType, std::string> generatorConanV1TranslationMap = {{GeneratorType::qmake,"qmake"},
                                                                                    {GeneratorType::cmake,"cmake"},
                                                                                    {GeneratorType::pkg_config,"pkg_config"},
                                                                                    {GeneratorType::make,"make"},
                                                                                    {GeneratorType::json,"json"},
                                                                                    {GeneratorType::bazel,"json"} // generate json then interpret to bazel
                                                                                    };

static const std::map<GeneratorType, std::string> generatorConanV2TranslationMap = {{GeneratorType::qmake,"PkgConfigDeps"},
                                                                                    {GeneratorType::cmake,"cmake"},
                                                                                    {GeneratorType::pkg_config,"pkg_config"},
                                                                                    {GeneratorType::make,"make"},
                                                                                    {GeneratorType::json,"json"},
                                                                                    {GeneratorType::bazel,"json"} // generate json then interpret to bazel
                                                                                    };


void ConanSystemTool::update()
{
}

void ConanSystemTool::bundle(const Dependency & dependency)
{
    fs::path destination = m_options.getDestinationRoot();
    destination /= ".conan";

    BaseSystemTool::PathType libPathNode = BaseSystemTool::PathType::LIB_PATHS;

#ifdef BOOST_OS_WINDOWS_AVAILABLE
    if (m_options.crossCompiling() && m_options.getOS() != "win") {
        libPathNode = BaseSystemTool::PathType::LIB_PATHS;

    }
    else {
        libPathNode = BaseSystemTool::PathType::BIN_PATHS;

    }
#endif

    std::vector<fs::path> libPaths = retrievePaths(dependency, libPathNode, destination);
    for (auto & libPath : libPaths) {
        if (boost::filesystem::exists(libPath)) {
            OsUtils::copySharedLibraries(libPath, m_options);
        }
    }
}

void ConanSystemTool::addRemoteImpl(const std::string & repositoryUrl)
{
    std::string remoteList = run ("remote","list");
    auto repoParts = split(repositoryUrl,'#');
    std::string repoId = repositoryUrl;
    std::vector<std::string> options;
    if (repoParts.size() < 2) {
        if (m_options.getVerbose()) {
            BOOST_LOG_TRIVIAL(info)<<"Using existing conan remote : " << repositoryUrl << ". Remote Url can be specified with remoteAlias#[remoteUrl[#position]]"<<std::endl;
        }
        return;
    }
    if (repoParts.size() >= 2) {
        repoId = repoParts.at(0);
        if (repoId == "conan-center") {
            repoId = "conancenter";
        }
        options.push_back(repoId);
        options.push_back(repoParts.at(1));
    }
    if (repoParts.size() == 3) {

        if (m_conanVersion < 2) {
            options.push_back("--insert");
        } else {
            options.push_back("--index");
        }
        options.push_back(repoParts.at(2));
    }

    auto remotelistParts = split(remoteList, '\n');
    std::vector<std::string> conanRemoteAliases;
    std::vector<std::string> conanRemoteUrls;
    for (auto remoteElem : remotelistParts) {
        auto remoteElemParts = split(remoteElem, ' ');
        if (remoteElemParts.size() >= 2) {
            conanRemoteAliases.push_back(remoteElemParts.at(0));
            conanRemoteUrls.push_back(remoteElemParts.at(1));
        }
    }

    if ( std::find(conanRemoteAliases.begin(), conanRemoteAliases.end(), repoId) == conanRemoteAliases.end() &&
         std::find(conanRemoteUrls.begin(), conanRemoteUrls.end(), repoParts.at(1)) == conanRemoteUrls.end()) {
        std::cout<<"Adding conan remote "<<repoId << " " << repoParts.at(1) <<" at "<<repoParts.at(2)<<std::endl;
        std::string result = run ("remote","add",options);
        return;
    }
}

void ConanSystemTool::addRemote(const std::string & remoteReference)
{
    if (remoteReference.empty()) {
        return;
    }
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
    fs::detail::utf8_codecvt_facet utf8;

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
        std::string separator = "";
        if (m_conanVersion >= 2) {
            separator = "/*";
        }
        boost::split(options, dependency.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            std::vector<std::string> optionInfos;
            boost::split(optionInfos, option, [](char c){return c == ':';});
            std::string conanOptionPrefix = optionInfos.front();
            optionInfos.erase(optionInfos.begin());
            if (optionInfos.empty()) {
                optionsArgs.push_back("-o " + option);
            } else {
                optionsArgs.push_back("-o " + conanOptionPrefix + separator + ":" + optionInfos.front());
            }
        }
    }
    std::string profileName = m_options.getConanProfile();
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
    }
    std::string cppStd="compiler.cppstd=";
    cppStd += m_options.getCppVersion();
    int result = -1;

    std::string buildForceDep = "--build=missing";
    std::vector<std::string> parsedArgs;
    for (auto & arg : m_options.getConanForceBuildRefs()) {
        if (arg == dependency.getPackageName()) {
            buildForceDep = "--build=" + arg;
            break;
        }
    }

    if (dependency.getMode() == "na") {
        if (m_options.getVerbose())
        {
            std::cout << m_systemInstallerPath.generic_string(utf8) << " install " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << buildForceDep.c_str() << " " << boost::algorithm::join(optionsArgs, " ") << " " << source.c_str() << std::endl;
        }
        result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), buildForceDep.c_str(), bp::args(optionsArgs), source.c_str());
    }
    else {
        std::string buildMode = "shared=True";
        if (dependency.getMode() == "static") {
            buildMode = "shared=False";
        }

        if (m_options.getVerbose())
        {
            std::cout << m_systemInstallerPath.generic_string(utf8) << " install " << "-o " << buildMode.c_str() << " " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << buildForceDep.c_str() << " " << boost::algorithm::join(optionsArgs, " ") << " " << source.c_str() << std::endl;
        }
        result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), buildForceDep.c_str(), bp::args(optionsArgs), source.c_str());
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

    std::vector<std::string> remoteSearch; // Conan V2
    if (m_conanVersion < 2) { // Conan V1
        remoteSearch.push_back("-r");
        remoteSearch.push_back("all");
    }

    std::vector<std::string> foundDeps = split( run ("search", remoteSearch, package) );
    std::cout<<"Conan::search results:"<<std::endl;

    std::string currentRemote;
    for (auto & dep : foundDeps) {
        if (m_conanVersion < 2) { // Conan V1
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
        else {  // Conan V2
            if (dep.find("    ") != std::string::npos) {
                boost::trim(dep);
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
            else if (dep.find("  ") != std::string::npos) {
            }
            else {
                currentRemote = dep;
            }
        }
    }
}

std::string ConanSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    // SLETODO : not used : need to refactor with conan v1/v2 management!
    /*
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
    */


    std::cout << "Remaken \"ConanSystemTool::retrieveInstallCommand\" method not implemented - Please contact developper" << std::endl;
    return "";
}

std::vector<std::string> ConanSystemTool::buildOptions(const Dependency & dep)
{
    std::string separator = "";
    if (m_conanVersion >= 2) {
        separator = "/*";
    }

    std::vector<std::string> options;
    std::vector<std::string> results;
    if (dep.getMode() != "na") {
        std::string modeValue = "False";
        if (dep.getMode() == "shared") {
            modeValue = "True";
        }
        results.push_back(dep.getPackageName() + separator + ":shared=" + modeValue);
    }
    if (dep.hasOptions()) {
        boost::split(options, dep.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            std::vector<std::string> optionInfos;
            boost::split(optionInfos, option, [](char c){return c == ':';});
            std::string conanOptionPrefix = optionInfos.front();
            optionInfos.erase(optionInfos.begin());
            if (optionInfos.empty()) {
                results.push_back(dep.getPackageName() + separator + ":" + option);
            } else {
                results.push_back(conanOptionPrefix + separator + ":" + optionInfos.front());
            }
        }
    }
    return results;
}

fs::path ConanSystemTool::createConanFile(const std::vector<Dependency> & deps)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path conanFilePath = DepUtils::getProjectBuildSubFolder(m_options)/ "conanfile.txt";
    if (!m_options.getDestinationRoot().empty()){
        conanFilePath = m_options.getDestinationRoot()/ "conanfile.txt";
    }
    std::ofstream fos(conanFilePath.generic_string(utf8),std::ios::out);
    fos<<"[requires]"<<'\n';
    for (auto & dependency : deps) {
        std::string sourceURL = computeConanRef(dependency);
        std::cout<<"===> Adding '"<<sourceURL<<"' dependency"<<std::endl;
        fos<<sourceURL<<'\n';
    }
    fos<<'\n';
    fos<<"[generators]"<<'\n';
    std::string generator = generatorConanV1TranslationMap.at(m_options.getGenerator());
    if (m_conanVersion >= 2) {
        generator = generatorConanV2TranslationMap.at(m_options.getGenerator());
    }
    fos<<generator<<'\n';
    fos<<"\n";
    fos<<"[options]"<<'\n';
    std::vector<std::string> options;
    for (auto & dependency : deps) {
        for (auto & option : buildOptions(dependency)) {
            if (std::find(options.begin(), options.end(), option) == options.end()) {
                options.push_back(option);
            }
        }
    }
    for (const auto & option: options) {
        fos<<option<<'\n';
    }
    fos.close();
    return conanFilePath;
}

void ConanSystemTool::translateJsonToRemakenDep(std::vector<Dependency> & deps, const fs::path & conanJsonBuildInfo)
{
    std::map<std::string,Dependency&> depsMap;
    for (auto & dep : deps) {
        depsMap.insert({dep.getPackageName(),dep});
    }
    std::vector<fs::path> conanPaths;
    BaseSystemTool::PathType libPathNode = BaseSystemTool::PathType::LIB_PATHS;

#ifdef BOOST_OS_WINDOWS_AVAILABLE
    if (m_options.crossCompiling() && m_options.getOS() != "win") {
        libPathNode = BaseSystemTool::PathType::LIB_PATHS;

    }
    else {
        libPathNode = BaseSystemTool::PathType::BIN_PATHS;

    }
#endif

    if (m_conanVersion < 2) {   // conan v1 - used only with bazel generator
        fs::detail::utf8_codecvt_facet utf8;
        if (fs::exists(conanJsonBuildInfo)) {
            std::ifstream ifs1{ conanJsonBuildInfo.generic_string(utf8) };
            nj::json conanBuildData = nj::json::parse(ifs1);
            for (auto dep : conanBuildData["dependencies"]) {
                std::string name = dep["name"];
                std::string version = dep["version"];
                std::string root = dep["rootpath"];
                std::vector<std::string> conan_bin_paths = dep["bin_paths"];
                std::vector<std::string> conan_lib_paths = dep["lib_paths"];
                std::vector<std::string> conan_include_paths = dep["include_paths"];
                std::vector<std::string> conan_libs = dep["libs"];
                if (!mapContains(depsMap,name)) {
                    Dependency d(m_options, name, version, Dependency::Type::CONAN);
                    depsMap.insert({name,d});
                    deps.push_back(d);
                }
                auto & rDep = depsMap.at(name);
                rDep.prefix() = root;
                fs::path rootPath(root,utf8);
                for (auto & libPath: conan_bin_paths) {
                    rDep.libdirs().push_back(libPath);
                }
                for (auto & libPath: conan_lib_paths) {
                    rDep.libdirs().push_back(libPath);
                }
                for (auto & include_path : conan_include_paths) {
                    rDep.cflags().push_back("-I" + include_path);
                }
                for (auto & lib : conan_libs) {
                    rDep.libs().push_back("-l" + lib);
                }
            }
        }
    }
    else {
        std::cout << "Remaken \"ConanSystemTool::translateJsonToRemakenDep\" method for conan v2 not implemented - Please contact developper" << std::endl;
    }
}

// configure command
std::pair<std::string, fs::path> ConanSystemTool::invokeGenerator(std::vector<Dependency> & deps)
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

    std::string dest_param = "-if";
    std::string generator = generatorConanV1TranslationMap.at(m_options.getGenerator());
    if (m_conanVersion >= 2) {
        dest_param = "-of";
        generator = generatorConanV2TranslationMap.at(m_options.getGenerator());
    }

    if (m_options.getVerbose()) {
        std::cout << m_systemInstallerPath.generic_string(utf8) << " install " << " " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << dest_param.c_str() << " " << destination.generic_string(utf8) << " -g " << generator.c_str() << " " << conanFilePath.generic_string(utf8) << std::endl;
    }
    result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(),  dest_param.c_str(), destination, "-g", generator, conanFilePath);

    if (result != 0) {
        throw std::runtime_error("Error installing conan dependencies");
    }

    if (m_options.getGenerator() == GeneratorType::bazel) {
         translateJsonToRemakenDep(deps,destination/"conanbuildinfo.json");
         PkgConfigTool pkgConfig(m_options);
         return pkgConfig.generate(deps, Dependency::Type::CONAN);
    }
    return {"conan_basic_setup",destination/m_options.getGeneratorFilePath("conanbuildinfo")};
}

void ConanSystemTool::write_pkg_file(std::vector<Dependency> & deps)
{
    fs::path destination = DepUtils::getProjectBuildSubFolder(m_options);
    fs::path conanFilePath = createConanFile(deps);
}

// Bundle
std::vector<fs::path> ConanSystemTool::retrievePaths(const Dependency & dependency, BaseSystemTool::PathType conanNode, const fs::path & destination)
{
    fs::detail::utf8_codecvt_facet utf8;

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
    std::string profileName = m_options.getConanProfile();
    if (m_options.crossCompiling() && m_options.getConanProfile() == "default") {
        profileName = m_options.getOS() + "-" + m_options.getBuildToolchain() + "-" + m_options.getArchitecture();
    }

    std::string dest_param = "-if";
    std::string generator_param = "-g"; // conan V1
    if (m_conanVersion >= 2) {
        dest_param = "-of";
        generator_param = "-f";
    }

    if (m_conanVersion < 2) {       // conan V1
        if (dependency.getMode() == "na") {
            if (m_options.getVerbose()) {
                std::cout << m_systemInstallerPath.generic_string(utf8) << " install " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << dest_param.c_str() << " " << destination << " " << boost::algorithm::join(optionsArgs, " ") << " " << generator_param.c_str() << " json " << source.c_str() << std::endl;
                result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), destination, bp::args(optionsArgs), generator_param.c_str(), "json", source.c_str());
            } else {
                result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), destination, bp::args(optionsArgs), generator_param.c_str(), "json", source.c_str(), bp::std_out > bp::null);
            }
        }
        else {
            std::string buildMode = "";//dependency.getName() + ":";
            if (dependency.getMode() == "static") {
                buildMode += "shared=False";
            }
            else {
                buildMode += "shared=True";
            }
            if (m_options.getVerbose()) {
                std::cout << m_systemInstallerPath.generic_string(utf8) << " install " << "-o " << buildMode.c_str() << " " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << dest_param.c_str() << " " << destination << " " << boost::algorithm::join(optionsArgs, " ") << " " << generator_param.c_str() << " json " << source.c_str() << std::endl;
                result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), destination, bp::args(optionsArgs), generator_param.c_str(), "json", source.c_str());
            } else {
                result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), destination, bp::args(optionsArgs), generator_param.c_str(), "json", source.c_str(), bp::std_out > bp::null);
            }
        }
        if (result != 0) {
            throw std::runtime_error("Error bundling conan dependency : " + source);
        }

        std::string fileName = dependency.getPackageName() + "_conanbuildinfo.json";
        fs::path conanBuildInfoJson = destination/fileName;
        fs::copy_file(destination/"conanbuildinfo.json", conanBuildInfoJson, fs::copy_options::overwrite_existing);
        fs::remove(destination/"conanbuildinfo.json");
        if (fs::exists(conanBuildInfoJson)) {
            std::ifstream ifs1{ conanBuildInfoJson.generic_string(utf8) };
            nj::json conanBuildData = nj::json::parse(ifs1);
            for (auto dep : conanBuildData["dependencies"]) {
                //auto root = dep["rootpath"];
                auto conan_paths = dep[conanNodeMap.at(conanNode)];
                for (auto & conan_path : conan_paths) {
                    boost::filesystem::path conanPath = std::string(conan_path);
                    if (boost::filesystem::exists(conanPath)) {
                        conanPaths.push_back(conanPath);
                    }
                }
            }
        }
    } else { // Conan V2
        fs::path workingDirectory = OsUtils::acquireTempFolderPath();
        std::string fileName = dependency.getPackageName() + "_conanbuildinfo.json";
        fs::path conanBuildInfoJson = workingDirectory/fileName;

        if (dependency.getMode() == "na") {
            std::string command = m_systemInstallerPath.generic_string(utf8) + " install " + boost::algorithm::join(settingsArgs, " ") + " -s " + buildType +
                                  " -s " + cppStd + " -pr " + profileName + " " + dest_param + " " + workingDirectory.generic_string(utf8) + " " + boost::algorithm::join(optionsArgs, " ") + " " + generator_param + " json " + source + " > " + conanBuildInfoJson.generic_string(utf8);
            if (m_options.getVerbose()) {
                //std::cout << m_systemInstallerPath << " install " << boost::algorithm::join(settingsArgs, " ") << " -s " << buildType.c_str() << " -s " << cppStd.c_str() << " -pr " << profileName.c_str() << " " << dest_param.c_str() << workingDirectory.generic_string(utf8).c_str() << boost::algorithm::join(optionsArgs, " ") << " " << source.c_str() << std::endl;
                //result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), workingDirectory, bp::args(optionsArgs), source.c_str());

                std::cout << command.c_str() << std::endl;
                result = std::system(command.c_str());
            } else {
                // SLETODO : issue with : bp::std_out > bp::null => use std::system (ok with it)
                //result = bp::system(m_systemInstallerPath, "install", bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), workingDirectory, bp::args(optionsArgs), source.c_str());
                result = std::system(command.c_str());
            }
        }
        else {
            std::string buildMode = "";//dependency.getName() + ":";
            if (dependency.getMode() == "static") {
                buildMode += "shared=False";
            }
            else {
                buildMode += "shared=True";
            }

            std::string command = m_systemInstallerPath.generic_string(utf8) + " install " + "-o " + buildMode + " " + boost::algorithm::join(settingsArgs, " ") + " -s " + buildType +
                                  " -s " + cppStd + " -pr " + profileName + " " + dest_param + " " + workingDirectory.generic_string(utf8) + " " + boost::algorithm::join(optionsArgs, " ") + " " + generator_param + " json " + source + " > " + conanBuildInfoJson.generic_string(utf8);

            if (m_options.getVerbose()) {
                //result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), workingDirectory, bp::args(optionsArgs), source.c_str());
                std::cout << command.c_str() << std::endl;
                result = std::system(command.c_str());
            }
            else {
                // SLETODO : issue with : bp::std_out > bp::null => use std::system (ok with it)
                //result = bp::system(m_systemInstallerPath, "install", "-o", buildMode.c_str(), bp::args(settingsArgs), "-s", buildType.c_str(), "-s", cppStd.c_str(), "-pr", profileName.c_str(), dest_param.c_str(), workingDirectory, bp::args(optionsArgs), source.c_str());
                result = std::system(command.c_str());
            }
        }
        if (result != 0) {
            OsUtils::releaseTempFolderPath(workingDirectory);
            throw std::runtime_error("Error bundling conan dependency : " + source);
        }

        if (fs::exists(conanBuildInfoJson)) {
            std::ifstream ifs1{ conanBuildInfoJson.generic_string(utf8) };
            nj::json conanBuildData = nj::json::parse(ifs1);

            for (const auto& item : conanBuildData["graph"]["nodes"])
            {
                if (item.find("binary") != item.end() && (item["binary"] == "Download" || item["binary"] == "Cache")) // SLETODO or Build? need to be tested
                {
                    if (item.find("cpp_info") != item.end() &&
                        item["cpp_info"].find("root")!= item["cpp_info"].end())
                    {
                        auto rootItem = item["cpp_info"]["root"];
                        if (rootItem.find(conanV2NodeMap.at(conanNode)) != rootItem.end()) {
                            for (auto & conan_path : rootItem[conanV2NodeMap.at(conanNode)]) {
                                boost::filesystem::path conanPath = std::string(conan_path);
                                if (boost::filesystem::exists(conanPath)) {
                                    conanPaths.push_back(conanPath);
                                    //std::cout << conanPath.generic_string(utf8).c_str() << std::endl;
                                }
                            }
                        }
                    }

                }
            }
        }
        OsUtils::releaseTempFolderPath(workingDirectory);
    }

    return conanPaths;
}

bool ConanSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string ConanSystemTool::computeConanRef( const Dependency &  dependency, bool cliMode )
{
    std::string sourceURL;

    if (cliMode && m_conanVersion >= 2) {
        sourceURL = "--requires=";
    }
    sourceURL += dependency.getPackageName();
    sourceURL += "/" + dependency.getVersion();
    if (cliMode) {
        sourceURL += "@";
    }
    // decorate url for remotes other than conan-center index
    auto repoParts = split(dependency.getBaseRepository(),'#');
    auto repoBaseName = repoParts.at(0);

    if ((repoBaseName != "conan-center")
        && (repoBaseName != "conancenter")) {
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

// SLETODO : soit on fait une nouvelle methode staticlibpath, soit parametre de fonction, soit on regarde le mode de la lib 'shared/static' pour determiner le chemin...

std::vector<fs::path> ConanSystemTool::libPaths(const Dependency & dependency)
{
    fs::path workingDirectory = OsUtils::acquireTempFolderPath();
    BaseSystemTool::PathType libPathNode = BaseSystemTool::PathType::LIB_PATHS;
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    if ((m_options.crossCompiling() && m_options.getOS() != "win") ||
        (dependency.getMode()=="static" && m_options.getOS() == "win")) {
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

std::vector<fs::path> ConanSystemTool::includePaths(const Dependency & dependency)
{
    fs::path workingDirectory = OsUtils::acquireTempFolderPath();
    BaseSystemTool::PathType includePathNode = BaseSystemTool::PathType::INCLUDE_PATHS;
    std::vector<fs::path> includePaths = retrievePaths(dependency, includePathNode, workingDirectory);
    OsUtils::releaseTempFolderPath(workingDirectory);
    return includePaths;
}


int ConanSystemTool::conanVersion()
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    if (m_options.getVerbose()) {
        std::cout << m_systemInstallerPath.generic_string(utf8) << " --version" << std::endl;
    }
    int result = bp::system(m_systemInstallerPath, "--version", bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running conan --version");
    }

    std::string res = listOutputFut.get();
    boost::erase_all(res, "\r\n");

    std::vector<std::string> outVect;
    boost::split(outVect, res, [&](char c) {return c == ' ';}); // split line
    boost::split(outVect, outVect.back(), [&](char c) {return c == '.';});  //split version

    int majorVersion = 0;
    try {
      majorVersion = boost::lexical_cast<int>(outVect.front()); // double could be anything with >> operator.
    }
    catch (std::exception& e) {
        throw std::runtime_error("Unable to get conan version");
    }
    return majorVersion;
}
