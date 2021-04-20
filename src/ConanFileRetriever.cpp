#include "ConanFileRetriever.h"
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
using namespace std;

ConanFileRetriever::ConanFileRetriever(const CmdOptions & options):SystemFileRetriever (options)
{
    m_tool = std::make_shared<ConanSystemTool>(options);
}

fs::path ConanFileRetriever::bundleArtefact(const Dependency & dependency)
{
    m_tool->bundle(dependency);
    fs::path outputDirectory = computeLocalDependencyRootDir(dependency);
    return outputDirectory;
}
/*
 *             equals(pkg.repoType,"conan") {# conan system package handling
                message("    --> ["$${pkg.repoType}"] adding " $${pkg.name} " dependency")
                #use url format according to remote as conan-center index urls are now without '@user/channel' suffix
                equals(pkg.repoUrl,conan-center) {
                    remakenConanDeps += $${pkg.name}/$${pkg.version}
                } else {
                    remakenConanDeps += $${pkg.name}/$${pkg.version}@$${pkg.identifier}/$${pkg.channel}
                }
                sharedLinkMode = False
                equals(pkg.linkMode,shared) {
                    sharedLinkMode = True
                }
                !equals(pkg.linkMode,na) {
                   remakenConanOptions += $${pkg.name}:shared=$${sharedLinkMode}
                }
                    conanOptions = $$split(pkg.toolOptions, $$LITERAL_HASH)
                    for (conanOption, conanOptions) {
                        conanOptionInfo = $$split(conanOption, :)
                        conanOptionPrefix = $$take_first(conanOptionInfo)
                        isEmpty(conanOptionInfo) {
                            remakenConanOptions += $${pkg.name}:$${conanOption}
                        }
                        else {
                            remakenConanOptions += $${conanOption}
                        }
                    }
                }

    #create conanfile.txt
    CONANFILECONTENT="[requires]"
    for (dep,remakenConanDeps) {
        CONANFILECONTENT+=$${dep}
    }
    CONANFILECONTENT+=""
    CONANFILECONTENT+="[generators]"
    CONANFILECONTENT+="qmake"
    CONANFILECONTENT+=""
    CONANFILECONTENT+="[options]"
    for (option,remakenConanOptions) {
        CONANFILECONTENT+=$${option}
    }
    conan install $$_PRO_FILE_PWD_/build/$$OUTPUTDIR/conanfile.txt -s $${conanArch} -s compiler.cppstd=$${conanCppStd} -s build_type=$${CONANBUILDTYPE} --build=missing -if $$_PRO_FILE_PWD_/build/$$OUTPUTDIR
*/
std::vector<std::string> ConanFileRetriever::buildOptions(const Dependency & dep)
{
    std::vector<std::string> options;
    std::vector<std::string> results;
    if (dep.getMode() != "na") {
        std::string modeValue = "False";
        if (dep.getMode() == "shared") {
            modeValue = "True";
        }
        results.push_back(dep.getPackageName() + ":" + dep.getMode() + "=" + modeValue);
    }
    if (dep.hasOptions()) {
        boost::split(options, dep.getToolOptions(), [](char c){return c == '#';});
        for (const auto & option: options) {
            results.push_back(dep.getPackageName() + ":" + option);
        }
    }
    return results;
}

fs::path ConanFileRetriever::createConanFile(const fs::path & projectFolderPath)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path conanFilePath = projectFolderPath / "build" / m_options.getConfig() / "conanfile.txt";
    ofstream fos(conanFilePath.generic_string(utf8),ios::out);
    fos<<"[requires]"<<'\n';
    for (auto & dependency : m_installedDeps) {
        std::string sourceURL = dependency.getPackageName();
        sourceURL += "/" + dependency.getVersion();
        fos<<sourceURL<<'\n';
    }
    fos<<'\n';
    fos<<"[generators]"<<'\n';
    fos<<"qmake"<<'\n';
    fos<<"\n";
    fos<<"[options]"<<'\n';
    for (auto & dependency : m_installedDeps) {
        for (auto & option : buildOptions(dependency)) {
            fos<<option<<'\n';
        }
    }
    fos.close();
    return conanFilePath;
}

void ConanFileRetriever::invokeGenerator(const fs::path & conanFilePath, ConanSystemTool::GeneratorType generator)
{
    ConanSystemTool tool(m_options);
    tool.invokeGenerator(conanFilePath, generator);
//conan install $$_PRO_FILE_PWD_/build/$$OUTPUTDIR/conanfile.txt -s $${conanArch} -s compiler.cppstd=$${conanCppStd} -s build_type=$${CONANBUILDTYPE} --build=missing -if $$_PRO_FILE_PWD_/build/$$OUTPUTDIR
}

void ConanFileRetriever::processPostInstallActions()
{
    if (m_options.projectModeEnabled()) {
        fs::path conanFilePath = createConanFile(m_options.getProjectRootPath());
        invokeGenerator(conanFilePath, ConanSystemTool::GeneratorType::qmake);
    }
}
