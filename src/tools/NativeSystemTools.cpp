#include "NativeSystemTools.h"
#include "utils/OsUtils.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <map>
#include <fstream>
using namespace std;

namespace bp = boost::process;

void NativeSystemTool::bundleScript ([[maybe_unused]] const Dependency & dependency, [[maybe_unused]] const fs::path & scriptFile)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string cmd = retrieveInstallCommand(dependency);

#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
    ofstream fos(scriptFile.generic_string(utf8),ios::out|ios::app);
    fos<<cmd<< '\n';
    fos.close();
#endif
}

void AptSystemTool::update()
{
    int result = bp::system(sudo(),m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating apt repositories");
    }
}

void AptSystemTool::listRemotes()
{
    fs::detail::utf8_codecvt_facet utf8;
    std::vector<std::string> remoteList;
    std::cout<<"Apt sources:"<<std::endl;
    remoteList = split( SystemTools::runShellCommand("grep", {"-Erh","^deb"},"/etc/apt/sources.list", {0,1}));
    for (const auto & remote: remoteList) {
         std::cout<<"=> "<<remote<<std::endl;
    }
    for ( const fs::directory_entry& x : fs::recursive_directory_iterator{"/etc/apt/sources.list.d"} ) {
        if (fs::is_regular_file(x)) {
            remoteList = split( SystemTools::runShellCommand("grep", {"-Erh","^deb"}, x.path().generic_string(utf8), {0,1}));
            for (const auto & remote: remoteList) {
                 std::cout<<"=> "<<remote<<std::endl;
            }
        }
    }
    /*boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    bp::system("/bin/grep", "-Erh","^deb", "/etc/apt/sources.list.d/*", bp::std_out > listOutputFut, ios);
    std::vector<std::string> remoteList = split( SystemTools::runShellCommand("grep", {"-Erh","^deb"}, "/etc/apt/sources.list*", {0,1}));
    for (const auto & remote: remoteList) {
         std::cout<<"=> "<<remote<<std::endl;
    }*/
}


void AptSystemTool::addPpaSource(const std::string & repositoryUrl)
{
    if (repositoryUrl.empty()) {
        return;
    }
    fs::detail::utf8_codecvt_facet utf8;
    std::string ppaList = SystemTools::runShellCommand ("grep", {"-Erh","^deb"}, "/etc/apt/sources.list", {0,1});
    for ( const fs::directory_entry& x : fs::recursive_directory_iterator{"/etc/apt/sources.list.d"} ) {
        if (fs::is_regular_file(x)) {
            ppaList += SystemTools::runShellCommand("grep", {"-Erh","^deb"}, x.path().generic_string(utf8), {0,1});
        }
    }
    if (ppaList.find(repositoryUrl) == std::string::npos) {
        std::cout<<"Adding ppa repository: "<<repositoryUrl<<std::endl;
        std::string result = runAsRoot("add-apt-repository", {"-y"}, repositoryUrl);
    }
}

void AptSystemTool::addRemote(const std::string & remoteReference)
{
    if (remoteReference != "system") {
        addPpaSource(remoteReference);
    }
}

void AptSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);

    addRemote(dependency.getBaseRepository());

    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());

    if (result != 0) {
        throw std::runtime_error("Error installing apt dependency : " + source);
    }
}

bool AptSystemTool::installed(const Dependency & dependency)
{
    fs::path dpkg = bp::search_path("dpkg-query");
    std::string source = computeToolRef(dependency);
    int result = bp::system(dpkg, "-W","-f='${Status}'", source.c_str(),"|","grep -q \"ok installed\"");
    return result == 0;
}

std::string AptSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " install -y " + source;
    return installCmd;
}

void AptSystemTool::search(const std::string & pkgName, const std::string & version)
{
    std::string package = pkgName;
    std::vector<std::string> foundDeps = split( SystemTools::runShellCommand ("apt-cache", "search", {}, package) );
    std::cout<<"Apt::search results:"<<std::endl;
    for (auto & dep : foundDeps) {
        std::vector<std::string> depDetails = split(dep,' ');
        std::cout<<dep<<"\t\t\t"<<depDetails.at(0)<<"||"<<depDetails.at(0)<<"|apt-get@system|"<<std::endl;
    }
}

void YumSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "update","-y");
    if (result != 0) {
        throw std::runtime_error("Error updating yum repositories");
    }
}

void YumSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install","-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing yum dependency : " + source);
    }
}

bool YumSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeToolRef(dependency);
    int result = bp::system(rpm, "-q",source.c_str());
    return result == 0;
}

std::string YumSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " install -y " + source;
    return installCmd;
}

void PacManSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "-Syyu","--noconfirm");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PacManSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "-S","--noconfirm", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing pacman dependency : " + source);
    }
}

bool PacManSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string PacManSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " -S --noconfirm " + source;
    return installCmd;
}

void PkgToolSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PkgToolSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "install", "-y", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing vcpkg dependency : " + source);
    }
}

bool PkgToolSystemTool::installed(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "info", source.c_str());
    return result == 0;
}

