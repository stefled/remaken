#include "SystemFileRetriever.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

namespace fs = boost::filesystem;
namespace bp = boost::process;

SystemFileRetriever::SystemFileRetriever(const CmdOptions & options, Dependency::Type dependencyType):AbstractFileRetriever (options)
{
    m_tool = SystemTools::createTool(options, dependencyType);
    m_scriptFilePath =  m_options.getDestinationRoot() / "bundle_system_install.sh";
    if (fs::exists(m_scriptFilePath)) {
        fs::remove(m_scriptFilePath);
    }
}

fs::path SystemFileRetriever::bundleArtefact(const Dependency & dependency)
{
    m_tool->bundle(dependency);
    fs::path outputDirectory = computeLocalDependencyRootDir(dependency);
    m_tool->bundleScript(dependency, m_scriptFilePath);
    return outputDirectory;
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

std::vector<fs::path> SystemFileRetriever::binPaths(const Dependency & dependency)
{
    return m_tool->binPaths(dependency);
}

std::vector<fs::path> SystemFileRetriever::libPaths(const Dependency & dependency)
{
    return m_tool->libPaths(dependency);
}

fs::path SystemFileRetriever::invokeGenerator(const std::vector<Dependency> & deps, GeneratorType generator)
{
    return m_tool->invokeGenerator(deps,generator);
}
