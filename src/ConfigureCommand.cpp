#include "ConfigureCommand.h"
#include "utils/DepTools.h"
#include "XpcfXmlManager.h"
#include "utils/OsTools.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include "FileHandlerFactory.h"
#include <memory>
#include <map>
#include <vector>


namespace bp = boost::process;

using namespace std;

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

    if (!m_options.getDependenciesFile().empty()) {
        bool projectFolder = false;
        fs::path depPath = DepTools::buildDependencyPath(m_options.getDependenciesFile());
        std::map<std::string,bool> conditionsMap = DepTools::parseConditionsFile(depPath.parent_path());
        if (m_options.projectModeEnabled()) {
            m_options.setProjectRootPath(depPath.parent_path());
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
        std::vector<Dependency> depsVect;
        DepTools::parseRecurse(depPath, m_options, depsVect);
        std::vector<Dependency> dependencies = DepTools::filterConditionDependencies(conditionsMap, depsVect);
        std::vector<fs::path> generatedFiles;
        for (auto & dep : dependencies) {
            depsVectMap[dep.getType()].push_back(dep);
        }
        if (mapContains(depsVectMap, Dependency::Type::CONAN)) {
            BOOST_LOG_TRIVIAL(info)<<"Conan dependencies build information generation in progress ... please wait";
            auto & depVect = depsVectMap[Dependency::Type::CONAN];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(depVect[0], m_options);
            generatedFiles.push_back(fileRetriever->invokeGenerator(depVect));
        }
        if (mapContains(depsVectMap, Dependency::Type::BREW)) {
            BOOST_LOG_TRIVIAL(info)<<"Brew dependencies build information generation in progress ... please wait";
            auto & depVect = depsVectMap[Dependency::Type::BREW];
            shared_ptr<IFileRetriever> fileRetriever = FileHandlerFactory::instance()->getFileHandler(depVect[0], m_options);
            generatedFiles.push_back(fileRetriever->invokeGenerator(depVect));
        }

        fs::path buildSubFolderPath = DepTools::getProjectBuildSubFolder(m_options);
        fs::path conanFilePath = buildSubFolderPath / "dependenciesBuildInfo.pri"; // extension should later depend on generator type
        ofstream fos(conanFilePath.generic_string(utf8),ios::out);
        for (auto & file : generatedFiles) {
            fos<<"include($$_PRO_FILE_PWD_/build/$$OUTPUTDIR/"<<file.filename().generic_string(utf8)<<")\n";
        }
        fos.close();
        return 0;

    }

    return -1;
}
