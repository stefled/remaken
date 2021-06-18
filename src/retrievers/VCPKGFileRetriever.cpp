#include "VCPKGFileRetriever.h"

VCPKGFileRetriever::VCPKGFileRetriever(const CmdOptions & options):SystemFileRetriever (options)
{
    m_tool = std::make_shared<VCPKGSystemTool>(options);
}
