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

#ifndef SYSTEMFILERETRIEVER_H
#define SYSTEMFILERETRIEVER_H
#include <string>
#include "Constants.h"
#include "CmdOptions.h"
#include "AbstractFileRetriever.h"
#include "SystemTools.h"
#include <optional>

class SystemFileRetriever : public AbstractFileRetriever
{
public:
    SystemFileRetriever(const CmdOptions & options, std::optional<std::reference_wrapper<const Dependency>> dependencyOpt=std::nullopt);
    ~SystemFileRetriever() override = default;

    fs::path installArtefact(const Dependency & dependency) override;
    fs::path retrieveArtefact(const Dependency & dependency) override;
    std::string computeSourcePath( const Dependency &  dependency) override;

protected:
    std::shared_ptr<BaseSystemTool> m_tool;
};

#endif // SYSTEMFILERETRIEVER_H
