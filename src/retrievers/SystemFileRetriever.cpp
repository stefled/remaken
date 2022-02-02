#include "SystemFileRetriever.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/predef.h>
#include <fstream>
using namespace std;

namespace fs = boost::filesystem;
namespace bp = boost::process;

SystemFileRetriever::SystemFileRetriever(const CmdOptions & options, Dependency::Type dependencyType):AbstractFileRetriever (options)
{
    fs::detail::utf8_codecvt_facet utf8;
    m_tool = SystemTools::createTool(options, dependencyType);
#if defined(BOOST_OS_MACOS_AVAILABLE) || defined(BOOST_OS_LINUX_AVAILABLE)
    if (m_tool->bundleScripted()) {
        std::string scriptFileName = "bundle_" + to_string(dependencyType) + "_install.sh";
        m_scriptFilePath =  m_options.getDestinationRoot() / scriptFileName;
        if (fs::exists(m_scriptFilePath)) {
            fs::remove(m_scriptFilePath);
        }
        ofstream fos(m_scriptFilePath.generic_string(utf8),ios::out);
        fos<<"#!/bin/bash"<< '\n';
        fos.close();
        fs::permissions(m_scriptFilePath, fs::add_perms|fs::owner_exe|fs::group_exe|fs::others_exe);
    }
#endif
}

fs::path SystemFileRetriever::bundleArtefact(const Dependency & dependency)
{
    m_tool->bundle(dependency);
    fs::path outputDirectory = computeLocalDependencyRootDir(dependency);
    if (m_tool->bundleScripted()) {
        m_tool->bundleScript(dependency, m_scriptFilePath);
    }
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
