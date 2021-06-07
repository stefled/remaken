#include "BundleXpcfCommand.h"
#include "managers/BundleManager.h"

BundleXpcfCommand::BundleXpcfCommand(const CmdOptions & options):AbstractCommand(BundleXpcfCommand::NAME),m_options(options)
{
}

int BundleXpcfCommand::execute()
{
    auto mgr = BundleManager{m_options};
    return mgr.bundleXpcf();
}
