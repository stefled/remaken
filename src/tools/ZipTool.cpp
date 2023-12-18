#include "SystemTools.h"

#include <boost/process.hpp>
#include <boost/predef.h>
#include <string>
#include "ZipTool.h"

namespace bp = boost::process;

class unzipTool : public ZipTool {
public:
    unzipTool(bool quiet,bool override):ZipTool("unzip", quiet, override) {}
    ~unzipTool() override = default;
    int uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder) override;
    int compressArtefact([[maybe_unused]] const fs::path & folderToCompress) override { return -1; };
};

int unzipTool::uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    std::vector<std::string> settingsArgs;
    if (m_quiet) {
        settingsArgs.push_back("-q");
    }
    if (m_override) {
        settingsArgs.push_back("-o");
    }
    else {
        settingsArgs.push_back("-u");
    }

    result = bp::system(m_zipToolPath, bp::args(settingsArgs), compressedDependency.generic_string(utf8).c_str(), "-d", destinationRootFolder.generic_string(utf8).c_str());
    return result;
}

class sevenZTool : public ZipTool {
public:
    sevenZTool(bool quiet, bool override = true):ZipTool("7z", quiet, override) {}
    ~sevenZTool() override = default;
    int uncompressArtefact(const fs::path & compressedDependency, const fs::path & destinationRootFolder) override;
    int compressArtefact(const fs::path & folderToCompress) override;
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

int sevenZTool::compressArtefact(const fs::path & folderToCompress)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    if (m_quiet) {
        result = bp::system(m_zipToolPath,"a", folderToCompress.generic_string(utf8).c_str(),  bp::std_out > bp::null);
    }
    else  {
        result = bp::system(m_zipToolPath,"a", folderToCompress.generic_string(utf8).c_str(),  "-bb3");
    }
    return result;
}

ZipTool::ZipTool(const std::string & tool, bool quiet, bool override):m_quiet(quiet), m_override(override)
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
        return std::make_shared<unzipTool>(!options.getVerbose(),options.override());
    }
    if (options.getZipTool() == "7z") {
        return std::make_shared<sevenZTool>(!options.getVerbose(),options.override());
    }
    // This should never happen, as command line options are validated in CmdOptions after parsing
    throw std::runtime_error("Unknown ziptool type " + options.getZipTool());
}



