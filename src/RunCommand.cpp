#include "RunCommand.h"
#include "DependencyManager.h"

RunCommand::RunCommand(const CmdOptions & options):AbstractCommand(RunCommand::NAME),m_options(options)
{
}

int RunCommand::execute()
{

    // supports debug/release config : will be reflected in constructed paths
    // parse pkgdeps recursively if any
    // parse xml
    // gather deps by types
    // conan -> generate json
    // brew, vcpkg, choco, conan -> get libPaths
    // remaken -> get path for each remaken deps (from xml or from pkgdeps parsing)
    if (!m_options.getXpcfXmlFile().empty()) {

    }

    if (!m_options.getDependenciesFile().empty()) {
        std::vector<Dependency> deps;
        DependencyManager::parseRecurse(m_options.getDependenciesFile(), m_options, deps);
        for (Dependency & dependency : deps) {
            std::cout<<dependency.toString()<<std::endl;
        }
    }
    if (m_options.environmentOnly()) {
    // display results
        return 0;
    }
    if (!m_options.getApplicationFile().empty()) {

    }

    return 0;
}
