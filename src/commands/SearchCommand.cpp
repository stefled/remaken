#include "SearchCommand.h"
#include "tools/SystemTools.h"


SearchCommand::SearchCommand(const CmdOptions & options):AbstractCommand(SearchCommand::NAME),m_options(options)
{
}

const std::map<std::string,Dependency::Type> stringToTypeMap =
{
    {"remaken",Dependency::Type::REMAKEN},
    {"brew",Dependency::Type::BREW},
    {"choco",Dependency::Type::CHOCO},
    {"conan",Dependency::Type::CONAN},
    {"scoop",Dependency::Type::SCOOP},
    {"vcpkg",Dependency::Type::VCPKG},
    {"system",Dependency::Type::SYSTEM}
};

int SearchCommand::execute()
{
    const std::map<std::string,std::string> & searchOpts = m_options.getSearchCommandOptions();
    std::string pkgSystem = searchOpts.at("packagingSystem");
    std::vector<std::shared_ptr<BaseSystemTool>> tools;
    if (!pkgSystem.empty()) {
        std::shared_ptr<BaseSystemTool> tool = SystemTools::createTool(m_options, stringToTypeMap.at(pkgSystem));
        tools.push_back(tool);
    }
    else {
        tools = SystemTools::retrieveTools(m_options);
    }
    for (auto & tool : tools) {
        std::cout<<"---------------------------"<<std::endl;
        tool->search(searchOpts.at("pkgName"),searchOpts.at("pkgVersion"));
    }
    return 0;
}
