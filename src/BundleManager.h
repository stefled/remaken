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

#ifndef BUNDLEMANAGER_H
#define BUNDLEMANAGER_H

#include <string>
#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Dependency.h"
#include "CmdOptions.h"
#include "Cache.h"
#include "tinyxmlhelper.h"
#include "XpcfXmlManager.h"

namespace fs = boost::filesystem;

class BundleManager
{
public:
    BundleManager(const CmdOptions & options);
    fs::path buildDependencyPath();
    int bundle();
    int bundleXpcf();

private:
    XpcfXmlManager m_xpcfManager;
    void parseIgnoreInstall(const fs::path &  dependenciesPath);

    void bundleDependencies(const fs::path & dependenciesFiles);
    void bundleDependency(const Dependency & dep);
    std::map<std::string,bool> m_ignoredPackages;
    const CmdOptions & m_options;

};

#endif
