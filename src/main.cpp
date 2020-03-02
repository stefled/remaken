#include <iostream>
#include "Constants.h"
#include "CmdOptions.h"
#include "InstallCommand.h"
#include "ParseCommand.h"
#include "BundleCommand.h"
#include "BundleXpcfCommand.h"
#include "VersionCommand.h"
#include <memory>

namespace po = boost::program_options;
using namespace std;

int main(int argc, char** argv)
{
    map<string,shared_ptr<AbstractCommand>> dispatcher;

    CmdOptions opts;
    if (auto result = opts.parseArguments(argc,argv); result != CmdOptions::OptionResult::RESULT_SUCCESS ) {
        return static_cast<int>(result);
    }
    dispatcher["install"] = make_shared<InstallCommand>(opts);
    dispatcher["parse"] = make_shared<ParseCommand>(opts);
    dispatcher["bundle"] = make_shared<BundleCommand>(opts);
    dispatcher["bundleXpcf"] = make_shared<BundleXpcfCommand>(opts);
    dispatcher["version"] = make_shared<VersionCommand>();
    dispatcher.at(opts.getAction())->execute();
    return 0;
}
