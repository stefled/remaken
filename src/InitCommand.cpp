#include "InitCommand.h"
#include "DependencyManager.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "PathBuilder.h"
#include "HttpFileRetriever.h"
#include "OsTools.h"
#include <boost/log/trivial.hpp>

InitCommand::InitCommand(const CmdOptions & options):AbstractCommand(InitCommand::NAME),m_options(options)
{
}

fs::path installArtefact(const CmdOptions & options, const std::string & source, const fs::path & outputDirectory, const std::string & name = "" )
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

int InitCommand::execute()
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPath = PathBuilder::getHomePath() / Constants::REMAKEN_FOLDER;
    fs::path remakenRulesPath = remakenRootPath / "rules";
    fs::path remakenProfilesPath = remakenRootPath / Constants::REMAKEN_PROFILES_FOLDER;
    fs::path qmakeRootPath = remakenRootPath / "qmake";

    auto linkStatus = boost::filesystem::symlink_status(qmakeRootPath);
    if (fs::exists(qmakeRootPath) || linkStatus.type() != fs::file_not_found) {
        BOOST_LOG_TRIVIAL(info)<<"qmake rules already installed ! skipping...";
        return 0;
    }
    if (!fs::exists(remakenRulesPath)) {
        fs::create_directories(remakenRulesPath);
    }
    if (!fs::exists(remakenProfilesPath)) {
        fs::create_directory(remakenProfilesPath);
    }
    std::string source="https://github.com/b-com-software-basis/builddefs-qmake/archive/github_v4.3.0.zip";
    fs::path artefactFolder = installArtefact(m_options,source,remakenRulesPath);

    if (fs::exists(artefactFolder)) {
        fs::create_symlink(artefactFolder,qmakeRootPath);
    }
    return 0;
}
