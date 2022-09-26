#include "OsUtils.h"
#include "Constants.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#ifdef BOOST_OS_WINDOWS_AVAILABLE
#include <wbemidl.h>
#endif

bool OsUtils::isElevated()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return true;
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return true;
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return true;
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return true;
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return true;
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle( hToken );
    }
    return fRet;
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
    return true;
#endif
    return false;
}

static const std::map<const std::string_view,const  std::string_view> os2sharedSuffix = {
    {"mac",".dylib"},
    {"win",".dll"},
    {"unix",".so"},
    {"android",".so"},
    {"ios",".dylib"},
    {"linux",".so"}
};

static const std::map<const std::string_view,const  std::string_view> os2staticSuffix = {
    {"mac",".a"},
    {"win",".lib"},
    {"unix",".a"},
    {"android",".a"},
    {"ios",".a"},
    {"linux",".a"}
};

static const std::map<const std::string_view,const  std::string_view> os2sharedPathEnv = {
    {"mac","DYLD_LIBRARY_PATH"},
    {"win","PATH"},
    {"unix","LD_LIBRARY_PATH"},
    {"android","LD_LIBRARY_PATH"},
    {"ios","DYLD_LIBRARY_PATH"},
    {"linux","LD_LIBRARY_PATH"}
};

const std::string_view & OsUtils::sharedSuffix(const std::string_view & osStr)
{
    if (os2sharedSuffix.find(osStr) == os2sharedSuffix.end()) {
        return os2sharedSuffix.at("unix");
    }
    return os2sharedSuffix.at(osStr);
}

const std::string_view & OsUtils::sharedLibraryPathEnvName(const std::string_view & osStr)
{
    if (os2sharedPathEnv.find(osStr) == os2sharedPathEnv.end()) {
        return os2sharedPathEnv.at("unix");
    }
    return os2sharedPathEnv.at(osStr);
}



const std::string_view & OsUtils::staticSuffix(const std::string_view & osStr)
{
    if (os2staticSuffix.find(osStr) == os2staticSuffix.end()) {
        return os2staticSuffix.at("unix");
    }
    return os2staticSuffix.at(osStr);
}

