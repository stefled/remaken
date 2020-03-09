#include <iostream>
#include "Constants.h"
#include "CmdOptions.h"
#include "InstallCommand.h"
#include "ParseCommand.h"
#include "BundleCommand.h"
#include "CleanCommand.h"
#include "BundleXpcfCommand.h"
#include "VersionCommand.h"
#include <memory>

using namespace std;

int main(int argc, char** argv)
{
    map<string,shared_ptr<AbstractCommand>> dispatcher;

    CmdOptions opts;
    try {
        if (auto result = opts.parseArguments(argc,argv); result != CmdOptions::OptionResult::RESULT_SUCCESS ) {
            return static_cast<int>(result);
        }
        dispatcher["clean"] = make_shared<CleanCommand>(opts);
        dispatcher["install"] = make_shared<InstallCommand>(opts);
        dispatcher["parse"] = make_shared<ParseCommand>(opts);
        dispatcher["bundle"] = make_shared<BundleCommand>(opts);
        dispatcher["bundleXpcf"] = make_shared<BundleXpcfCommand>(opts);
        dispatcher["version"] = make_shared<VersionCommand>();
        if (mapContains(dispatcher,opts.getAction())) {
            dispatcher.at(opts.getAction())->execute();
        }
        else {
            return -1;
        }
        return 0;
    }
    catch (std::runtime_error & e) {
        std::cout << "ERROR: "<<e.what() << std::endl;
        opts.printUsage();
    }
    return -1;
}
