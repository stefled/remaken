#include "CleanCommand.h"
#include "DependencyManager.h"

CleanCommand::CleanCommand(const CmdOptions & options):AbstractCommand(CleanCommand::NAME),m_options(options)
{
}

// to add: clean all (including shared/static debug/release options or all flavors,
// considering subdeps : tricky for brew, conan .. can use info behavior to figure out the deps tree)
// clean from pkgdeps.txt
// clean deps based on dep type
int CleanCommand::execute()
{
    auto mgr = DependencyManager{m_options};
    return mgr.clean();
}
