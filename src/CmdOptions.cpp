#include "CmdOptions.h"
#include <iostream>

#include "Constants.h"
#include <exception>
#include <algorithm>
#include "ZipTool.h"
#include <boost/process.hpp>
#include <boost/predef.h>
#include "PathBuilder.h"
namespace bp = boost::process;
using namespace std;

std::string computeOS()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return "android";
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return "ios";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return "linux";
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "solaris";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "mac";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "win";
#endif
}

std::string computeToolChain()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return "clang";
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return "clang";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return "gcc";
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "gcc";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "clang";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "cl";
#endif
}


CmdOptions::CmdOptions()
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPath = PathBuilder::getHomePath() / Constants::REMAKEN_FOLDER;
    char * rootDirectoryVar = getenv(Constants::REMAKENPKGROOT);
    if (rootDirectoryVar != nullptr) {
        std::cerr<<Constants::REMAKENPKGROOT<<" environment variable exists"<<std::endl;
        remakenRootPath = rootDirectoryVar;
    }
    else if (fs::exists(remakenRootPath / Constants::REMAKENPKGFILENAME)) {
        fs::path pkgFile = remakenRootPath / Constants::REMAKENPKGFILENAME;
        ifstream fis(pkgFile.string(utf8),ios::in);
        fs::path pkgPath;
        while (!fis.eof()) {
            std::string curLine;
            getline(fis,curLine);
            if (!curLine.empty()) {
                pkgPath = curLine;
            }
        }
        fis.close();
        remakenRootPath = pkgPath;
    }
    remakenRootPath /= "packages";

    fs::path configPath = PathBuilder::getHomePath() / Constants::REMAKEN_FOLDER / "config";
    m_cliApp.require_subcommand(1);
    m_cliApp.fallthrough(true);
    m_cliApp.set_config("--configfile",configPath.generic_string(utf8),"remaken configuration file to read");

    m_config = "release";
    m_cliApp.add_option("--config,-c", m_config, "Config: release, debug", true);
    m_cppVersion = "11";
    m_cliApp.add_option("--cpp-std",m_cppVersion, "c++ standard version: 11, 14, 17, 20 ...", true);
    m_remakenRoot = remakenRootPath.generic_string(utf8);
    m_cliApp.add_option("--remaken-root,-r", m_remakenRoot, "Remaken root directory", true);
    m_toolchain = computeToolChain();
    m_cliApp.add_option("--build-toolchain,-b", m_toolchain, "Build toolchain: clang, clang-version, "
                                                             "gcc-version, cl-version .. ex: cl-14.1", true);
    m_os = computeOS();
    m_cliApp.add_option("--operating-system,-o", m_os, "Operating system: mac, win, unix, ios, android", true);
    m_architecture = "x86_64";
    m_cliApp.add_option("--architecture,-a",m_architecture, "Architecture: x86_64, i386, arm, arm64, arm64-v8a, armeabi-v7a, armv6, armv7, armv7hf, armv8",true);
    m_verbose = false;
    m_cliApp.add_flag("--verbose,-v", m_verbose, "verbose mode");
    m_dependenciesFile = "packagedependencies.txt";

    CLI::App * bundleCommand = m_cliApp.add_subcommand("bundle","copy shared libraries dependencies to a destination folder");
    bundleCommand->add_option("--destination,-d", m_destinationRoot, "Destination directory")->required();
    bundleCommand->add_option("file", m_dependenciesFile, "Remaken dependencies files", true);

    CLI::App * bundleXpcfCommand = m_cliApp.add_subcommand("bundleXpcf","copy xpcf modules and their dependencies from their declaration in a xpcf xml file");
    m_moduleSubfolder = "modules";
    bundleXpcfCommand->add_option("--destination,-d", m_destinationRoot, "Destination directory")->required();
    bundleXpcfCommand->add_option("--modules-subfolder,-s", m_moduleSubfolder, "relative folder where XPCF modules will be "
                                                                               "copied with their dependencies", true);
    bundleXpcfCommand->add_option("file", m_dependenciesFile, "XPCF xml module declaration file")->required();

    CLI::App * cleanCommand = m_cliApp.add_subcommand("clean","WARNING : remove every remaken installed packages");

    CLI::App * profileCommand = m_cliApp.add_subcommand("profile","manage remaken profiles configuration");
    CLI::App * initProfileCommand = profileCommand->add_subcommand("init","create remaken default profile from current options");
    CLI::App * displayProfileCommand = profileCommand->add_subcommand("display","display remaken current profile (display current options and profile options)");
    m_defaultProfileOptions = false;
    displayProfileCommand->add_flag("--with-default,-w", m_defaultProfileOptions, "display all profile options : default and provided");
    initProfileCommand ->add_flag("--with-default,-w", m_defaultProfileOptions, "create remaken profile with all profile options : default and provided");

    CLI::App * initCommand = m_cliApp.add_subcommand("init","initialize remaken root folder and retrieve qmake rules");
    m_qmakeRulesTag = Constants::QMAKE_RULES_DEFAULT_TAG;
    initCommand->add_option("--tag", m_qmakeRulesTag, "the qmake rules tag version to install - either provide a tag or the keyword 'latest' to install latest qmake rules",true);

    CLI::App * versionCommand = m_cliApp.add_subcommand("version","display remaken version");

    CLI::App * installCommand = m_cliApp.add_subcommand("install","install dependencies for a package from its packagedependencies file(s)");
    installCommand->add_option("--alternate-remote-type,-l", m_altRepoType, "alternate remote type: github, artifactory, nexus, path");
    installCommand->add_option("--alternate-remote-url,-u", m_altRepoUrl, "alternate remote url to use when the declared remote fails to provide a dependency");
    installCommand->add_option("--apiKey,-k", m_apiKey, "Artifactory api key");
    m_override = false;
    installCommand->add_flag("--override,-e", m_override, "override existing files while (re)-installing packages");
    installCommand->add_option("file", m_dependenciesFile, "Remaken dependencies files", true);

    m_ignoreCache = false;
    installCommand->add_flag("--ignore-cache,-i", m_ignoreCache, "ignore cache entries : dependencies update is forced");
    m_mode = "shared";
    installCommand->add_option("--mode,-m", m_mode, "Mode: shared, static", true);
    m_repositoryType = "github";
    installCommand->add_option("--type,-t", m_repositoryType, "Repository type: github, artifactory, nexus, path", true);
    m_zipTool = ZipTool::getZipToolIdentifier();
    installCommand->add_option("--ziptool,-z", m_zipTool, "unzipper tool name : unzip, compact ...", true);

    CLI::App * packageCommand = m_cliApp.add_subcommand("package","package a build result in remaken format");
    packageCommand->add_option("--sourcedir,-s", m_packageOptions["sourcedir"], "product root directory (where libs and includes are located)", true)->required();
    packageCommand->add_option("--includedir,-i", m_packageOptions["sourcedir"], " relative path to include folder to export (defaults to the sourcedir provided with -s)\n");
    packageCommand->add_option("--libdir,-l", m_packageOptions["sourcedir"], " relative path to the library folder to export (defaults to the sourcedir provided with -s)\n");
    packageCommand->add_option("--redistfile,-f", m_packageOptions["sourcedir"], " relative path and filename of a redistribution file to use (such as redist.txt intel ipp's file). Only listed libraries in this file will be packaged\n");
    packageCommand->add_option("--destination,-d", m_destinationRoot, "Destination directory")->required();
    packageCommand->add_option("--packagename,-p", m_packageOptions["sourcedir"], " package name\n");
    packageCommand->add_option("--packageversion,-k", m_packageOptions["sourcedir"], " package version\n");
    packageCommand->add_option("--ignore-mode,-n", m_packageOptions["sourcedir"], " forces the pkg-config generated file to ignore the mode when providing -L flags\n");
    m_mode = "shared";
    packageCommand->add_option("--mode,-m", m_mode, "Mode: shared, static", true);
    packageCommand->add_option("--withsuffix,-w", m_packageOptions["sourcedir"], " specify the suffix used by the thirdparty when building with mode mode\n");
    packageCommand->add_option("--useOriginalPCfiles,-u", m_packageOptions["sourcedir"], " specify to search and use original pkgconfig files from the thirdparty, instead of generating them\n");

    /*print "    -s, --sourcedir                  => product root directory (where libs and includes are located)\n");
    print "    -o, --osname                     => specify the operating system targeted by the product build. It is one of [win|mac|linux]. (defaults to the current OS environment)\n";
    print "    -i, --includedir                 => relative path to include folder to export (defaults to the sourcedir provided with -s)\n";
    print "    -l, --libdir                     => relative path to the library folder to export (defaults to the sourcedir provided with -s)\n";
    print "    -r, --redistfile                 => relative path and filename of a redistribution file to use (such as redist.txt intel ipp's file). Only listed libraries in this file will be packaged\n";
    print "    -d, --destinationdir             => package directory root destination (where the resulting packaging will be stored)\n";
    print "    -p, --packagename                => package name\n";
    print "    -v, --packageversion             => package version\n";
    print "    -n, --ignore-mode                => forces the pkg-config generated file to ignore the mode when providing -L flags\n";
    print "    -m, --mode [debug|release]       => specify the current product build mode. Binaries will be packaged in the appropriate [mode] folder\n";
    print "    -a, --architecture [x86_64|i386] => specify the current product build architecture. Binaries will be packaged in the appropriate [architecture] folder\n";
    print "    -w, --withsuffix suffix          => specify the suffix used by the thirdparty when building with mode mode\n";
    print "    -u, --useOriginalPCfiles         => specify to search and use original pkgconfig files from the thirdparty, instead of generating them\n";
   */

    CLI::App * parseCommand = m_cliApp.add_subcommand("parse","check dependency file validity");
    parseCommand->add_option("file", m_dependenciesFile, "Remaken dependencies files", true);
}


