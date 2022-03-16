/**
 * @copyright Copyright (c) 2019 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Loïc Touraine
 *
 * @file
 * @brief description of file
 * @date 2019-11-15
 */

#ifndef CMDOPTIONS_H
#define CMDOPTIONS_H

#include <string>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <libs/CLI11/include/CLI/CLI.hpp>
#include "Constants.h"

//remaken install -r  path_to_remaken_root -i -o linux -t github -l nexus -u http://url_to_root_nexus_repo --cpp-std 17 -c debug -- packagedependencies-github.txt

//remaken bundle -d ~/tmp/conanDeployed/ --cpp-std 17 -c debug [-- packagedependencies.txt]

//remaken bundleXpcf -d ~/tmp/conanDeployed/ -s relative_install_path_to_modules_folder --cpp-std 17 -c debug -- xpcfApplication.xml
namespace fs = boost::filesystem;

class CmdOptions
{
public:
    typedef enum {
        RESULT_HELP = 1,
        RESULT_SUCCESS = 0,
        RESULT_ERROR = -1
     } OptionResult;

    CmdOptions();
    OptionResult parseArguments(int argc, char** argv);
    void printUsage();
    void display();

    const std::string & getAction() const {
        return m_action;
    }

    const std::string & getDependenciesFile() const {
        return m_dependenciesFile;
    }

    const std::string & getApplicationFile() const {
        return m_applicationFile;
    }

    const std::vector<std::string> & getApplicationArguments() const {
        return m_applicationArguments;
    }

    const std::string & getXpcfXmlFile() const {
        return m_xpcfConfigurationFile;
    }

    const std::string & getArchitecture() const {
        return m_architecture;
    }

    const std::string & getMode() const {
        return m_mode;
    }

    const std::string & getConfig() const {
        return m_config;
    }

    const std::string & getApiKey() const {
        return m_apiKey;
    }

    const std::string & getOS() const {
        return m_os;
    }

    const fs::path & getDestinationRoot() const {
        return m_destinationRootPath;
    }

    fs::path getBundleDestinationRoot() const {
        fs::path destinationFolderPath = m_destinationRootPath;
        if (m_isXpcfBundle) {
            destinationFolderPath /= m_moduleSubfolderPath;
        }
        return destinationFolderPath;
    }

    const fs::path & getRemakenRoot() const {
        return m_remakenRootPath;
    }

    const std::string & getRepositoryType() const {
        return m_repositoryType;
    }

    const std::string & getCppVersion() const {
        return m_cppVersion;
    }

    const std::string & getBuildToolchain() const {
        return m_toolchain;
    }

    const std::string & getZipTool() const {
        return m_zipTool;
    }

    bool getVerbose() const {
        return m_verbose;
    }

    const std::string & getAlternateRepoType() const {
        return m_altRepoType;
    }

    const std::string & getAlternateRepoUrl() const {
        return m_altRepoUrl;
    }

    const fs::path & getModulesSubfolder() const {
        return m_moduleSubfolderPath;
    }

    bool invertRepositoryOrder () const {
        return m_invertRepositoryOrder;
    }

    bool useCache() const {
        return !m_ignoreCache;
    }

    bool environmentOnly() const {
        return m_environment;
    }

    bool isXpcfBundle() const {
        return m_isXpcfBundle;
    }

    bool cleanAllEnabled() const {
        return m_cleanAll;
    }

    bool override() const {
        return m_override;
    }

    bool recurse() const {
        return m_recurse;
    }

    bool force() const {
        return m_force;
    }

    bool regexEnabled() const {
        return m_regex;
    }

    bool treeEnabled() const {
        return m_tree;
    }

    bool projectModeEnabled() const;

    bool crossCompiling() const {
        return m_crossCompile;
    }

    bool installWizards() const {
        return m_installWizards;
    }

    const std::string & getBuildConfig() const {
        return m_buildConfig;
    }
    
    const std::string & getSubcommand() const {
        return m_subcommand;
    }

    const std::string & getQmakeRulesTag() const {
        return m_qmakeRulesTag;
    }

    const std::string & getVcpkgTag() const {
        return m_vcpkgTag;
    }

    const std::string & getConanProfile() const {
        return m_conanProfile;
    }

    void setProjectRootPath(const fs::path & projectPath) const {
        m_projectRootPath = projectPath;
    }

    const fs::path & getProjectRootPath() const  {
        return m_projectRootPath;
    }

    const std::string & getRemakenPkgRef() const  {
        return m_remakenPackageRef;
    }

    const std::map<std::string,std::string> & getCompressCommandOptions() const {
        return m_packageCompressOptions;
    }

    const std::map<std::string,std::string> & getSearchCommandOptions() const {
        return m_searchOptions;
    }

    const std::map<std::string,std::string> & getListCommandOptions() const {
        return m_listOptions;
    }

    void verboseMessage(const std::string & msg) const {
        if (m_verbose) {
            std::cout<<msg<<std::endl;
        }
    }

    GeneratorType getGenerator() const;

    std::string getGeneratorFileExtension() const;

    std::string getGeneratorFilePath(const std::string & file) const;

    void writeConfigurationFile(const std::string & profileName = "default") const;
    void displayConfigurationSettings() const;

private:
    std::string getOptionString(const std::string & optionName);
    void validateOptions();
    void initBuildConfig();
    std::string m_action;
    std::string m_applicationFile;
    std::string m_dependenciesFile;
    std::string m_xpcfConfigurationFile;
    std::string m_architecture;
    std::string m_mode;
    std::string m_config;
    std::string m_apiKey;
    std::string m_os;
    std::string m_destinationRoot;
    std::string m_remakenRoot;
    fs::path m_destinationRootPath;
    fs::path m_remakenRootPath;
    mutable fs::path m_projectRootPath;
    std::string m_repositoryType;
    std::string m_cppVersion;
    std::string m_toolchain;
    std::string m_zipTool;
    std::string m_altRepoUrl;
    std::string m_altRepoType;
    std::string m_moduleSubfolder;
    std::map<std::string,std::string> m_packageOptions;
    std::map<std::string,std::string> m_packageCompressOptions;
    std::map<std::string,std::string> m_listOptions;
    std::map<std::string,std::string> m_searchOptions;
    std::vector<std::string> m_applicationArguments;
    std::string m_buildConfig;
    std::string m_subcommand;
    std::string m_qmakeRulesTag;
    std::string m_vcpkgTag = "";
    std::string m_conanProfile = "default";
    std::string m_profileName = "default";
    fs::path m_moduleSubfolderPath;
    std::string m_generator = "qmake";
    std::string m_remakenPackageRef;
    bool m_ignoreCache;
    bool m_invertRepositoryOrder = false;
    bool m_verbose;
    bool m_recurse;
    bool m_regex = false;
    bool m_tree = false;
    bool m_environment = false;
    mutable bool m_projectMode = false;
    bool m_isXpcfBundle = false;
    bool m_cleanAll = true;
    bool m_force = false;
    bool m_override = false;
    bool m_defaultProfileOptions = false;
    bool m_crossCompile = false;
    bool m_installWizards = false;
    CLI::App m_cliApp{"remaken"};
};

#endif // CMDOPTIONS_H
