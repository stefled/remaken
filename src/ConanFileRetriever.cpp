#include "ConanFileRetriever.h"
#include <boost/log/trivial.hpp>
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
*/

fs::path ConanFileRetriever::createConanFile(const fs::path & projectFolderPath, const std::vector<Dependency> & conanDeps)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path conanFilePath = projectFolderPath / "build";
    ofstream fos(conanFilePath.generic_string(utf8),ios::out);
    fos<<"[requires]"<<'\n';
    fos.close();
    return conanFilePath;
}

void ConanFileRetriever::invokeGenerator(const fs::path & conanFilePath, const fs::path &  projectFolderPath, ConanFileRetriever::GeneratorType generator)
{

}
