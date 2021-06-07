#include "src/tools/SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include "PkgConfigTool.h"

namespace bp = boost::process;

PkgConfigTool::PkgConfigTool()
{
    m_pkgConfigToolPath = bp::search_path(getPkgConfigToolIdentifier()); //or get it from somewhere else.
    if (m_pkgConfigToolPath.empty()) {
        throw std::runtime_error("Error : pkg-config command not found on the system. Please install it first.");
    }
}


std::string PkgConfigTool::getPkgConfigToolIdentifier()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "pkg-config";
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
   return "pkg-config";
#endif
    return "";
}



