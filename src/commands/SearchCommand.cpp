#include "SearchCommand.h"
#include "tools/SystemTools.h"


SearchCommand::SearchCommand(const CmdOptions & options):AbstractCommand(SearchCommand::NAME),m_options(options)
{
}

int SearchCommand::execute()
{
    std::vector<std::shared_ptr<BaseSystemTool>> tools = SystemTools::retrieveTools(m_options);
    for (auto & tool : tools) {
        std::cout<<"---------------------------"<<std::endl;
        const std::map<std::string,std::string> & searchOpts = m_options.getSearchCommandOptions();
        tool->search(searchOpts.at("pkgName"),searchOpts.at("pkgVersion"));
    }
    return 0;
}
