#include "RemoteCommand.h"
#include "tools/SystemTools.h"
#include "utils/DepUtils.h"



RemoteCommand::RemoteCommand(const CmdOptions & options):AbstractCommand(RemoteCommand::NAME),m_options(options)
{
}

int RemoteCommand::execute()
{
    std::vector<Dependency> deps;
    auto subCommand = m_options.getSubcommand();
    if (subCommand == "add") {
        if (m_options.recurse()) {
            DepUtils::parseRecurse(m_options.getDependenciesFile(), m_options, deps);
        }
        else {
            deps = DepUtils::parse(m_options.getDependenciesFile(), m_options);
        }
        std::cout<<"Adding declared remotes/taps/sources/repositories declared in dependencies:"<<std::endl;
        for (const auto & dep :deps) {
            if (dep.getType() != Dependency::Type::REMAKEN) {
                if (!dep.getBaseRepository().empty()) {
                    std::cout<<"=> "<<dep.getBaseRepository()<<std::endl;
                    auto tool = SystemTools::createTool(m_options,dep.getType());
                    tool->addRemote(dep.getBaseRepository());
                }
            }
        }
    }
    if (subCommand == "list") {
        auto  tools = SystemTools::retrieveTools(m_options);
        for (auto & tool : tools) {
            tool->listRemotes();
            std::cout<<std::endl;
        }
    }
    if (subCommand == "listfile") {
        if (m_options.recurse()) {
            DepUtils::parseRecurse(m_options.getDependenciesFile(), m_options, deps);
        }
        else {
            deps = DepUtils::parse(m_options.getDependenciesFile(), m_options);
        }
        std::cout<<"Additional repositories declared in dependencies:"<<std::endl;
        for (const auto & dep :deps) {
            if (dep.getType() != Dependency::Type::REMAKEN) {
                if (!dep.getBaseRepository().empty()) {
                    std::cout<<"=> "<<dep.getBaseRepository()<<std::endl;
                }
            }
        }
    }

    return 0;
}
