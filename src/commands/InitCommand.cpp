#include "InitCommand.h"
#include "managers/DependencyManager.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "utils/PathBuilder.h"
#include "retrievers/HttpFileRetriever.h"
#include "utils/OsUtils.h"
#include "utils/DepUtils.h"
#include "tools/GitTool.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>


namespace bp = boost::process;

InitCommand::InitCommand(const CmdOptions & options):AbstractCommand(InitCommand::NAME),m_options(options)
{
}

fs::path installArtefact(const CmdOptions & options, const std::string & source, const fs::path & outputDirectory)
{
    auto fileRetriever = std::make_shared<HttpFileRetriever>(options);
    fs::detail::utf8_codecvt_facet utf8;
    fs::path compressedDependency = fileRetriever->retrieveArtefact(source);
    if (!fs::exists(outputDirectory)) {
        fs::create_directories(outputDirectory);
    }
    std::shared_ptr<ZipTool> zipTool = ZipTool::createZipTool(options);
    int result = zipTool->uncompressArtefact(compressedDependency,outputDirectory);
    if (result != 0) {
        throw std::runtime_error("Error uncompressing artefact " + source);
    }
    fs::remove(compressedDependency);
    for (fs::directory_entry& x : fs::directory_iterator(outputDirectory)) {
        if (fs::is_directory(x.path())) {
            return x.path();
        }
    }
    return fs::path();
}

int setupVCPKG(const fs::path & remakenRootPackagesPath, const std::string & tag)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    GitTool tool;
    BOOST_LOG_TRIVIAL(info)<<"Installing vcpkg to [ "<<remakenRootPackagesPath.generic_string(utf8)<<"/vcpkg ]";
    result = tool.clone(Constants::VCPKG_REPOURL, remakenRootPackagesPath / "vcpkg", tag);
    if (result != 0) {
        return result;
    }
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    result = bp::system(remakenRootPackagesPath / "vcpkg" / "bootstrap-vcpkg.bat");
#else
    result = bp::system(remakenRootPackagesPath / "vcpkg" / "bootstrap-vcpkg.sh");
#endif
    return result;
}

#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
int setupBrew()
{
    int result = -1;
    result = bp::system("/bin/bash","-c","-fsSL","https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)");
    return result;
}
#endif

int setupArtifactPackager(const CmdOptions & options)
{
    fs::path remakenRootPath = PathBuilder::getHomePath(options) / Constants::REMAKEN_FOLDER;
    fs::path remakenScriptsPath = remakenRootPath / "scripts";

    if (!fs::exists(remakenScriptsPath)) {
        fs::create_directories(remakenScriptsPath);
    }

    std::string source="https://github.com/b-com-software-basis/remaken/releases/download/";
    std::string release = "artifactpackager";
    std::string tag = options.getArtifactPackagerTag();
    if (!tag.empty()) {
        release = tag;
    }
    source += release;
    source += "/";
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    std::string filename = "win_artiPackager.bat";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    std::string filename = "mac_artiPackager.sh";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    std::string filename = "unixes_artiPackager.sh";
#endif
    source +=filename;
    BOOST_LOG_TRIVIAL(info)<<"Installing ArtifactManager version ["<<release<<"]";
    //fs::path artefactFolder = installArtefact(options,source,remakenScriptsPath,false);
    fs::path artefactPath = DepUtils::downloadFile(options,source,remakenScriptsPath,filename);
    fs::permissions(artefactPath, fs::add_perms|fs::owner_exe|fs::group_exe|fs::others_exe);
    return 0;
}

int setupWizards(const fs::path & rulesPath, const CmdOptions & options)
{
    BOOST_LOG_TRIVIAL(info)<<"Installing qt creator wizards";
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    fs::path qtWizardsPath = PathBuilder::getUTF8PathObserver(getenv("APPDATA"));
#else
    fs::path qtWizardsPath = PathBuilder::getHomePath(options)/".config";
#endif
    qtWizardsPath /= "QtProject";
    qtWizardsPath /= "qtcreator";
    qtWizardsPath /= "templates";
    qtWizardsPath /= "wizards";
    if (!fs::exists(qtWizardsPath)) {
        BOOST_LOG_TRIVIAL(info)<<"==> creating directory "<<qtWizardsPath;
        fs::create_directories(qtWizardsPath);
    }
    if (!fs::exists(qtWizardsPath/"classes")) {
        BOOST_LOG_TRIVIAL(info)<<"==> creating directory "<<qtWizardsPath/"classes";
        fs::create_directory(qtWizardsPath/"classes");
    }
    if (!fs::exists(qtWizardsPath/"projects")) {
        BOOST_LOG_TRIVIAL(info)<<"==> creating directory "<<qtWizardsPath/"projects";
        fs::create_directory(qtWizardsPath/"projects");
    }
    OsUtils::copyFolder(rulesPath/"wizards"/"qtcreator"/"classes",qtWizardsPath/"classes",true);
    OsUtils::copyFolder(rulesPath/"wizards"/"qtcreator"/"projects",qtWizardsPath/"projects",true);
    return 0;
}

int InitCommand::execute()
{
    fs::detail::utf8_codecvt_facet utf8;

    if (!fs::exists(m_options.getRemakenRoot())) {
        fs::create_directories(m_options.getRemakenRoot());
    }
    auto subCommand = m_options.getSubcommand();
    if (subCommand == "vcpkg") {
        return setupVCPKG(m_options.getRemakenRoot(), m_options.getVcpkgTag());
    }

#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
    if (subCommand == "brew") {
        return setupBrew();
    }
#endif
    if (subCommand == "artifactpkg") {
        return setupArtifactPackager(m_options);
    }

    // process init command
    fs::path remakenRootPath = PathBuilder::getHomePath(m_options) / Constants::REMAKEN_FOLDER;
    fs::path remakenRulesPath = remakenRootPath / "rules";
    fs::path remakenProfilesPath = remakenRootPath / Constants::REMAKEN_PROFILES_FOLDER;
    fs::path qmakeRootPath = remakenRulesPath / "qmake";


    if (!fs::exists(remakenProfilesPath)) {
        fs::create_directories(remakenProfilesPath);
    }

    if (m_options.force()) {
        fs::remove(qmakeRootPath);
    }

    auto linkStatus = boost::filesystem::symlink_status(qmakeRootPath);
    if (linkStatus.type() != fs::file_not_found || fs::exists(qmakeRootPath)) {
        BOOST_LOG_TRIVIAL(info)<<"qmake rules already installed ! skipping...";
    }
    else {
        if (!fs::exists(remakenRulesPath)) {
            fs::create_directories(remakenRulesPath);
        }
        std::string source="https://github.com/b-com-software-basis/builddefs-qmake/releases/download/";
        std::string release = m_options.getQmakeRulesTag();
        if (release == "latest") {
            release = "builddefs-qmake-latest";
        }
        source += release;
        source += "/builddefs-qmake-package.zip";
        BOOST_LOG_TRIVIAL(info)<<"Installing qmake rules version ["<<release<<"]";
        fs::path artefactFolder = installArtefact(m_options,source,remakenRulesPath);

        if (fs::exists(artefactFolder)) {
            fs::create_symlink(artefactFolder.filename(),qmakeRootPath);
        }
    }

    if (m_options.installWizards() || subCommand == "wizards") {
        setupWizards(qmakeRootPath, m_options);
    }
    return 0;
}
