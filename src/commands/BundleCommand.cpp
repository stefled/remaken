#include "BundleCommand.h"
#include "managers/BundleManager.h"

BundleCommand::BundleCommand(const CmdOptions & options):AbstractCommand(BundleCommand::NAME),m_options(options)
{
}

int BundleCommand::execute()
{
    auto mgr = BundleManager{m_options};
    return mgr.bundle();
}
