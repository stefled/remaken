#include "ConanFileRetriever.h"
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
using namespace std;

ConanFileRetriever::ConanFileRetriever(const CmdOptions & options):SystemFileRetriever (options,Dependency::Type::CONAN)
{
}

// bundle command
fs::path ConanFileRetriever::bundleArtefact(const Dependency & dependency)
{
    m_options.verboseMessage("--------------- Conan bundle ---------------");
    m_options.verboseMessage("===> bundling: " + dependency.getName() + "/"+ dependency.getVersion());
    m_tool->bundle(dependency);
    return fs::path();
}

// configure command
std::pair<std::string, fs::path> ConanFileRetriever::invokeGenerator(std::vector<Dependency> & deps)
{
    return m_tool->invokeGenerator(deps);
}

// info command
void ConanFileRetriever::write_pkg_file(std::vector<Dependency> & deps)
{
    return m_tool->write_pkg_file(deps);
}

