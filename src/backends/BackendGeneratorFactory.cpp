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

#include "backends/BackendGeneratorFactory.h"
#include "backends/BazelGeneratorBackend.h"
#include "backends/CMakeGeneratorBackend.h"
#include "backends/JSONGeneratorBackend.h"
#include "backends/QMakeGeneratorBackend.h"

namespace BackendGeneratorFactory {
std::shared_ptr<IGeneratorBackend> getGenerator(const CmdOptions & options)
{
    switch(options.getGenerator()) {
    case GeneratorType::bazel: return std::make_shared<BazelGeneratorBackend>(options);
        break;
    case GeneratorType::cmake: return std::make_shared<CMakeGeneratorBackend>(options);
        break;
    case GeneratorType::qmake: return std::make_shared<QMakeGeneratorBackend>(options);
        break;
    case GeneratorType::json: return std::make_shared<JSONGeneratorBackend>(options);
        break;
    default:
        throw std::runtime_error("Generator not imnplemented - generator support coming in future releases");
        break;
    }
}
}

