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
    m_cliApp.add_option("--architecture,-a",m_architecture, "Architecture: x86_64, i386, arm, arm64, armv6, armv7, armv7hf, armv8",true);
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

    CLI::App * initCommand = m_cliApp.add_subcommand("init","initialize remaken root folder and retrieve qmake rules");

    CLI::App * installCommand = m_cliApp.add_subcommand("install","install dependencies for a package from its packagedependencies file(s)");
    installCommand->add_option("--alternate-remote-type,-l", m_altRepoType, "alternate remote type: github, artifactory, nexus, path");
    installCommand->add_option("--alternate-remote-url,-u", m_altRepoUrl, "alternate remote url to use when the declared remote fails to provide a dependency");
    installCommand->add_option("--apiKey,-k", m_apiKey, "Artifactory api key");
    installCommand->add_option("file", m_dependenciesFile, "Remaken dependencies files", true);

    m_ignoreCache = false;
    installCommand->add_flag("--ignore-cache,-i", m_ignoreCache, "ignore cache entries : dependencies update is forced");
    m_mode = "shared";
    installCommand->add_option("--mode,-m", m_mode, "Mode: shared, static", true);
    m_repositoryType = "github";
    installCommand->add_option("--type,-t", m_repositoryType, "Repository type: github, artifactory, nexus, path", true);
    m_zipTool = ZipTool::getZipToolIdentifier();
    installCommand->add_option("--ziptool,-z", m_zipTool, "unzipper tool name : unzip, compact ...", true);

    CLI::App * parseCommand = m_cliApp.add_subcommand("parse","check dependency file validity");
    parseCommand->add_option("file", m_dependenciesFile, "Remaken dependencies files", true);
}


static const map<std::string,std::vector<std::string>> validationMap ={{"action",{"install","parse","version","bundle", "bundleXpcf"}},
                                                                       {"--architecture",{"x86_64","i386","arm","arm64","armv6","armv7","armv7hf","armv8"}},
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
