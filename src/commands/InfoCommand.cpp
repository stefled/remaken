#include "InfoCommand.h"
#include "managers/DependencyManager.h"

InfoCommand::InfoCommand(const CmdOptions & options):AbstractCommand(InfoCommand::NAME),m_options(options)
{
}

int InfoCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.info();
}