static const map<std::string,std::vector<std::string>> validationMap ={{"action",{"install","parse","version","bundle", "bundleXpcf"}},
                                                                       {"--architecture",{"x86_64","i386","arm","arm64","arm64-v8a","armeabi-v7a","armv6","armv7","armv7hf","armv8"}},
                                                                       {"--config",{"release","debug"}},
                                                                       {"--mode",{"shared","static"}},
                                                                       {"--type",{"github","artifactory","nexus","path"}},
                                                                       {"--alternate-remote-type",{"github","artifactory","nexus","path"}},
                                                                       {"--operating-system",{"mac","win","unix","android","ios","linux"}},
                                                                       {"--cpp-std",{"11","14","17","20"}}
                                                                      };

void CmdOptions::initBuildConfig()
{
    m_buildConfig = getOS();
    m_buildConfig += "-" + getBuildToolchain();
    m_buildConfig += "|" + getArchitecture();
    m_buildConfig += "|" + getCppVersion();
    m_buildConfig += "|" + getMode();
    m_buildConfig += "|" + getConfig();
}

void CmdOptions::validateOptions()
{
    for (auto opt : m_cliApp.get_options()) {
        auto name = opt->get_name();
        auto value = opt->as<std::string>();
        if (validationMap.find(name) != validationMap.end()) {
            auto && validValues = validationMap.at(name);
            if (std::find(validValues.begin(), validValues.end(), value) == validValues.end()) {
                string message("Option " + name);
                message += " was set with invalid value " ;
                message += value;
                throw std::runtime_error(message);
            }
        }
    }
    auto sub = m_cliApp.get_subcommands().at(0);

    if (sub->get_name() == "bundleXpcf") {
        m_isXpcfBundle = true;
    }

    if (sub->get_name() == "profile") {
        if (sub->get_subcommands().size() == 0) {
            string message("Command profile ");
            message += " was used without subcommand" ;
            throw std::runtime_error(message);
        }
     }
    fs::path zipToolPath = bp::search_path(m_zipTool); //or get it from somewhere else.
    if (zipToolPath.empty()) {
        throw std::runtime_error("Error : " + m_zipTool + " command not found on the system. Please install it first.");
    }
}

