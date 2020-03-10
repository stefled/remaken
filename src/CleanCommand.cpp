#include "CleanCommand.h"
#include "DependencyManager.h"

CleanCommand::CleanCommand(const CmdOptions & options):AbstractCommand(CleanCommand::NAME),m_options(options)
{
}

int CleanCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.clean();
}