std::string PkgToolSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " install -y " + source;
    return installCmd;
}

void PkgUtilSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "--catalog");
    if (result != 0) {
        throw std::runtime_error("Error updating pacman repositories");
    }
}

void PkgUtilSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "--install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing pacman dependency : " + source);
    }
}

bool PkgUtilSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string PkgUtilSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " --install --yes " + source;
    return installCmd;
}

void ChocoSystemTool::update()
{
    int result = bp::system(m_systemInstallerPath, "outdated");
    if (result != 0) {
        throw std::runtime_error("Error updating choco repositories");
    }
}

void ChocoSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(m_systemInstallerPath, "install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing choco dependency : " + source);
    }
}

bool ChocoSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string ChocoSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = m_systemInstallerPath.generic_string(utf8);
    installCmd += " install","--yes " + source;
    return installCmd;
}

void ChocoSystemTool::search(const std::string & pkgName, const std::string & version)
{
    std::string package = pkgName;
    std::vector<std::string> foundDeps = split( run ("search", {"--by-id-only"}, package) );
    std::cout<<"Choco::search results:"<<std::endl;
    for (auto & dep : foundDeps) {
        if (dep.find("hocolatey") == std::string::npos && dep.find("packages found.") == std::string::npos &&
            dep.find("Did you know Pro") == std::string::npos && dep.find("Features? Learn more") == std::string::npos) {
            std::vector<std::string> depDetails = split(dep,' ');
            std::cout<<depDetails.at(0)<<"\t"<<depDetails.at(1)<<"\t\t"<<depDetails.at(0)<<"|"<<depDetails.at(1)<<"|choco|"<<std::endl;
        }
    }
}

void ScoopSystemTool::update()
{
    int result = bp::system(m_systemInstallerPath, "update");
    if (result != 0) {
        throw std::runtime_error("Error updating scoop repositories");
    }
}

void ScoopSystemTool::listRemotes()
{
    std::vector<std::string> remoteList = split( run ("bucket","list") );
    std::cout<<"Scoop remotes:"<<std::endl;
    for (const auto & remote: remoteList) {
         std::cout<<"=> "<<remote<<std::endl;
    }
}

void ScoopSystemTool::addRemote(const std::string & remoteReference)
{
    if (remoteReference.empty() || (remoteReference == "system")) {
        return;
    }
    std::string bucketList = run ("bucket","list");
    auto repoParts = split(remoteReference,'#');
    std::string repoId = remoteReference;
    std::vector<std::string> options;
    if (repoParts.size() < 2) {
        options.push_back(repoId);
    }
    if (repoParts.size() == 2) {
        repoId = repoParts.at(0);
        options.push_back(repoId);
        options.push_back(repoParts.at(1));
    }
    if (bucketList.find(repoId) == std::string::npos) {
        std::cout<<"Adding scoop bucket: "<<remoteReference<<std::endl;
        std::string result = run ("bucket","add",options);
    }
}

void ScoopSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    addRemote(dependency.getBaseRepository());
    int result = bp::system(m_systemInstallerPath, "install","--yes", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing scoop dependency : " + source);
    }
}

bool ScoopSystemTool::installed(const Dependency & dependency)
{
    return false;
}

std::string ScoopSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = m_systemInstallerPath.generic_string(utf8);
    installCmd += " install","--yes " + source;
    return installCmd;
}

void ScoopSystemTool::search(const std::string & pkgName, const std::string & version)
{
    std::string package = pkgName;
    std::vector<std::string> foundDeps = split( run ("search", {}, package) );
    std::cout<<"Scoop::search results:"<<std::endl;
    for (auto & dep : foundDeps) {
        if (dep.find("bucket:") == std::string::npos) {
            boost::erase_all(dep, "(");
            boost::erase_all(dep, ")");
            std::vector<std::string> depDetails = split(dep,' ');
            std::cout<<depDetails.at(0)<<"\t"<<depDetails.at(1)<<"\t\t"<<depDetails.at(0)<<"|"<<depDetails.at(1)<<"|scoop|"<<std::endl;
        }
    }
}

void ZypperSystemTool::update()
{
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","ref");
    if (result != 0) {
        throw std::runtime_error("Error updating zypper repositories");
    }
}

void ZypperSystemTool::install(const Dependency & dependency)
{
    std::string source = computeToolRef(dependency);
    int result = bp::system(sudo(), m_systemInstallerPath, "--non-interactive","in", source.c_str());
    if (result != 0) {
        throw std::runtime_error("Error installing zypper dependency : " + source);
    }
}

bool ZypperSystemTool::installed(const Dependency & dependency)
{
    fs::path rpm = bp::search_path("rpm");
    std::string source = computeToolRef(dependency);
    int result = bp::system(rpm, "-q",source.c_str());
    return result == 0;
}

std::string ZypperSystemTool::retrieveInstallCommand(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string source = computeToolRef(dependency);
    std::string installCmd = sudo().generic_string(utf8);
    installCmd += " " + m_systemInstallerPath.generic_string(utf8);
    installCmd += " --non-interactive in " + source;
    return installCmd;
}




