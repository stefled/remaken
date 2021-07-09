#include "src/tools/SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <boost/algorithm/string_regex.hpp>
#include <string>
#include "PkgConfigTool.h"

namespace bp = boost::process;

PkgConfigTool::PkgConfigTool(const CmdOptions & options):m_options(options)
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
    m_pkgConfigPaths +=  pkgConfigPath.generic_string(utf8);
}


void PkgConfigTool::libs(const std::string & name, std::vector<std::string> & libFlagList, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    int result = bp::system(m_pkgConfigToolPath.generic_string(utf8), "--libs", bp::args(options), env,  name, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --libs on '" + name + "'");
    }
    std::string res = listOutputFut.get();
    if (res[0] == '\n') {
        res.erase(0,1);
    }
    if (!res.empty()) {
        libFlagList.push_back(res);
    }
}

void PkgConfigTool::cflags(const std::string & name, std::vector<std::string> & cFlagList, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    int result = bp::system(m_pkgConfigToolPath.generic_string(utf8), "--cflags", bp::args(options), env,  name, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --cflags on '" + name + "'");
    }

    std::string res = listOutputFut.get();
    if (res[0] == '\n') {
        res.erase(0,1);
    }
    if (!res.empty()) {
        cFlagList.push_back(res);
    }
}


static const std::map<Dependency::Type,std::string> type2prefixMap = {
  {Dependency::Type::BREW,"BREW"},
    {Dependency::Type::REMAKEN,"REMAKEN"},
    {Dependency::Type::CONAN,"CONAN"},
    {Dependency::Type::CHOCO,"CHOCO"},
    {Dependency::Type::SYSTEM,"SYSTEM"},
    {Dependency::Type::SCOOP,"SCOOP"},
    {Dependency::Type::VCPKG,"VCPKG"}
};

fs::path PkgConfigTool::generateQmake(const std::vector<std::string>&  cflags, const std::vector<std::string>&  libs,
                                      Dependency::Type type)
{
    fs::detail::utf8_codecvt_facet utf8;
    if (!mapContains(type2prefixMap,type)) {
        throw std::runtime_error("Error dependency type " + std::to_string(static_cast<uint32_t>(type)) + " no supported");
    }
    std::string prefix = type2prefixMap.at(type);
    std::string filename = boost::to_lower_copy(prefix) + "buildinfo.pri";
    fs::path filePath = DepUtils::getProjectBuildSubFolder(m_options)/filename;
    std::ofstream fos(filePath.generic_string(utf8),std::ios::out);
    std::string libdirs, libsStr, defines;
    for (auto & cflagInfos : cflags) {
        std::vector<std::string> cflagsVect;
        boost::split_regex(cflagsVect, cflagInfos, boost::regex( " -" ));
        for (auto & cflag: cflagsVect) {
            if (cflag[0] == '-') {
                cflag.erase(0,1);
            }
            std::string cflagPrefix = cflag.substr(0,1);
            if (cflagPrefix == "I") {
                // remove -I
                cflag.erase(0,1);
                boost::trim(cflag);
                fos<<prefix<<"_INCLUDEPATH += \""<<cflag<<"\""<<std::endl;
            }
            else if (cflagPrefix == "D") {
                // remove -D
                cflag.erase(0,1);
                boost::trim(cflag);
                defines += " " + cflag;
            }
        }
    }
    fos<<prefix<<"_DEFINES += "<<defines<<std::endl;
    for (auto & libInfos : libs) {
        std::vector<std::string> optionsVect;
        boost::split_regex(optionsVect, libInfos, boost::regex( " -" ));
        for (auto & option: optionsVect) {
            if (option[0] == '-') {
                option.erase(0,1);
            }
            std::string optionPrefix = option.substr(0,1);
            if (optionPrefix == "L") {
                option.erase(0,1);
                libdirs += " -L\"" + option + "\"";
            }
            //TODO : extract lib paths from libdefs and put quotes around libs path
            else {
                boost::trim(option);
                libsStr += " -" + option;
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
    return filePath;
}

fs::path PkgConfigTool::generate(GeneratorType genType, const std::vector<std::string>&  cflags, const std::vector<std::string>&  libs,
                       Dependency::Type depType)
{
    if (genType == GeneratorType::qmake) {
        return generateQmake(cflags, libs, depType);
    }
    else {
        throw std::runtime_error("Only qmake generator is supported, other generators' support coming in future releases");
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



