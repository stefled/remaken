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

#ifndef BAZELGENERATORBACKEND_H
#define BAZELGENERATORBACKEND_H
#include "backends/IGeneratorBackend.h"

class BazelGeneratorBackend : virtual public AbstractGeneratorBackend
{
public:
    BazelGeneratorBackend(const CmdOptions & options):AbstractGeneratorBackend(options) {}
    ~BazelGeneratorBackend() override = default;
    fs::path generate(const std::vector<Dependency> & deps, Dependency::Type depType) override;
};

#endif // BAZELGENERATORBACKEND_H
