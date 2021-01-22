#include "OsTools.h"
#include "Constants.h"

#ifdef BOOST_OS_WINDOWS_AVAILABLE
#include <wbemidl.h>
#endif

bool OsTools::isElevated()
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

const std::string_view & OsTools::sharedSuffix(const std::string_view & osStr)
{
    if (os2sharedSuffix.find(osStr) == os2sharedSuffix.end()) {
        return os2sharedSuffix.at("unix");
    }
    return os2sharedSuffix.at(osStr);
}


const std::string_view & OsTools::staticSuffix(const std::string_view & osStr)
{
    if (os2staticSuffix.find(osStr) == os2staticSuffix.end()) {
        return os2staticSuffix.at("unix");
    }
    return os2staticSuffix.at(osStr);
}


void OsTools::copyLibraries(const fs::path & sourceRootFolder, const fs::path & destinationFolderPath, const std::string_view & suffix)
{
    fs::detail::utf8_codecvt_facet utf8;
    for (fs::directory_entry& x : fs::directory_iterator(sourceRootFolder)) {
        fs::path filepath = x.path();
        auto linkStatus = fs::symlink_status(x.path());
        if (linkStatus.type() == fs::symlink_file) {
            if (fs::exists(destinationFolderPath/filepath.filename())) {
                fs::remove(destinationFolderPath/filepath.filename());
            }
            fs::copy_symlink(x.path(),destinationFolderPath/filepath.filename());
        }
        else if (is_regular_file(filepath)) {
            fs::path currentPath = filepath;
            fs::path fileSuffix;
            while (currentPath.has_extension() && fileSuffix.string(utf8) != suffix) {
                fileSuffix = currentPath.extension();
                currentPath = currentPath.stem();
            }
            if (fileSuffix.string(utf8) == suffix) {
                fs::copy_file(filepath , destinationFolderPath/filepath.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
    }
}


void OsTools::copyLibraries(const fs::path & sourceRootFolder, const CmdOptions & options, std::function<const std::string_view &(const std::string_view &)> suffixFunction)
{
    fs::path destinationFolderPath = options.getDestinationRoot();
    if (options.isXpcfBundle()) {
        destinationFolderPath /= options.getModulesSubfolder();
    }
    copyLibraries(sourceRootFolder, destinationFolderPath, suffixFunction(options.getOS()));
}


void OsTools::copySharedLibraries(const fs::path & sourceRootFolder, const CmdOptions & options)
{
    copyLibraries(sourceRootFolder, options, &sharedSuffix);
}


void OsTools::copyStaticLibraries(const fs::path & sourceRootFolder, const CmdOptions & options)
{
    copyLibraries(sourceRootFolder, options, &staticSuffix);
}
