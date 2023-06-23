#include "PackageCommand.h"
//#include "managers/DependencyManager.h"
#include "tools/ZipTool.h"
//#include "utils/OsUtils.h"

PackageCommand::PackageCommand(const CmdOptions & options):AbstractCommand(PackageCommand::NAME),m_options(options)
{
}

void PackageCommand::compressFolder(fs::path folderPath)
{
    auto zipTool = ZipTool::createZipTool(m_options);

}
// whatever package type i.e. lib, bin or headers
// copy *.pc, pkgdeps*.txt
// copy or create .pkginfo with adequate .lib, .bin or .headers
// copy pkgname-version_remakeninfo.txt
// copy every folder from post install package root folder

int PackageCommand::compress()
{
    fs::detail::utf8_codecvt_facet utf8;
    //bool bRecurse = false;
    const std::map<std::string,std::string> & compressOptions = m_options.getCompressCommandOptions();
    fs::path rootFolder = compressOptions.at("rootdir");
    fs::path toolChainPrefix =  m_options.getOS() + "-" + m_options.getBuildToolchain();
    if (fs::is_directory(rootFolder)) {
        fs::path subFolder = rootFolder;
        subFolder /= toolChainPrefix;
        if (fs::is_directory(subFolder)) {
            rootFolder = subFolder;
        }
        for (fs::directory_entry& pkgFolder : fs::directory_iterator(rootFolder)) {
            if (fs::is_directory(pkgFolder.path())) {//found pkg
                std::cout<<pkgFolder.path().c_str()<<std::endl;
                fs::path pkgName = pkgFolder.path().filename();
                for (fs::directory_entry& pkgVersionFolder : fs::directory_iterator(pkgFolder.path())) {
                    if (fs::is_directory(pkgVersionFolder.path())) {//found version
                        std::cout<<pkgVersionFolder.path().c_str()<<std::endl;
                        fs::path pkgVersion = pkgVersionFolder.path().filename();
                        if (!fs::exists(pkgVersionFolder.path()/"lib") || !fs::is_directory(pkgVersionFolder.path()/"lib")) {// package without libs
                            //fs::create_directories();
                            //use mode and config from options at first
                            fs::path tmpFolder = toolChainPrefix;
                            tmpFolder += "_";
                            tmpFolder += m_options.getArchitecture();
                            tmpFolder += "_";
                            tmpFolder += m_options.getMode();
                            tmpFolder += "_";
                            tmpFolder += m_options.getConfig();
                            tmpFolder /= pkgName;
                            tmpFolder /= pkgVersion;
                            std::cout<<tmpFolder.c_str()<<std::endl;

                            fs::create_directories(tmpFolder/Constants::PKGINFO_FOLDER);
                            try {
                                fs::copy(pkgVersionFolder.path()/"interfaces", tmpFolder/"interfaces", fs::copy_options::recursive);
                            }
                            catch (std::exception&) { // Not using fs::filesystem_error since std::bad_alloc can throw too.
                                // Handle exception or use error code overload of fs::copy.
                            }
                        }
                        else {// package with libs
                            for (fs::directory_entry& platformFolder : fs::directory_iterator(pkgVersionFolder.path()/"lib")) {
                                if (fs::is_directory(platformFolder.path())) {//found platform
                                    std::cout<<platformFolder.path().c_str()<<std::endl;
                                    fs::path platform = platformFolder.path().filename();
                                    for (fs::directory_entry& modeFolder : fs::directory_iterator(platformFolder.path())) {
                                        if (fs::is_directory(modeFolder.path())) {//found mode
                                            std::cout<<modeFolder.path().c_str()<<std::endl;
                                            fs::path mode = modeFolder.path().filename();
                                            for (fs::directory_entry& configFolder : fs::directory_iterator(modeFolder.path())) {
                                                if (fs::is_directory(configFolder.path())) {//found config
                                                    std::cout<<configFolder.path().c_str()<<std::endl;
                                                    fs::path config = configFolder.path().filename();
                                                    fs::path tmpFolder = toolChainPrefix;
                                                    tmpFolder += "_";
                                                    tmpFolder += platform;
                                                    tmpFolder += "_";
                                                    tmpFolder += mode;
                                                    tmpFolder += "_";
                                                    tmpFolder += config;
                                                    tmpFolder /= pkgName;
                                                    tmpFolder /= pkgVersion;
                                                    std::cout<<tmpFolder.c_str()<<std::endl;

                                                    fs::create_directories(tmpFolder/Constants::PKGINFO_FOLDER);
                                                    try {
                                                        fs::copy(pkgVersionFolder.path()/"interfaces", tmpFolder/"interfaces", fs::copy_options::recursive);
                                                        //OsUtils::copyLibraries(configFolder.path(),tmpFolder/,OsUtils::sharedSuffix(m_options.getOS()));
                                                        //OsUtils::copyLibraries(configFolder.path(),tmpFolder/,OsUtils::staticSuffix(m_options.getOS()));
                                                    }
                                                    catch (std::exception&) { // Not using fs::filesystem_error since std::bad_alloc can throw too.
                                                        // Handle exception or use error code overload of fs::copy.
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }
    return 0;
}

int PackageCommand::execute()
{
    auto subCommand = m_options.getSubcommand();
    if (subCommand == "compress") {
        return compress();
    }
    // no subcommand, process package command
    // auto mgr = DependencyManager{m_options};
    //return mgr.bundle();
    return 0;
}
