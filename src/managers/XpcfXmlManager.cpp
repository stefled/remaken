#include "XpcfXmlManager.h"
#include "Constants.h"
#include <list>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/dll.hpp>
#include <boost/algorithm/string.hpp>
#include <future>
#include <boost/log/trivial.hpp>
#include "utils/PathBuilder.h"
#include <regex>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

XpcfXmlManager::XpcfXmlManager(const CmdOptions & options):m_options(options)
{
}

fs::path XpcfXmlManager::findPackageRoot(const fs::path & moduleLibPath, bool verbose)
{
    fs::detail::utf8_codecvt_facet utf8;
    std::string versionRegex = "[0-9]+\\.[0-9]+\\.[0-9]+";
    fs::path currentFilename = moduleLibPath.filename();
    fs::path currentModulePath = moduleLibPath;
    bool bFoundVersion = false;
    std::smatch sm;
    while (!bFoundVersion && !currentModulePath.empty()) {
        std::regex tmplRegex(versionRegex, std::regex_constants::extended);
        std::string currentFilenameStr = currentFilename.string(utf8);
        if (std::regex_search(currentFilenameStr, sm, tmplRegex, std::regex_constants::match_any)) {
            std::string matchStr = sm.str(0);
            if (verbose) {
                BOOST_LOG_TRIVIAL(info)<<"Found "<< matchStr<<" version for modulepath "<<currentModulePath;
            }
            return currentModulePath;
        }
        else {
            currentModulePath = currentModulePath.parent_path();
            currentFilename = currentModulePath.filename();
        }
    }
    // no path found : return empty path
    return fs::path();
}

void XpcfXmlManager::updateModuleNode(tinyxml2::XMLElement * xmlModuleElt)
{
    fs::detail::utf8_codecvt_facet utf8;
    xmlModuleElt->SetAttribute("path",m_options.getModulesSubfolder().string(utf8).c_str());
}

int XpcfXmlManager::updateXpcfModulesPath(const fs::path & configurationFilePath)
{
    fs::detail::utf8_codecvt_facet utf8;
    int result = -1;
    tinyxml2::XMLDocument xmlDoc;
    enum tinyxml2::XMLError loadOkay = xmlDoc.LoadFile(configurationFilePath.string(utf8).c_str());
    if (loadOkay == 0) {
        try {
            //TODO : check each element exists before using it !
            // a check should be performed upon announced module uuid and inner module uuid
            // check xml node is xpcf-registry first !
            tinyxml2::XMLElement * rootElt = xmlDoc.RootElement();
            string rootName = rootElt->Value();
            if (rootName != "xpcf-registry" && rootName != "xpcf-configuration") {
                return -1;
            }
            result = 0;

            processXmlNode(rootElt, "module", std::bind(&XpcfXmlManager::updateModuleNode, this, _1));
            xmlDoc.SaveFile(configurationFilePath.string(utf8).c_str());
        }
        catch (const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(error)<<e.what();
            return -1;
        }
    }
    return result;
}

void XpcfXmlManager::declareModule(tinyxml2::XMLElement * xmlModuleElt)
{
    std::string moduleName = xmlModuleElt->Attribute("name");
    std::string moduleDescription = "";
    if (xmlModuleElt->Attribute("description") != nullptr) {
        moduleDescription = xmlModuleElt->Attribute("description");
    }
    std::string moduleUuid =  xmlModuleElt->Attribute("uuid");
    fs::path modulePath = PathBuilder::buildModuleFolderPath(xmlModuleElt->Attribute("path"), m_options);
    if (! mapContains(m_modulesUUiDMap, moduleName)) {
        m_modulesUUiDMap[moduleName] = moduleUuid;
    }
    else {
        std::string previousModuleUUID = m_modulesUUiDMap.at(moduleName);
        if (moduleUuid != previousModuleUUID) {
            BOOST_LOG_TRIVIAL(warning)<<"Already found a module named "<<moduleName<<" with a different UUID: first UUID found ="<<previousModuleUUID<<" last UUID = "<<moduleUuid;
        }
    }

    if (! mapContains(m_modulesPathMap, moduleName)) {
        m_modulesPathMap[moduleName] = modulePath;
    }
}


const std::map<std::string, fs::path> & XpcfXmlManager::parseXpcfModulesConfiguration(const fs::path & configurationFilePath)
{
    tinyxml2::XMLDocument doc;
    m_modulesPathMap.clear();
    enum tinyxml2::XMLError loadOkay = doc.LoadFile(configurationFilePath.string().c_str());
    if (loadOkay == 0) {
        try {
            //TODO : check each element exists before using it !
            // a check should be performed upon announced module uuid and inner module uuid
            // check xml node is xpcf-registry first !
            tinyxml2::XMLElement * rootElt = doc.RootElement();
            string rootName = rootElt->Value();
            if (rootName != "xpcf-registry" && rootName != "xpcf-configuration") {
                throw std::runtime_error("Error parsing xpcf configuration file : root node is neither <xpcf-registry> nor <xpcf-configuration> : invalid format ");
            }
            processXmlNode(rootElt, "module", std::bind(&XpcfXmlManager::declareModule, this, _1));
        }
        catch (const std::runtime_error & e) {
            BOOST_LOG_TRIVIAL(error)<<e.what();
            throw std::runtime_error("Error parsing xpcf configuration file : invalid format ");
        }
    }
    return m_modulesPathMap;
}