void OsUtils::copyLibrary(const fs::path & sourceFile, const fs::path & destinationFolderPath, const std::string_view & suffix, bool overwrite)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path currentPath = sourceFile;
    fs::path fileSuffix;
    while (currentPath.has_extension() && fileSuffix.string(utf8) != suffix) {
        fileSuffix = currentPath.extension();
        currentPath = currentPath.stem();
    }
    if (fileSuffix.string(utf8) == suffix) {
        auto linkStatus = fs::symlink_status(sourceFile);
        if (linkStatus.type() == fs::symlink_file) {
            if (fs::is_symlink(destinationFolderPath/sourceFile.filename())) {
                fs::remove(destinationFolderPath/sourceFile.filename());
            }
            fs::copy_symlink(sourceFile, destinationFolderPath/sourceFile.filename());
        }
        else if (is_regular_file(sourceFile)) {
            if (!fs::exists(destinationFolderPath/sourceFile.filename()) || overwrite) {
                fs::copy_file(sourceFile , destinationFolderPath/sourceFile.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
    }
}

void OsUtils::copyLibraries(const fs::path & sourceRootFolder, const fs::path & destinationFolderPath, const std::string_view & suffix, bool overwrite)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::vector<fs::path> symlinkFiles;
    // first copy concrete libraries, store symlinks
    for (fs::directory_entry& x : fs::directory_iterator(sourceRootFolder)) {
        auto linkStatus = fs::symlink_status(x.path());
        if (linkStatus.type() == fs::symlink_file) {
            symlinkFiles.push_back(x.path());
        }
        else {
            copyLibrary(x.path(), destinationFolderPath, suffix, overwrite);
        }
    }
    // then copy symlinks
    for (auto & file: symlinkFiles) {
         copyLibrary(file, destinationFolderPath, suffix, overwrite);
    }
}

template <class T> void copyFolder(const fs::path & srcFolderPath, const fs::path & dstFolderPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    for ( const fs::directory_entry& x : T{srcFolderPath} ) {
        const auto& path = x.path();
        auto relativePathStr = path.generic_string(utf8);
        boost::algorithm::replace_first(relativePathStr, srcFolderPath.generic_string(utf8), "");
        fs::copy(path, dstFolderPath / relativePathStr,fs::copy_options::overwrite_existing);
    }
}

void OsUtils::copyFolder(const fs::path & srcFolderPath, const fs::path & dstFolderPath, bool bRecurse)
{

    if (!fs::exists(srcFolderPath) || !fs::is_directory(srcFolderPath))
    {
        throw std::runtime_error("Source directory " + srcFolderPath.string() + " does not exist or is not a directory");
    }
    if (!fs::exists(dstFolderPath))
    {
        BOOST_LOG_TRIVIAL(info)<<"==> creating directory "<<dstFolderPath;
        if (!fs::create_directory(dstFolderPath)) {
            throw std::runtime_error("Cannot create destination directory " + dstFolderPath.string());
        }
    }

    if (bRecurse) {
        ::copyFolder<fs::recursive_directory_iterator>(srcFolderPath, dstFolderPath);
    }
    else {
        ::copyFolder<fs::directory_iterator>(srcFolderPath, dstFolderPath);
    }
}


void OsUtils::copyLibraries(const fs::path & sourceRootFolder, const CmdOptions & options, std::function<const std::string_view &(const std::string_view &)> suffixFunction)
{
    fs::path destinationFolderPath = options.getDestinationRoot();
    if (options.isXpcfBundle()) {
        destinationFolderPath /= options.getModulesSubfolder();
    }
    copyLibraries(sourceRootFolder, destinationFolderPath, suffixFunction(options.getOS()), options.override());
}


void OsUtils::copySharedLibraries(const fs::path & sourceRootFolder, const CmdOptions & options)
{
    copyLibraries(sourceRootFolder, options, &sharedSuffix);
}


void OsUtils::copyStaticLibraries(const fs::path & sourceRootFolder, const CmdOptions & options)
{
    copyLibraries(sourceRootFolder, options, &staticSuffix);
}

fs::path OsUtils::computeRemakenRootPackageDir(const CmdOptions & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path osSubFolder(options.getOS() + "-" + options.getBuildToolchain() , utf8);
    fs::path remakenRootDir = options.getRemakenRoot()/osSubFolder;
    return remakenRootDir;
}


fs::path OsUtils::acquireTempFolderPath()
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    fs::path tmpDir(fs::temp_directory_path().generic_string(utf8));
    tmpDir /= boost::uuids::to_string(uuid);

    try {
        fs::create_directories(tmpDir);
    }
    catch (const fs::filesystem_error & e) {
        throw std::runtime_error("Unable to create working directory " + tmpDir.generic_string(utf8));
    }
    return tmpDir;
}

void OsUtils::releaseTempFolderPath(const fs::path & tmpDir)
{
    fs::detail::utf8_codecvt_facet utf8;
    try {
        for (fs::directory_entry& x : fs::directory_iterator(tmpDir)) {
            if (is_regular_file(x.path())) {
                fs::remove(x.path());
            }
        }
        fs::remove(tmpDir);
    }
    catch (const fs::filesystem_error & e) {
        throw std::runtime_error("Unable to cleanup working directory " + tmpDir.generic_string(utf8));
    }
}

fs::path OsUtils::extractPath(const fs::path & first, const fs::path & second)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (first.empty() || second.empty()) {
        return fs::path();
    }
    std::string firstStr = first.generic_string(utf8);
    std::string secondStr = second.generic_string(utf8);
    if (first.size() < second.size()) {
        firstStr =  second.generic_string(utf8);
        secondStr = first.generic_string(utf8);
    }
    secondStr += fs::path::separator;
    boost::algorithm::replace_first(firstStr, secondStr,"");
    return fs::path(firstStr,utf8);
}
