#include "BundleCommand.h"
#include "managers/DependencyManager.h"

BundleCommand::BundleCommand(const CmdOptions & options):AbstractCommand(BundleCommand::NAME),m_options(options)
{
}

int BundleCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.bundle();
}
