#include "FSFileRetriever.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

FSFileRetriever::FSFileRetriever(const CmdOptions & options):AbstractFileRetriever (options)
{
}

fs::path FSFileRetriever::retrieveArtefact(const Dependency & dependency)
{
    // LOGGER.info(std::string.format("Download file %s", url));
    std::string source = this->computeSourcePath(dependency);
    fs::detail::utf8_codecvt_facet utf8;
    fs::path sourcePath(source,utf8);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    fs::path output = this->m_workingDirectory / boost::uuids::to_string(uuid);
    output += sourcePath.extension();
    try {
        fs::copy(sourcePath,output);
    }
    catch (const fs::filesystem_error & e) {
        throw std::runtime_error(e.what());
    }
    return output;
}
