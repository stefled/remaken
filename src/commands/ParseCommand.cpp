#include "ParseCommand.h"
#include "DependencyManager.h"

ParseCommand::ParseCommand(const CmdOptions & options):AbstractCommand(ParseCommand::NAME),m_options(options)
{
}

int ParseCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.parse();
}
