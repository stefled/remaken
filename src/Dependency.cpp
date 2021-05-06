#include "Dependency.h"
#include "tools/SystemTools.h"
#include "Constants.h"
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

const std::map<std::string,Dependency::Type> str2type = {
    {"bcomBuild",Dependency::Type::REMAKEN},
    {"thirdParties",Dependency::Type::REMAKEN},
    {"artifactory",Dependency::Type::REMAKEN},
    {"nexus",Dependency::Type::REMAKEN},
    {"github",Dependency::Type::REMAKEN},
    {"path",Dependency::Type::REMAKEN},
    {"conan",Dependency::Type::CONAN},
    {"system",Dependency::Type::SYSTEM},
    {"vcpkg",Dependency::Type::VCPKG},
    {"brew",Dependency::Type::BREW},
    {"choco",Dependency::Type::SYSTEM},
    {"scoop",Dependency::Type::SCOOP},
    {"apt-get",Dependency::Type::SYSTEM},
    {"yum",Dependency::Type::SYSTEM}
};

Dependency::Type deduceType(const std::string typeStr)
{
    if (str2type.find(typeStr) != str2type.end()) {
        return str2type.at(typeStr);
    }
    return Dependency::Type::REMAKEN;
}

std::string stripEndlineChar(const std::string & str)
{
    std::string strippedStr = str;
    if (!strippedStr.empty() && strippedStr[strippedStr.size() - 1] == '\r') {
        strippedStr.erase(strippedStr.size() - 1);
    }
    return strippedStr;
}

static const std::map<std::string,std::string> identifier2repoType = {
    {"bcomBuild","artifactory"},
    {"thirdParties","artifactory"},
    {"apt-get","system"},
    {"brew","system"},
    {"choco","system"},
    {"scoop","system"},
    {"pacman","system"},
    {"pkg","system"},
    {"pkgutil","system"},
    {"yum","system"},
    {"zypper","system"}
};


std::ostream& operator<< (std::ostream& stream, const Dependency& dep)
{
    stream << dep.getName() << dep.getVersion() << '\n';
    return stream;
}

std::string Dependency::toString() const
{
    std::string str = getPackageName() + "|" + getVersion() + "|" + getName() + "|" + getIdentifier() + "@" + getRepositoryType() + "|" + getBaseRepository() + "|" + getMode() + "|" + getToolOptions();
    return str;
}

bool Dependency::isSystemDependency() const
{
    return (m_repositoryType == "system");
}

bool Dependency::isSpecificSystemToolDependency() const
{
    return isSystemDependency() && ! isGenericSystemDependency() && (m_identifier != SystemTools::getToolIdentifier()) && (SystemTools::isToolSupported(m_identifier));
}

bool Dependency::isGenericSystemDependency() const
{
    return isSystemDependency() && ((m_identifier == "system") || (m_identifier == SystemTools::getToolIdentifier()));
}

bool Dependency::needsPriviledgeElevation() const
{
#ifdef BOOST_OS_WINDOWS_AVAILABLE
    return (isSystemDependency() && SystemTools::getToolIdentifier() == "choco");
#endif
    return false;
}

std::string Dependency::parseConditions(const std::string & token)
{
    std::vector<std::string> conditions;
    std::string firstPart;
    boost::split(conditions, token, [](char c){return c == '%';});
    firstPart = conditions[0];
    if (conditions.size() >= 2) {
        m_bHasConditions = true;
        for (uint32_t i = 1 ; i< conditions.size() ; i++) {
            m_buildConditions.push_back(conditions[i]);
        }
    }
    return firstPart;
}


