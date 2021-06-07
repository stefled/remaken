#include "InstallCommand.h"
#include "DependencyManager.h"

InstallCommand::InstallCommand(const CmdOptions & options):AbstractCommand(InstallCommand::NAME),m_options(options)
{
}

int InstallCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.retrieve();
}
