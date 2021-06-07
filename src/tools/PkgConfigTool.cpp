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
        throw std::runtime_error("Error: pkg-config tool not available: please install pkg-config !");
    }
}

void PkgConfigTool::addPath(const fs::path & pkgConfigPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (!m_pkgConfigPaths.empty()) {
        m_pkgConfigPaths += ":";
    }
    m_pkgConfigPaths +=  pkgConfigPath.parent_path().generic_string(utf8);
}


std::string PkgConfigTool::libs(const std::string & name)
{
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    int result = bp::system(m_pkgConfigToolPath, "--libs", env,  name, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --libs on '" + name + "'");
    }
    return listOutputFut.get();
}

std::string PkgConfigTool::cflags(const std::string & name)
{
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    int result = bp::system(m_pkgConfigToolPath, "--cflags", env,  name, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --cflags on '" + name + "'");
    }
    return listOutputFut.get();
}

fs::path PkgConfigTool::generateQmake(const std::vector<std::string>&  cflags, const std::vector<std::string>&  libs,
                                      const std::string & prefix, const fs::path & destination)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::ofstream fos(destination.generic_string(utf8),std::ios::out);
    std::string libdirs, libsStr;
    for (auto & cflagInfos : cflags) {
        std::vector<std::string> cflagsVect;
        boost::split(cflagsVect, cflagInfos, [&](char c){return c == ' ';});
        for (auto & cflag: cflagsVect) {
            // remove -I
            cflag.erase(0,2);
            boost::trim(cflag);
            fos<<prefix<<"_INCLUDEPATH += \""<<cflag<<"\""<<std::endl;
        }
    }
    for (auto & libInfos : libs) {
        std::vector<std::string> optionsVect;
        boost::split(optionsVect, libInfos, [&](char c){return c == ' ';});
        for (auto & option: optionsVect) {
            std::string prefix = option.substr(0,2);
            if (prefix == "-L") {
                option.erase(0,2);
                libdirs += " -L\"" + option + "\"";
            }
            //TODO : extract lib paths from libdefs and put quotes around libs path
            else {
                libsStr += " " + option;
            }
        }
    }
    fos<<prefix<<"_LIBS +="<<libsStr<<std::endl;
    fos<<prefix<<"_SYSTEMLIBS += "<<std::endl;
    fos<<prefix<<"_FRAMEWORKS += "<<std::endl;
    fos<<prefix<<"_FRAMEWORKS_PATH += "<<std::endl;
    fos<<prefix<<"_LIBDIRS +="<<libdirs<<std::endl;
    fos<<prefix<<"_BINDIRS +="<<std::endl;
    fos<<prefix<<"_DEFINES +="<<std::endl;
    fos<<prefix<<"_DEFINES +="<<std::endl;
    fos<<prefix<<"_QMAKE_CXXFLAGS +="<<std::endl;
    fos<<prefix<<"_QMAKE_CFLAGS +="<<std::endl;
    fos<<prefix<<"_QMAKE_LFLAGS +="<<std::endl;
    fos<<std::endl;
    fos<<"INCLUDEPATH += $$"<<prefix<<"_INCLUDEPATH"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_LIBS"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_LIBDIRS"<<std::endl;
    fos<<"BINDIRS += $$"<<prefix<<"_BINDIRS"<<std::endl;
    fos<<"DEFINES += $$"<<prefix<<"_DEFINES"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_FRAMEWORKS"<<std::endl;
    fos<<"LIBS += $$"<<prefix<<"_FRAMEWORK_PATHS"<<std::endl;
    fos<<"QMAKE_CXXFLAGS += $$"<<prefix<<"_QMAKE_CXXFLAGS"<<std::endl;
    fos<<"QMAKE_CFLAGS += $$"<<prefix<<"_QMAKE_CFLAGS"<<std::endl;
    fos<<"QMAKE_LFLAGS += $$"<<prefix<<"_QMAKE_LFLAGS"<<std::endl;
    fos.close();
    return destination;
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



