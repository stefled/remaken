#include "SystemFileRetriever.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

namespace fs = boost::filesystem;
namespace bp = boost::process;

SystemFileRetriever::SystemFileRetriever(const CmdOptions & options, std::optional<std::reference_wrapper<const Dependency>> dependencyOpt):AbstractFileRetriever (options)
{
    m_tool = SystemTools::createTool(options, dependencyOpt);
}

std::string SystemFileRetriever::computeSourcePath( const Dependency &  dependency)
{
    return m_tool->computeSourcePath(dependency);
}

fs::path SystemFileRetriever::installArtefactImpl(const Dependency & dependency)
{
    return retrieveArtefact(dependency);
}

fs::path SystemFileRetriever::retrieveArtefact(const Dependency & dependency)
{
    std::string source = this->computeSourcePath(dependency);
    m_tool->install(dependency);
    fs::path output(source);
    return output;
}
