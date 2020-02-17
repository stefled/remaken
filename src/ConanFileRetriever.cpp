#include "ConanFileRetriever.h"
#include <boost/log/trivial.hpp>

ConanFileRetriever::ConanFileRetriever(const CmdOptions & options):SystemFileRetriever (options)
{
    m_tool = std::make_shared<ConanSystemTool>(options);
}

fs::path ConanFileRetriever::bundleArtefact(const Dependency & dependency)
{
    m_tool->bundle(dependency);
    fs::path outputDirectory = computeLocalDependencyRootDir(dependency);
    return outputDirectory;
}
