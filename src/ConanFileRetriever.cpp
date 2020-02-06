#include "ConanFileRetriever.h"

ConanFileRetriever::ConanFileRetriever(const CmdOptions & options):SystemFileRetriever (options)
{
    m_tool = std::make_shared<ConanSystemTool>(options);
}