Dependency::Dependency(const std::string & rawFormat, const std::string & mainMode):m_packageChannel("stable"),m_repositoryType("github")
{
    std::vector<std::string> results, pkgInformations, repositoryInformations, conditions;
    boost::split(results, rawFormat, [](char c){return c == '|';});
    boost::split(pkgInformations, results[0], [](char c){return c == '#';});
    boost::split(repositoryInformations, results[3], [](char c){return c == '@';});


    if (pkgInformations.size() >= 2) {
        m_packageName = pkgInformations[0];
        m_packageChannel = parseConditions(pkgInformations[1]);
        boost::trim(m_packageChannel);
    }
    else {
        m_packageName = parseConditions(pkgInformations[0]);
        boost::trim(m_packageName);
    }
    m_version = results[1];
    boost::trim(m_version);
    m_name = parseConditions(results[2]);
    boost::trim(m_name);
    m_identifier = repositoryInformations[0];
    boost::trim(m_identifier);
    m_repositoryType = m_identifier;
    if (identifier2repoType.find(m_identifier) != identifier2repoType.end()) {
        m_repositoryType = identifier2repoType.at(m_identifier);
    } // should lead to an error when no transcription exists ? in validate ??
    if (repositoryInformations.size() >= 2) {
        m_repositoryType = repositoryInformations[1];
        boost::trim(m_repositoryType);
        m_bHasIdentifier = !m_identifier.empty();
    }
    m_type = deduceType(m_repositoryType);

    if (m_identifier != m_repositoryType) {
        m_bHasIdentifier = !m_identifier.empty();
        m_type = deduceType(m_identifier);
    }

    if (results.size() < 5) {
        if ((m_repositoryType == "conan") ||
            (m_repositoryType == "system") ||
            (m_repositoryType == "vcpkg")) {
                m_baseRepository = m_repositoryType;
        }
    }
    else {
        m_baseRepository = stripEndlineChar(results[4]);
        boost::trim(m_baseRepository);
        if ((m_baseRepository.empty()) &&
            ((m_repositoryType == "conan") ||
            (m_repositoryType == "system") ||
            (m_repositoryType == "vcpkg"))) {
                m_baseRepository = m_repositoryType;
        }
    }

    if (m_baseRepository.find("https://github") != std::string::npos) {// github url
        if ((m_repositoryType == "artifactory") &&
                ((m_identifier == "bcomBuild") ||
                (m_identifier == "thirdParties"))) { // erroneous deduction : repository type maybe a github repository
            m_repositoryType = "github";
        }
    }

    if (results.size() >= 6){
        m_mode = stripEndlineChar(results[5]);
        boost::trim(m_mode);
        if ((m_mode!= "static" && m_mode!= "shared" && m_mode!= "na") ||  (m_mode == "default")){
             m_mode = mainMode;
        }
    }
    else {
        m_mode = mainMode;
        /*if (m_type == Dependency::Type::CONAN) {// before conan mode was set to na
            m_mode = "na"; // should detect supported options
        }*/
    }
    if (results.size() == 7){
        m_toolOptions = stripEndlineChar(results[6]);
        if (!m_toolOptions.empty()) {
            m_bHasOptions = true;
        }
    }
}


static const std::vector<std::string> repoValidation = {"artifactory","github","nexus","path","vcpkg","conan","system"};
static const std::vector<std::string> linkModeValidation = {"static","shared","default","na"};
static const std::map<std::string,std::vector<std::string>> unsupportedLinkModeRelations = {{"na",{"artifactory","nexus","github","path","vcpkg"}}};
static const std::vector<std::string> systemIdentifierMap = {"system","apt-get","brew","yum","choco","scoop","pkg", "pkgutil", "pacman", "zypper"};

bool Dependency::validate() const
{
    if (find(repoValidation,m_repositoryType) == std::end(repoValidation)) {
        BOOST_LOG_TRIVIAL(error)<<"Dependency file error : invalid repository type : unsupported value= "<< m_repositoryType<<std::endl<<log(*this);
        return false;
    }

    if (m_baseRepository.empty()) {
        BOOST_LOG_TRIVIAL(error)<<"Dependency file error : missing repository url for "<<m_packageName<<"/"<< m_version<<" package"<<std::endl<<log(*this);
    }

    if (m_repositoryType == "system") {
        if (find(systemIdentifierMap,m_identifier) == std::end(systemIdentifierMap)) {
            BOOST_LOG_TRIVIAL(error)<<"Dependency file error : invalid tool identifier  value= "<<m_identifier<<" for "<< m_repositoryType<<" repository"<<std::endl<<log(*this);
            return false;
        }
    }
    else {
        if (find(systemIdentifierMap,m_identifier) != std::end(systemIdentifierMap)) {
            BOOST_LOG_TRIVIAL(error)<<"Dependency file error : invalid identifier value= "<<m_identifier<<" for "<< m_repositoryType<<" repository"<<std::endl<<log(*this);
            return false;
        }
    }
    if (find(linkModeValidation,m_mode) == std::end(linkModeValidation)) {
        BOOST_LOG_TRIVIAL(error)<<"Dependency file error : invalid link mode value= "<<m_mode<<std::endl<<log(*this);
        return false;
    }
    for (auto & element:unsupportedLinkModeRelations) {
        if (m_mode == element.first) {
            auto & unsupportedRepolist = element.second;
            if (find(unsupportedRepolist,m_repositoryType) != std::end(unsupportedRepolist)) {
                BOOST_LOG_TRIVIAL(error)<<"Dependency file error : invalid link mode= "<<m_mode<<" for "<< m_repositoryType<<" repository"<<std::endl<<log(*this);
                return false;
            }
        }
    }
    return true;
}

