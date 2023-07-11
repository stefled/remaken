#include "ListCommand.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "utils/DepUtils.h"
#include "utils/OsUtils.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <regex>

namespace bp = boost::process;

ListCommand::ListCommand(const CmdOptions & options):AbstractCommand(ListCommand::NAME),m_options(options)
{
}

int ListCommand::listPackages()
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPackagesPath = OsUtils::computeRemakenRootPackageDir(m_options);
    if (!fs::exists(remakenRootPackagesPath)) {
        return -1;
    }
    for (fs::directory_entry& x : fs::recursive_directory_iterator(remakenRootPackagesPath)) {
        if (fs::is_directory(x.path())) {
            std::string versionRegexStr = "[0-9]+\\.[0-9]+\\.[0-9]+";

            std::regex versionRegex(versionRegexStr, std::regex_constants::extended);
            fs::path leafFolder = x.path().filename();
            std::string leafFolderStr = leafFolder.generic_string(utf8);
            std::string parentFolderStr = x.path().parent_path().filename().generic_string(utf8);
            std::smatch sm;
            if (std::regex_search(leafFolderStr, sm, versionRegex, std::regex_constants::match_any)) {
                std::cout<< parentFolderStr<<":"<<leafFolderStr<<std::endl;
                if (m_options.treeEnabled()) {
                    DepUtils::readInfos(x.path()/"packagedependencies.txt", m_options);
                    std::cout<<std::endl;
                }
            }
        }
    }

    return 0;
}

int ListCommand::listPackageVersions(const std::string & pkgName)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPackagesPath = OsUtils::computeRemakenRootPackageDir(m_options);
    if (!fs::exists(remakenRootPackagesPath)) {
        return -1;
    }
    for (fs::directory_entry& x : fs::recursive_directory_iterator(remakenRootPackagesPath)) {
        if (fs::is_directory(x.path())) {
            std::string versionRegexStr = "[0-9]+\\.[0-9]+\\.[0-9]+";

            std::regex versionRegex(versionRegexStr, std::regex_constants::extended);

            fs::path leafFolder = x.path().filename();
            std::string leafFolderStr = leafFolder.generic_string(utf8);
            std::string parentFolderStr = x.path().parent_path().filename().generic_string(utf8);
            std::smatch sm;
            bool pkgCondition;
            if (m_options.regexEnabled()) {
                std::regex pkgRegex(pkgName, std::regex_constants::extended);
                pkgCondition = std::regex_search(parentFolderStr, sm, pkgRegex, std::regex_constants::match_any);
            }
            else {
                pkgCondition = (parentFolderStr == pkgName);

            }
            if (std::regex_search(leafFolderStr, sm, versionRegex, std::regex_constants::match_any)) {
                if (pkgCondition) {
                    std::cout<< parentFolderStr <<":"<<leafFolder.generic_string(utf8)<<std::endl;
                    if (m_options.treeEnabled()) {
                        DepUtils::readInfos(x.path()/"packagedependencies.txt", m_options);
                        std::cout<<std::endl;
                    }
                }
            }
        }
    }
    return 0;
}

int ListCommand::listPackageFiles(const std::string & pkgName, const std::string & pkgVersion)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path remakenRootPackagesPath = OsUtils::computeRemakenRootPackageDir(m_options);
    if (!fs::exists(remakenRootPackagesPath)) {
        return -1;
    }
    fs::path pkgPath = DepUtils::findPackageFolder(m_options, pkgName, pkgVersion);
    if (!pkgPath.empty()) {
        if (m_options.treeEnabled()) {
            DepUtils::readInfos(pkgPath/"packagedependencies.txt", m_options);
        }
        std::cout<<std::endl;
        for (fs::directory_entry& pathElt : fs::recursive_directory_iterator(pkgPath)) {
            if (fs::is_regular_file(pathElt.path())) {
                std::cout<< pathElt.path().generic_string(utf8)<<std::endl;
            }
        }
    }
    return 0;
}

int ListCommand::execute()
{
    fs::detail::utf8_codecvt_facet utf8;

    std::map<std::string,std::string> listOptions = m_options.getListCommandOptions();

    if (listOptions.at("pkgName").empty()) {
        return listPackages();
    }
    if (listOptions.at("pkgVersion").empty()) {
        return listPackageVersions(listOptions.at("pkgName"));
    }
    return listPackageFiles(listOptions.at("pkgName"), listOptions.at("pkgVersion"));
}
