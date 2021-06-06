#include "AbstractFileRetriever.h"
#include "utils/OsTools.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>

AbstractFileRetriever::AbstractFileRetriever(const CmdOptions & options):m_options(options)
{
    m_workingDirectory = OsTools::acquireTempFolderPath();
    m_zipTool = ZipTool::createZipTool(m_options);
}

AbstractFileRetriever::~AbstractFileRetriever()
{
    OsTools::releaseTempFolderPath(m_workingDirectory);
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
    fs::path libPath = computeLocalDependencyRootDir(dependency);
    libPath = libPath / "lib" / m_options.getArchitecture() / dependency.getMode() / m_options.getConfig();
    return libPath;
}

fs::path AbstractFileRetriever::computeRootBinDir( const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path binPath = computeLocalDependencyRootDir(dependency);
    binPath = binPath / "bin" / m_options.getArchitecture() / dependency.getMode() / m_options.getConfig();
    return binPath;
}


fs::path AbstractFileRetriever::installArtefact(const Dependency & dependency)
{
    fs::path folder = installArtefactImpl(dependency);
    m_installedDeps.push_back(dependency);
    return folder;
}


fs::path AbstractFileRetriever::invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator)
{
    return fs::path();
}

fs::path AbstractFileRetriever::installArtefactImpl(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path compressedDependency = retrieveArtefact(dependency);
    //zipper::Unzipper unzipper(compressedDependency.generic_string(utf8));
    fs::path outputDirectory = OsTools::computeRemakenRootPackageDir(m_options);
    if (dependency.hasIdentifier()) {
        outputDirectory /= dependency.getIdentifier();
    }
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
    outputDirectory = computeLocalDependencyRootDir(dependency);
    if (!fs::exists(outputDirectory)) {
        throw std::runtime_error("Error : dependency folder " + outputDirectory.generic_string(utf8) + " doesn't exist after package unzip");
    }
    return outputDirectory;
}

void AbstractFileRetriever::copySharedLibraries(const fs::path & sourceRootFolder)
{
    OsTools::copySharedLibraries(sourceRootFolder, m_options);
}

fs::path AbstractFileRetriever::bundleArtefact(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path rootLibDir = computeRootLibDir(dependency);
    if (!fs::exists(rootLibDir)) { // ignoring header only dependencies
        BOOST_LOG_TRIVIAL(warning)<<"Ignoring "<<dependency.getName()<<" dependency: no shared library found from "<<rootLibDir;
        return "";
    }
    copySharedLibraries(rootLibDir);
    fs::path outputDirectory = computeLocalDependencyRootDir(dependency);
    return outputDirectory;
}

std::vector<fs::path> AbstractFileRetriever::binPaths(const Dependency & dependency)
{
    std::vector<fs::path> paths;
    fs::detail::utf8_codecvt_facet utf8;
    paths.push_back(computeRootBinDir(dependency));
    return paths;
}

std::vector<fs::path> AbstractFileRetriever::libPaths(const Dependency & dependency)
{
    std::vector<fs::path> paths;
    fs::detail::utf8_codecvt_facet utf8;
    paths.push_back(computeRootLibDir(dependency));
    return paths;
}


fs::path AbstractFileRetriever::computeLocalDependencyRootDir( const Dependency &  dependency) // not the root output dir
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path depFullPath = OsTools::computeRemakenRootPackageDir(m_options);
    fs::path depSubPath(dependency.getPackageName() , utf8);
    depSubPath /= dependency.getVersion();
    if (dependency.hasIdentifier()) {
        depFullPath /= dependency.getIdentifier();
    }
    depFullPath /= depSubPath;
    return depFullPath;
}
