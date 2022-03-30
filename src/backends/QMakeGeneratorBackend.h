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

#ifndef QMAKEGENERATORBACKEND_H
#define QMAKEGENERATORBACKEND_H
#include "backends/IGeneratorBackend.h"

class QMakeGeneratorBackend : virtual public AbstractGeneratorBackend
{
public:
    QMakeGeneratorBackend(const CmdOptions & options):AbstractGeneratorBackend(options,".pri") {}
    ~QMakeGeneratorBackend() override = default;
    std::pair<std::string, fs::path> generate(const std::vector<Dependency> & deps, Dependency::Type depType) override;
    void generateIndex(std::map<std::string,fs::path> setupInfos) override;
    void generateConfigureConditionsFile(const fs::path &  rootFolderPath, const std::vector<Dependency> & deps) override;
    void parseConditionsFile(const fs::path &  rootFolderPath, std::map<std::string,bool> & conditionsMap) override;
};

#endif // QMAKEGENERATORBACKEND_H