CmdOptions::OptionResult CmdOptions::parseArguments(int argc, char** argv)
{
    try {
        fs::detail::utf8_codecvt_facet utf8;
        m_cliApp.parse(argc, argv);
        validateOptions();
        auto sub = m_cliApp.get_subcommands().at(0);
        if (sub->get_name() == "profile") {
            m_profileSubcommand = sub->get_subcommands().at(0)->get_name();
        }
        if (m_cliApp.get_subcommands().size() == 1) {
            m_action = m_cliApp.get_subcommands().at(0)->get_name();
        }
        else {
            m_action = "help";
            return OptionResult::RESULT_ERROR; //Usage ??
        }
        // path is assigned after to ensure utf8 codecvt
        m_destinationRootPath.assign(m_destinationRoot,utf8);
        m_remakenRootPath.assign(m_remakenRoot,utf8);
        m_moduleSubfolderPath.assign(m_moduleSubfolder,utf8);
    }
    catch (const CLI::ParseError &e) {
        if (m_cliApp.exit(e) > 0) {
            return OptionResult::RESULT_ERROR;
        }

    }
    if (m_repositoryType == "artifactory" && m_apiKey.empty()) {
        cout << "Error : apiKey argument must be specified for artifactory repositories !"<<endl;
        return OptionResult::RESULT_ERROR;
    }
    initBuildConfig();
    return OptionResult::RESULT_SUCCESS;
}

void CmdOptions::writeConfigurationFile() const
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPath = PathBuilder::getHomePath() / Constants::REMAKEN_FOLDER;
    fs::path remakenProfilesPath = remakenRootPath / Constants::REMAKEN_PROFILES_FOLDER;
    fs::path remakenConfigPath = remakenRootPath/"config";
    ofstream fos;
    fos.open(remakenConfigPath.generic_string(utf8),ios::out|ios::trunc);
    fos<<m_cliApp.config_to_str(m_defaultProfileOptions,true);
    fos.close();
}


void CmdOptions::displayConfigurationSettings() const
{
    std::cout<<m_cliApp.config_to_str(m_defaultProfileOptions,true);
}

void CmdOptions::printUsage()
{
    cout << m_cliApp.help() <<endl;
}

void CmdOptions::display()
{
    cout<<"Action =>\t"<<m_action<<endl;
    cout<<"OS =>\t"<<m_os<<endl;
    cout<<"Architecture =>\t"<<m_architecture<<endl;
    cout<<"Mode =>\t"<<m_mode<<endl;
    cout<<"Config =>\t"<<m_config<<endl;
    cout<<"API Key =>\t"<<m_apiKey<<endl;
    cout<<"Repository type =>\t"<<m_repositoryType<<endl;
    cout<<"Destination directory =>\t"<<m_destinationRoot<<endl;
    cout<<"Destination directory path =>\t"<<m_destinationRootPath<<endl;
    cout<<"C++ version =>\t"<<m_cppVersion<<endl;
}
