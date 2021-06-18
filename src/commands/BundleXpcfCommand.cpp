#include "BundleXpcfCommand.h"
#include "managers/DependencyManager.h"

BundleXpcfCommand::BundleXpcfCommand(const CmdOptions & options):AbstractCommand(BundleXpcfCommand::NAME),m_options(options)
{
}

int BundleXpcfCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.bundleXpcf();
}
