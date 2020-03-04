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
    m_optionsDesc.add_options()
            ("help,h", "produce help message")
            ("action", po::value<string>(&m_action)->default_value("install"), "Action to perform : install, parse (to check dependency file validity), "
                                                                               "bundle (to copy shared libraries deps in a destination folder), "
                                                                               "bundleXpcf (to copy xpcf modules and their dependencies from an xpcf xml file),"
                                                                               "version")
            ("architecture,a", po::value<string>(&m_architecture)->default_value("x86_64"), "Architecture: x86_64, i386, arm, arm64")
            ("apiKey,k", po::value<string>(&m_apiKey), "Artifactory api key")
            ("config,c", po::value<string>(&m_config)->default_value("release"), "Config: release, debug")
            ("destination,d", po::value<string>(&m_destinationRoot), "Destination directory")
            ("remaken-root,r", po::value<string>(&m_remakenRoot)->default_value(remakenRootPath.generic_string(utf8)), "Remaken root directory")
            ("file,f", po::value<string>(&m_dependenciesFile)->default_value("packagedependencies.txt"), "Dependencies files")
            ("mode,m", po::value<string>(&m_mode)->default_value("shared"), "Mode: shared, static")
            ("type,t", po::value<string>(&m_repositoryType)->default_value("github"), "Repository type: github, artifactory, nexus, path")
            ("build-toolchain,b", po::value<string>(&m_toolchain)->default_value(computeToolChain()), "Build toolchain: clang, clang-version, "
                                                                                                      "gcc-version, cl-version .. ex: cl-14.1")
            ("operating-system,o", po::value<string>(&m_os)->default_value(computeOS()), "Operating system: mac, win, unix, ios, android")
            ("cpp-std", po::value<string>(&m_cppVersion)->default_value("11"), "c++ standard version: 11, 14, 17, 20 ...")
            ("alternate-remote-type,l", po::value<string>(&m_altRepoType), "alternate remote type: github, artifactory, nexus, path")
            ("alternate-remote-url,u", po::value<string>(&m_altRepoUrl), "alternate remote url to use when the declared remote fails to provide a dependency")
            ("ignore-cache,i", po::bool_switch(&m_ignoreCache)->default_value(false), "ignore cache entries : dependencies update is forced")
            ("ziptool,z", po::value<string>(&m_zipTool)->default_value(ZipTool::getZipToolIdentifier()), "unzipper tool name : unzip, compact ...")
            ("modules-subfolder,s", po::value<string>(&m_moduleSubfolder)->default_value("modules"), "relative folder where XPCF modules will be "
                                                                                                     "copied with their dependencies")
            ("verbose,v", po::bool_switch(&m_verbose), "verbose mode")
            ;
}


static const map<std::string,std::vector<std::string>> validationMap ={{"action",{"install","parse","version","bundle", "bundleXpcf"}},
                                                                       {"architecture",{"x86_64","i386"}},
                                                                       {"config",{"release","debug"}},
                                                                       {"mode",{"shared","static"}},
                                                                       {"type",{"github","artifactory","nexus","path"}},
                                                                       {"alternate-remote-type",{"github","artifactory","nexus","path"}},
                                                                       {"operating-system",{"mac","win","unix","android","ios","linux"}},
                                                                       {"cpp-std",{"11","14","17","20"}}
                                                                      };

void CmdOptions::validateOptions()
{
    for (auto [name,value] : m_optionsVars) {
        if (validationMap.find(name) != validationMap.end()) {
            auto && validValues = validationMap.at(name);
            if (std::find(validValues.begin(), validValues.end(), value.as<string>()) == validValues.end()) {
                string message("Option " + name);
                message += " was set with invalid value " ;
                message += value.as<string>();
                throw std::runtime_error(message);
            }
        }
        if (name == "action") {
            if (value.as<string>() == "bundleXpcf") {
                m_isXpcfBundle = true;
            }
        }
    }
    fs::path zipToolPath = bp::search_path(m_optionsVars["ziptool"].as<string>()); //or get it from somewhere else.
    if (zipToolPath.empty()) {
        throw std::runtime_error("Error : " + m_optionsVars["ziptool"].as<string>() + " command not found on the system. Please install it first.");
    }
}

CmdOptions::OptionResult CmdOptions::parseArguments(int argc, char** argv)
{
    try {
        po::positional_options_description p;
        p.add("action", 1);

        po::store(po::command_line_parser(argc, argv).
                  options(m_optionsDesc).positional(p).run(), m_optionsVars);

        fs::path configPath = PathBuilder::getHomePath() / Constants::REMAKEN_FOLDER / "config";

        if (fs::exists(configPath)) {
            ifstream configFile(configPath.generic_string());
            po::store(po::parse_config_file(configFile,m_optionsDesc), m_optionsVars);
        }

        po::notify(m_optionsVars);
        validateOptions();
        fs::detail::utf8_codecvt_facet utf8;
        // path is assigned after to ensure utf8 codecvt
        m_destinationRootPath.assign(m_destinationRoot,utf8);
        m_remakenRootPath.assign(m_remakenRoot,utf8);
        m_moduleSubfolderPath.assign(m_moduleSubfolder,utf8);
    }
    catch(exception& e) {
        cout << "Error : " <<e.what() << endl;
    }
    if (m_optionsVars.count("help")) {
        printUsage();
        return OptionResult::RESULT_HELP;
    }
    if (m_repositoryType == "artifactory" && !m_optionsVars.count("apiKey")) {
        cout << "Error : apiKey argument must be specified for artifactory repositories !"<<endl;
        return OptionResult::RESULT_ERROR;
    }
    return OptionResult::RESULT_SUCCESS;
}

void CmdOptions::printUsage()
{
    cout << m_optionsDesc <<endl;
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
