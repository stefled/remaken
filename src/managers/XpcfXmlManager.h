/**
 * @copyright Copyright (c) 2019 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Loïc Touraine
 *
 * @file
 * @date 2021-02-24
 */

#ifndef XPCFXMLMANAGER_H
#define XPCFXMLMANAGER_H

#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Dependency.h"
#include "CmdOptions.h"
#include "Cache.h"
#include "tinyxmlhelper.h"

namespace fs = boost::filesystem;

class XpcfXmlManager
{
public:
    XpcfXmlManager(const CmdOptions & options);
    const std::map<std::string, fs::path> & parseXpcfModulesConfiguration(const fs::path & configurationFilePath);
    int updateXpcfModulesPath(const fs::path & configurationFilePath);
    static fs::path findPackageRoot(const fs::path & moduleLibPath);

private:
    void updateModuleNode(tinyxml2::XMLElement * xmlModuleElt);
    void declareModule(tinyxml2::XMLElement * xmlModuleElt);

    std::map<std::string, fs::path> m_modulesPathMap;
    std::map<std::string, std::string> m_modulesUUiDMap;
    const CmdOptions & m_options;

};

#endif
