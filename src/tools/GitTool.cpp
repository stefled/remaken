#include "src/tools/SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include "GitTool.h"

namespace bp = boost::process;

GitTool::GitTool(bool override): m_override(override)
{
    m_gitToolPath = bp::search_path(getGitToolIdentifier()); //or get it from somewhere else.
    if (m_gitToolPath.empty()) {
        throw std::runtime_error("Error : git command not found on the system. Please install it first.");
    }
}

int GitTool::clone(const std::string & url, const fs::path & destinationRootFolder, bool recurseSubModule)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    std::vector<std::string> settingsArgs;
    if (recurseSubModule) {
        settingsArgs.push_back("--recursive");
    }
    if (m_override) {
        if (fs::exists(destinationRootFolder)) {
            fs::remove(destinationRootFolder);
        }
        settingsArgs.push_back("-o");
    }

    result = bp::system(m_gitToolPath, "clone", bp::args(settingsArgs), url.c_str(), destinationRootFolder.generic_string(utf8).c_str());
    return result;
}

std::string GitTool::getGitToolIdentifier()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "git";
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
   return "git";
#endif
    return "";
}



