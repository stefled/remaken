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
