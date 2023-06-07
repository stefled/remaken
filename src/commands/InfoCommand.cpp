#include "InfoCommand.h"
#include "managers/DependencyManager.h"
#include "utils/DepUtils.h"
#include "FileHandlerFactory.h"
#include <memory>
#include <boost/process.hpp>

namespace bp = boost::process;

using namespace std;

InfoCommand::InfoCommand(const CmdOptions & options):AbstractCommand(InfoCommand::NAME),m_options(options)
{
}

int InfoCommand::execute()
{
    std::cout<<"--------- Dependencies ---------"<<std::endl;
    auto mgr = DependencyManager{m_options};
    int ret = mgr.info();

    if (m_options.infoDisplayPathsOption()) {
        displayInfoPaths();
    }

    auto subCommand = m_options.getSubcommand();
    if (subCommand == "pkg_systemfile") {
        return write_pkg_file();
    }

    return ret;
}

void InfoCommand::displayInfoPaths()
{
    fs::detail::utf8_codecvt_facet utf8;

    std::cout<<std::endl;
    std::cout<<"--------- Starting display paths of dependencies ---------"<<std::endl;
    std::cout<<"NB: Only existing paths are displayed. Please install your dependencies before."<<std::endl;
    if (!m_options.getDependenciesFile().empty()) {
        fs::path depPath = DepUtils::buildDependencyPath(m_options.getDependenciesFile());
        fs::path depFolder = depPath.parent_path();
        fs::path buildProjectSubFolderPath = DepUtils::getProjectBuildSubFolder(m_options);

        std::vector<Dependency> depsVect;
        DepUtils::parseRecurse(depPath, m_options, depsVect);


        std::map<std::string,Dependency> depsMap;
        for (auto dependency : depsVect) {//filter redundant deps
            //std::cout<<dependency.toString()<<std::endl;
            depsMap.insert_or_assign(dependency.getName()+dependency.getVersion(), dependency);
        }

        // SLETODO : need to get root path, and includepaths

        std::cout<<"--------- include Paths ---------"<<std::endl;
        for (auto & [name,dependency]: depsMap) {
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
            std::vector<fs::path> includePaths = fileRetriever->includePaths(dependency);
            bool displayDepName = false;
            for (auto path: includePaths) {
                if (fs::exists(path)) {
                    if (!displayDepName) {
                        displayDepName = true;
                        std::cout<<"  -- " << dependency.getName() << ": --" << std::endl;
                    }
                    std::cout << "     " << path.generic_string(utf8) << std::endl;
                }
            }
        }

        std::cout<<"--------- lib Paths ---------"<<std::endl;
        std::cout<<"(for shared dependency, path of shared librairies is displayed)"<<std::endl;
#ifdef BOOST_OS_WINDOWS_AVAILABLE
        std::cout<<"NB : Conan shared librairies are in \"bin\" directory for native builds on Windows"<<std::endl;
#endif
        for (auto & [name,dependency]: depsMap) {
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
            std::vector<fs::path> libPaths = fileRetriever->libPaths(dependency);
            bool displayDepName = false;
            for (auto path: libPaths) {
                if (fs::exists(path)) {
                    if (!displayDepName) {
                        displayDepName = true;
                        std::cout<<"  -- " << dependency.getName() << " (" << dependency.getMode() << ") : --" << std::endl;
                    }
                    std::cout << "     " << path.generic_string(utf8) << std::endl;
                }
            }
        }

        std::cout<<"--------- bin Paths ---------"<<std::endl;
        for (auto & [name,dependency]: depsMap) {
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(dependency, m_options);
            std::vector<fs::path> binPaths = fileRetriever->binPaths(dependency);
            bool displayDepName = false;
            for (auto path: binPaths) {
                if (fs::exists(path)) {
                    if (!displayDepName) {
                        displayDepName = true;
                        std::cout<<"  -- " << dependency.getName() << ": --" << std::endl;
                    }
                    std::cout << "     " << path.generic_string(utf8) << std::endl;
                }
            }
        }
    }
}

int InfoCommand::write_pkg_file()
{
    fs::detail::utf8_codecvt_facet utf8;
    std::map<Dependency::Type,std::vector<Dependency>> depsVectMap;

    std::vector<fs::path> libPaths;

    std::cout<<"--------- Starting generate packages file ---------"<<std::endl;
    if (!m_options.getDependenciesFile().empty()) {
        fs::path depPath = DepUtils::buildDependencyPath(m_options.getDependenciesFile());
        fs::path depFolder = depPath.parent_path();
        fs::path buildProjectSubFolderPath = DepUtils::getProjectBuildSubFolder(m_options);

        std::vector<Dependency> depsVect;
        DepUtils::parseRecurse(depPath, m_options, depsVect);

        for (auto & dep : depsVect) {
            depsVectMap[dep.getType()].push_back(dep);
        }
        std::map<std::string,fs::path> setupInfoMap;
        if (mapContains(depsVectMap, Dependency::Type::CONAN)) {
            auto & depVect = depsVectMap[Dependency::Type::CONAN];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::CONAN, m_options, "conan");
            fileRetriever->write_pkg_file(depVect);
        }
        /*
         * TODO
        if (mapContains(depsVectMap, Dependency::Type::REMAKEN)) {
            auto & depVect = depsVectMap[Dependency::Type::REMAKEN];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::REMAKEN, m_options, "github");
            fileRetriever->write_pkg_file(depVect);
        }
        if (mapContains(depsVectMap, Dependency::Type::BREW)) {
            auto & depVect = depsVectMap[Dependency::Type::BREW];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::BREW, m_options, "system");
            fileRetriever->write_pkg_file(depVect);
        }
        if (mapContains(depsVectMap, Dependency::Type::SYSTEM)) {
            auto & depVect = depsVectMap[Dependency::Type::SYSTEM];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::SYSTEM, m_options, "system");
            fileRetriever->write_pkg_file(depVect);
        }
        */
        std::cout<<"====> Generate pkg file done successfully !"<<std::endl;
        return 0;
    }
    return -1;
}

