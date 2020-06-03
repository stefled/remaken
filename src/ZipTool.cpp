#include "SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include "ZipTool.h"

namespace bp = boost::process;

class unzipTool : public ZipTool {
public:
    unzipTool(bool quiet):ZipTool("unzip",quiet) {}
    ~unzipTool() override = default;
    int uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder) override;
};

int unzipTool::uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    if (m_quiet) {
        result = bp::system(m_zipToolPath,  "-q", "-u", compressedDependency.generic_string(utf8).c_str(), "-d", destinationRootFolder.generic_string(utf8).c_str());
    }
    else  {
        result = bp::system(m_zipToolPath, "-u", compressedDependency.generic_string(utf8).c_str(), "-d", destinationRootFolder.generic_string(utf8).c_str());
    }
    return result;
}

class sevenZTool : public ZipTool {
public:
    sevenZTool(bool quiet):ZipTool("7z",quiet) {}
    ~sevenZTool() override = default;
    int uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder) override;
};

int sevenZTool::uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string outputDirOption = "-o";
    outputDirOption += destinationRootFolder.generic_string(utf8).c_str();
    int result = -1;
    if (m_quiet) {
        result = bp::system(m_zipToolPath,"x", compressedDependency.generic_string(utf8).c_str(), outputDirOption, "-y", bp::std_out > bp::null);
    }
    else  {
        result = bp::system(m_zipToolPath,"x", compressedDependency.generic_string(utf8).c_str(), outputDirOption, "-y", "-bb3");
    }
    return result;
}

ZipTool::ZipTool(const std::string & tool, bool quiet):m_quiet(quiet)
{
    m_zipToolPath = bp::search_path(tool); //or get it from somewhere else.
    if (m_zipToolPath.empty()) {
        throw std::runtime_error("Error : " + tool + " command not found on the system. Please install it first.");
    }
}

std::string ZipTool::getZipToolIdentifier()
{
#ifdef BOOST_OS_ANDROID_AVAILABLE
    return "unzip";
#endif
#ifdef BOOST_OS_IOS_AVAILABLE
    return "unzip";
#endif
#ifdef BOOST_OS_LINUX_AVAILABLE
    return "unzip";
#endif
#ifdef BOOST_OS_SOLARIS_AVAILABLE
    return "unzip";
#endif
#ifdef BOOST_OS_MACOS_AVAILABLE
    return "unzip";
#endif
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return "7z";
#endif
#ifdef BOOST_OS_BSD_FREE_AVAILABLE
   return "unzip";
#endif
    return "";
}

std::shared_ptr<ZipTool> ZipTool::createZipTool(const CmdOptions & options)
{
    //TODO : return appropriate derived ziptool depending on option value
    if (options.getZipTool() == "unzip") {
        return std::make_shared<unzipTool>(!options.getVerbose());
    }
    if (options.getZipTool() == "7z") {
        return std::make_shared<sevenZTool>(!options.getVerbose());
    }
    // This should never happen, as command line options are validated in CmdOptions after parsing
    throw std::runtime_error("Unknown ziptool type " + options.getZipTool());
}



