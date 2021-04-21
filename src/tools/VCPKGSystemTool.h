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
 * @brief description of file
 * @date 2019-11-15
 */

#ifndef VCPKGSYSTEMTOOL_H
#define VCPKGSYSTEMTOOL_H

#include "SystemTools.h"

class VCPKGSystemTool : public BaseSystemTool
{
public:
    fs::detail::utf8_codecvt_facet utf8;
    VCPKGSystemTool(const CmdOptions & options):BaseSystemTool(options, options.getRemakenRoot().generic_string(utf8) + "/vcpkg/vcpkg") {}
    ~VCPKGSystemTool() override = default;
    void update() override;
    void install(const Dependency & dependency) override;
    bool installed(const Dependency & dependency) override;
    std::vector<std::string> binPaths(const Dependency & dependency) override;
    std::vector<std::string> libPaths(const Dependency & dependency) override;

protected:
    std::string computeToolRef( const Dependency &  dependency) override;
};


#endif
