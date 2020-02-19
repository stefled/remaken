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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>

//--action install -r  path_to_remaken_root -i -o linux -t github -l nexus -u http://url_to_root_nexus_repo --cpp-std 17 -c debug -f packagedependencies-github.txt

//--action bundle -d ~/tmp/conanDeployed/ --cpp-std 17 -c debug [-f packagedependencies.txt]

//--action bundleXpcf -d ~/tmp/conanDeployed/ -s relative_install_path_to_modules_folder --cpp-std 17 -c debug -f xpcfApplication.xml
namespace po = boost::program_options;
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

    inline const std::string & getAction() const {
        return m_action;
    }

    inline const std::string & getDependenciesFile() const {
        return m_dependenciesFile;
    }

    inline const std::string & getArchitecture() const {
        return m_architecture;
    }

    inline const std::string & getMode() const {
        return m_mode;
    }

    inline const std::string & getConfig() const {
        return m_config;
    }

    inline const std::string & getApiKey() const {
        return m_apiKey;
    }

    inline const std::string & getOS() const {
        return m_os;
    }

    inline const fs::path & getDestinationRoot() const {
        return m_destinationRootPath;
    }

    inline const fs::path & getRemakenRoot() const {
        return m_remakenRootPath;
    }

    inline const std::string & getRepositoryType() const {
        return m_repositoryType;
    }

    inline const std::string & getCppVersion() const {
        return m_cppVersion;
    }

    inline const std::string & getBuildToolchain() const {
        return m_toolchain;
    }

    inline const std::string & getZipTool() const {
        return m_zipTool;
    }

    inline bool getVerbose() const {
        return m_verbose;
    }

    inline const std::string & getAlternateRepoType() const {
        return m_altRepoType;
    }

    inline const std::string & getAlternateRepoUrl() const {
        return m_altRepoUrl;
    }

    inline bool useCache() const {
        return !m_ignoreCache;
    }

private:
    void validateOptions();
    std::string m_action;
    std::string m_dependenciesFile;
    std::string m_architecture;
    std::string m_mode;
    std::string m_config;
    std::string m_apiKey;
    std::string m_os;
    std::string m_destinationRoot;
    std::string m_remakenRoot;
    fs::path m_destinationRootPath;
    fs::path m_remakenRootPath;
    std::string m_repositoryType;
    std::string m_cppVersion;
    std::string m_toolchain;
    std::string m_zipTool;
    std::string m_altRepoUrl;
    std::string m_altRepoType;
    std::string m_moduleSubfolder;
    bool m_ignoreCache;
    bool m_verbose;

    po::options_description m_optionsDesc{"Usage"};
    po::variables_map m_optionsVars;
};

#endif // CMDOPTIONS_H
