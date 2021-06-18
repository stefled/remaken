#include "VersionCommand.h"
#include <iostream>

using namespace std;

VersionCommand::VersionCommand():AbstractCommand(VersionCommand::NAME)
{

}

int VersionCommand::execute()
{
    cout<<getVersion()<<endl;
    return 0;
}

std::string VersionCommand::getVersion() const
{
    std::string version = MYVERSIONSTRING;
    return version;
}
