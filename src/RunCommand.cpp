#include "RunCommand.h"
#include "DependencyManager.h"

RunCommand::RunCommand(const CmdOptions & options):AbstractCommand(RunCommand::NAME),m_options(options)
{
}

int RunCommand::execute()
{
    return 0;
}
