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

const std::string_view & OsTools::sharedSuffix(const std::string_view & osStr)
{
    if (os2sharedSuffix.find(osStr) == os2sharedSuffix.end()) {
        return os2sharedSuffix.at("unix");
    }
    return os2sharedSuffix.at(osStr);
}


void OsTools::copySharedLibraries(const fs::path & sourceRootFolder, const CmdOptions & options, bool xpcfBundle)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path destinationFolderPath = options.getDestinationRoot();
    if (xpcfBundle) {
        destinationFolderPath /= options.getModulesSubfolder();
    }
    for (fs::directory_entry& x : fs::directory_iterator(sourceRootFolder)) {
        fs::path filepath = x.path();
        if (fs::is_symlink(x.path())) {
            if (fs::exists(destinationFolderPath/filepath.filename())) {
                fs::remove(destinationFolderPath/filepath.filename());
            }
        //    fs::copy_symlink(x.path(),options.getDestinationRoot()/filepath.filename());
        }
        else if (is_regular_file(filepath)) {
            if (filepath.extension().string(utf8) == sharedSuffix(options.getOS())) {
                fs::copy_file(filepath , destinationFolderPath/filepath.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
    }
}
