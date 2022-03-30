#include "AbstractFileRetriever.h"
#include "utils/DepUtils.h"
#include "utils/OsUtils.h"
#include "tools/PkgConfigTool.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>

AbstractFileRetriever::AbstractFileRetriever(const CmdOptions & options):m_options(options)
{
    m_workingDirectory = OsUtils::acquireTempFolderPath();
    m_zipTool = ZipTool::createZipTool(m_options);
}

AbstractFileRetriever::~AbstractFileRetriever()
{
    OsUtils::releaseTempFolderPath(m_workingDirectory);
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


std::pair<std::string, fs::path> AbstractFileRetriever::invokeGenerator(std::vector<Dependency> & deps)
{
    fs::detail::utf8_codecvt_facet utf8;
    PkgConfigTool pkgConfig(m_options);
    // call pkg-config on dep and populate libs and cflags variables
    // TODO:check pkg exists from pkgconfig ?
    std::vector<std::string> cflags, libs;
    for ( auto & dep : deps) {
        std::cout<<"==> Adding '"<<dep.getName()<<":"<<dep.getVersion()<<"' dependency"<<std::endl;
        fs::path prefix = computeLocalDependencyRootDir(dep);
        fs::path libdir = computeRootLibDir(dep);
        dep.prefix() = prefix.generic_string(utf8);
        dep.libdirs().push_back(libdir.generic_string(utf8));
        pkgConfig.addPath(prefix);
        std::string prefixOpt = "--define-variable=prefix=" + dep.prefix();
        std::string libdirOpt = "--define-variable=libdir=" + libdir.generic_string(utf8);
        m_options.verboseMessage("===> using prefix: " + prefixOpt);
        m_options.verboseMessage("===> using libdir: " + libdirOpt);

        pkgConfig.cflags(dep, {prefixOpt, libdirOpt});
        pkgConfig.libs(dep, {prefixOpt, libdirOpt});
    }

    // format CFLAGS and LIBS results
    return pkgConfig.generate(deps,Dependency::Type::REMAKEN);
}

fs::path AbstractFileRetriever::installArtefactImpl(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path compressedDependency = retrieveArtefact(dependency);
    // zipper::Unzipper unzipper(compressedDependency.generic_string(utf8));
    fs::path outputDirectory = OsUtils::computeRemakenRootPackageDir(m_options);
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
    // unzipper.extract(outputDirectory.generic_string(utf8));
    // unzipper.close();
    fs::remove(compressedDependency);
    outputDirectory = computeLocalDependencyRootDir(dependency);
    if (!fs::exists(outputDirectory)) {
        throw std::runtime_error("Error : dependency folder " + outputDirectory.generic_string(utf8) + " doesn't exist after package unzip");
    }
    return outputDirectory;
}

void AbstractFileRetriever::copySharedLibraries(const fs::path & sourceRootFolder)
{
    OsUtils::copySharedLibraries(sourceRootFolder, m_options);
}

fs::path AbstractFileRetriever::bundleArtefact(const Dependency & dependency)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path rootLibDir = computeRootLibDir(dependency);
    if (!fs::exists(rootLibDir)) { // ignoring header only dependencies
        BOOST_LOG_TRIVIAL(warning)<<"Ignoring "<<dependency.getName()<<" dependency: no shared library found from "<<rootLibDir;
        return "";
    }
    m_options.verboseMessage("--------------- Remaken bundle ---------------");
    m_options.verboseMessage("===> bundling: " + dependency.getName() + "/"+ dependency.getVersion() + " from " + rootLibDir.generic_string(utf8));
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
    fs::path depFullPath = OsUtils::computeRemakenRootPackageDir(m_options);
    fs::path depSubPath(dependency.getPackageName() , utf8);
    depSubPath /= dependency.getVersion();
    if (dependency.hasIdentifier()) {
        depFullPath /= dependency.getIdentifier();
    }
    depFullPath /= depSubPath;
    return depFullPath;
}
