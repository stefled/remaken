#include "AbstractFileRetriever.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

AbstractFileRetriever::AbstractFileRetriever(const CmdOptions & options):m_options(options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    fs::path tmpDir(fs::temp_directory_path().generic_string(utf8));
    m_workingDirectory = tmpDir / boost::uuids::to_string(uuid);
    m_zipTool = ZipTool::createZipTool(m_options);

    try {
        fs::create_directories(m_workingDirectory);
    }
    catch (const fs::filesystem_error & e) {
        throw std::runtime_error("Unable to create working directory " + m_workingDirectory.generic_string());
    }
}

AbstractFileRetriever::~AbstractFileRetriever()
{
    cleanUpWorkingDirectory();
}

void AbstractFileRetriever::cleanUpWorkingDirectory()
{
    try {
        for (fs::directory_entry& x : fs::directory_iterator(m_workingDirectory)) {
            if (is_regular_file(x.path())) {
                fs::remove(x.path());
            }
        }
        fs::remove(m_workingDirectory);
    }
    catch (const fs::filesystem_error & e) {
        throw std::runtime_error("Unable to cleanup working directory " + m_workingDirectory.generic_string());
    }
}

std::string AbstractFileRetriever::computeSourcePath( const Dependency &  dependency)
{
    /*
          Package sample name : curl_1.5.3_i386_static_release.zip
          Abstract naming : PACKAGENAME_VERSION_ARCHITECTURE_MODE_CONFIG.zip

           |- curl
              |- version
                   |- win
                   |- mac
                   |- unix
       */
    std::string sourcePath = dependency.getBaseRepository();
    sourcePath += "/" + dependency.getPackageName();
    sourcePath += "/" + dependency.getVersion();
    sourcePath += "/" + m_options.getOS();
    sourcePath += "/" + dependency.getPackageName();
    sourcePath += "_" + dependency.getVersion();
    sourcePath += "_" + m_options.getArchitecture();
    sourcePath += "_" + dependency.getMode();
    sourcePath += "_"+ m_options.getConfig();
    sourcePath += ".zip";
    return sourcePath;
}

fs::path AbstractFileRetriever::computeRootLibDir( const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path libPath = computeLocalDepencyRootDir(dependency);
    libPath = libPath / "lib" / m_options.getArchitecture() / dependency.getMode() / m_options.getConfig();
    return libPath;
}

fs::path AbstractFileRetriever::installArtefact(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path compressedDependency = retrieveArtefact(dependency);
    //zipper::Unzipper unzipper(compressedDependency.generic_string(utf8));
    fs::path outputDirectory = this->computeRemakenRootDir(dependency);
    fs::path osSubFolder(m_options.getOS() + "-" + m_options.getBuildToolchain() , utf8);
    outputDirectory /= osSubFolder;
    if (!fs::exists(outputDirectory)) {
        fs::create_directories(outputDirectory);
    }
    int result = m_zipTool->uncompressArtefact(compressedDependency,outputDirectory);
    if (result != 0) {
        throw std::runtime_error("Error uncompressing dependency " + dependency.getName());
    }
    //unzipper.extract(outputDirectory.generic_string(utf8));
    //unzipper.close();
    fs::remove(compressedDependency);
    return outputDirectory;
}

fs::path AbstractFileRetriever::computeLocalDepencyRootDir( const Dependency &  dependency) // not the root output dir
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path libPath = computeRemakenRootDir(dependency);
    fs::path osSubFolder(m_options.getOS() + "-" + m_options.getBuildToolchain() , utf8);
    fs::path depSubPath(dependency.getPackageName() , utf8);
    depSubPath /= dependency.getVersion();

    if (fs::exists(libPath/osSubFolder/depSubPath)) {
        libPath /= osSubFolder;
    }
    libPath /= depSubPath;
    return libPath;
}

fs::path AbstractFileRetriever::computeRemakenRootDir( const Dependency &  dependency) // not the root output dir
{
    if (dependency.hasIdentifier()) {
        return m_options.getRemakenRoot() / dependency.getIdentifier();
    }
    return m_options.getRemakenRoot();
}
