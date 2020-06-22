#include "ProfileCommand.h"
#include "DependencyManager.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "PathBuilder.h"
#include "HttpFileRetriever.h"
#include "OsTools.h"
#include <boost/log/trivial.hpp>

ProfileCommand::ProfileCommand(const CmdOptions & options):AbstractCommand(ProfileCommand::NAME),m_options(options)
{
}

int ProfileCommand::execute()
{
    auto subCommand = m_options.getProfileSubcommand();
    if (subCommand == "display") {
        m_options.displayConfigurationSettings();
        return 0;
    }
    if (subCommand == "init") {
        m_options.writeConfigurationFile();
        return 0;
    }
    return 0;
}
