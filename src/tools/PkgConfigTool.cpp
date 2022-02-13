#include "src/tools/SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <boost/algorithm/string_regex.hpp>
#include <string>
#include "PkgConfigTool.h"
#include "backends/BackendGeneratorFactory.h"

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


void PkgConfigTool::libs(Dependency & dep, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    std::string pkgconfigFileName = dep.getName();
    if (dep.getType() == Dependency::Type::REMAKEN) {
        pkgconfigFileName = "bcom-" +  dep.getName();
    }
    int result = bp::system(m_pkgConfigToolPath.generic_string(utf8), "--libs", bp::args(options), env,  pkgconfigFileName, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --libs on '" + pkgconfigFileName + "'");
    }
    std::string res = listOutputFut.get();
    if (res[0] == '\n') {
        res.erase(0,1);
    }
    if (!res.empty()) {
        dep.libs().push_back(res);;
    }
}

void PkgConfigTool::cflags(Dependency & dep, const std::vector<std::string> & options)
{
    fs::detail::utf8_codecvt_facet utf8;
    boost::asio::io_context ios;
    std::future<std::string> listOutputFut;
    auto env = boost::this_process::environment();
    env["PKG_CONFIG_PATH"] = m_pkgConfigPaths;
    std::string pkgconfigFileName = dep.getName();
    if (dep.getType() == Dependency::Type::REMAKEN) {
        pkgconfigFileName = "bcom-" +  dep.getName();
    }
    int result = bp::system(m_pkgConfigToolPath.generic_string(utf8), "--cflags", bp::args(options), env,  pkgconfigFileName, bp::std_out > listOutputFut, ios);
    if (result != 0) {
        throw std::runtime_error("Error running pkg-config --cflags on '" + pkgconfigFileName + "'");
    }

    std::string res = listOutputFut.get();
    if (res[0] == '\n') {
        res.erase(0,1);
    }
    if (!res.empty()) {
        dep.cflags().push_back(res);
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

fs::path PkgConfigTool::generate(const std::vector<Dependency> & deps, Dependency::Type depType)
{
    std::shared_ptr<IGeneratorBackend> generator = BackendGeneratorFactory::getGenerator(m_options);
    return generator->generate(deps, depType);
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



