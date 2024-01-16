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

#include "backends/CMakeGeneratorBackend.h"

#include <boost/log/trivial.hpp>

std::pair<std::string, fs::path> CMakeGeneratorBackend::generate([[maybe_unused]] const std::vector<Dependency> & deps, [[maybe_unused]] Dependency::Type depType)
{
    BOOST_LOG_TRIVIAL(warning)<<"CMakeGeneratorBackend::generate NOT IMPLEMENTED";
    return {"",fs::path()};
}

void CMakeGeneratorBackend::generateIndex([[maybe_unused]] std::map<std::string,fs::path> setupInfos)
{
    BOOST_LOG_TRIVIAL(warning)<<"CMakeGeneratorBackend::generateIndex NOT IMPLEMENTED";
}

void CMakeGeneratorBackend::generateConfigureConditionsFile([[maybe_unused]] const fs::path &  rootFolderPath, [[maybe_unused]] const std::vector<Dependency> & deps)
{
    BOOST_LOG_TRIVIAL(warning)<<"CMakeGeneratorBackend::generateConfigureConditionsFile NOT IMPLEMENTED";
}

void CMakeGeneratorBackend::parseConditionsFile([[maybe_unused]] const fs::path &  rootFolderPath, [[maybe_unused]] std::map<std::string,bool> & conditionsMap)
{
    BOOST_LOG_TRIVIAL(warning)<<"CMakeGeneratorBackend::parseConditionsFile NOT IMPLEMENTED";
}
