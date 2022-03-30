#include "ConfigureCommand.h"
#include "utils/DepUtils.h"
#include "managers/XpcfXmlManager.h"
#include "utils/OsUtils.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include "FileHandlerFactory.h"
#include <memory>
#include <map>
#include <vector>
#include "backends/BackendGeneratorFactory.h"


namespace bp = boost::process;

using namespace std;
// TODO recurse ou non
// TODO more information while installing
ConfigureCommand::ConfigureCommand(const CmdOptions & options):AbstractCommand(ConfigureCommand::NAME),m_options(options)
{
}

int ConfigureCommand::execute()
{
    // pb : configurecommand must be run on generated pkgdeps ... the command should use configure options and/or generate the pkgdeps ??
    // should Install command generate the configure_conditions.pri ? or is it the role of configure command ?
    fs::detail::utf8_codecvt_facet utf8;
    // supports debug/release config : will be reflected in constructed paths
    // parse pkgdeps recursively if any
    // parse xml
    // gather deps by types
    // conan -> generate json
    // brew, vcpkg, choco, conan -> get libPaths
    // remaken -> get path for each remaken deps (from xml or from pkgdeps parsing)
    std::map<Dependency::Type,std::vector<Dependency>> depsVectMap;

    std::vector<fs::path> libPaths;
    std::cout<<"--------- Starting dependencies build configuration ---------"<<std::endl;
    std::shared_ptr<IGeneratorBackend> generator = BackendGeneratorFactory::getGenerator(m_options);
    if (!m_options.getDependenciesFile().empty()) {
        bool projectFolder = false;
        fs::path depPath = DepUtils::buildDependencyPath(m_options.getDependenciesFile());
        fs::path depFolder = depPath.parent_path();
        std::map<std::string, bool> conditionsMap;
        if (!m_options.override()) {
            // try to parse archived conditions file in project folder
            generator->parseConditionsFile(depFolder, conditionsMap);
            // try to parse generated conditions file in build platform folder
            generator->parseConditionsFile(depFolder/DepUtils::getBuildPlatformFolder(m_options), conditionsMap);
        }
        if (m_options.projectModeEnabled()) {
            m_options.setProjectRootPath(depFolder);
        }
        for (fs::directory_entry& x : fs::directory_iterator(depPath.parent_path())) {
            if (is_regular_file(x.path())) {
                if (x.path().extension() == ".pro") {
                    projectFolder = true;
                }
            }
        }
        if (!projectFolder) {
            BOOST_LOG_TRIVIAL(error)<<"packagedependencies.txt parent folder is not a project folder : no .pro file found in "<<depPath.parent_path().generic_string(utf8);
            return -1;
        }
        fs::path buildProjectSubFolderPath = DepUtils::getProjectBuildSubFolder(m_options);
        fs::path timestampPath = buildProjectSubFolderPath/ ".timestamp";
        std::chrono::duration nsDuration = std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::system_clock::now().time_since_epoch());
        // check timestamp exists
        if (fs::exists(timestampPath) && !m_options.force() && !m_options.override()) {
            std::time_t timestampFileTime = fs::last_write_time(timestampPath);
            std::time_t pkgDepFileTime = fs::last_write_time(depPath);

            if (timestampFileTime > pkgDepFileTime) {// timestamp wrote after last pkgdep file
                std::cout<<"=> Build options are already up to date: exiting !"<<std::endl;
                return 0;
            }
        }
        // timestamp obsolete or absent
        if (!m_options.force() && !m_options.override()) {
            std::cout<<"=> File " << depPath.generic_string(utf8) << " has recent changes: cleaning up and starting a fresh build configuration"<<std::endl;
        }
        else {
            std::cout<<"=> Force generation: cleaning up and starting a fresh build configuration ---------"<<std::endl;
        }

        fs::remove_all(buildProjectSubFolderPath);
        fs::create_directories(buildProjectSubFolderPath);
        fs::detail::utf8_codecvt_facet utf8;

        std::vector<Dependency> depsVect;
        DepUtils::parseRecurse(depPath, m_options, depsVect);
        std::vector<Dependency> dependencies = DepUtils::filterConditionDependencies(conditionsMap, depsVect);

        for (auto & dep : dependencies) {
            depsVectMap[dep.getType()].push_back(dep);
        }
        std::map<std::string,fs::path> setupInfoMap;
        if (mapContains(depsVectMap, Dependency::Type::CONAN)) {
            std::cout<<std::endl<<"=> Conan dependencies build information generation in progress ... please wait"<<std::endl;
            auto & depVect = depsVectMap[Dependency::Type::CONAN];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::CONAN, m_options, "conan");
            setupInfoMap.insert(fileRetriever->invokeGenerator(depVect));
        }
        if (mapContains(depsVectMap, Dependency::Type::REMAKEN)) {
            std::cout<<std::endl<<"=> Remaken dependencies build information generation in progress ... please wait"<<std::endl;
            auto & depVect = depsVectMap[Dependency::Type::REMAKEN];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::REMAKEN, m_options, "github");
            setupInfoMap.insert(fileRetriever->invokeGenerator(depVect));
        }
        if (mapContains(depsVectMap, Dependency::Type::BREW)) {
            std::cout<<std::endl<<"=> Brew dependencies build information generation in progress ... please wait"<<std::endl;
            auto & depVect = depsVectMap[Dependency::Type::BREW];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::BREW, m_options, "system");
            setupInfoMap.insert(fileRetriever->invokeGenerator(depVect));
        }
        if (mapContains(depsVectMap, Dependency::Type::SYSTEM)) {
            std::cout<<std::endl<<"=> System dependencies build information generation in progress ... please wait"<<std::endl;
            auto & depVect = depsVectMap[Dependency::Type::SYSTEM];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(Dependency::Type::SYSTEM, m_options, "system");
            setupInfoMap.insert(fileRetriever->invokeGenerator(depVect));
        }
        std::cout<<std::endl<<"=> Generating main dependenciesBuildInfo file"<<std::endl;
        generator->generateIndex(setupInfoMap);
        std::ofstream fos(timestampPath.generic_string(utf8),std::ios::out);
        int64_t nanoSeconds = nsDuration.count();
        fos<<std::to_string(nanoSeconds)<<std::endl;
        fos.close();
        generator->generateConfigureConditionsFile(depFolder, dependencies);
        std::cout<<"====> Configure done successfully !"<<std::endl;
        return 0;
    }
    return -1;
}
