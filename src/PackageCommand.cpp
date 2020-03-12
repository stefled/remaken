#include "PackageCommand.h"
#include "DependencyManager.h"

PackageCommand::PackageCommand(const CmdOptions & options):AbstractCommand(PackageCommand::NAME),m_options(options)
{
}

int PackageCommand::execute()
{
   // auto mgr = DependencyManager{m_options};
    //return mgr.bundle();
    return 0;
}
