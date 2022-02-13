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

#ifndef IGENERATORBACKEND_H
#define IGENERATORBACKEND_H
#include <boost/filesystem.hpp>
#include <exception>
#include "Dependency.h"
#include "Constants.h"
#include "CmdOptions.h"
#include <boost/algorithm/string_regex.hpp>
#include "utils/DepUtils.h"

namespace fs = boost::filesystem;

class IGeneratorBackend
{
public:
    virtual ~IGeneratorBackend() = default;
    virtual fs::path generate(const std::vector<Dependency> & deps, Dependency::Type depType) = 0;
};


class AbstractGeneratorBackend : virtual public IGeneratorBackend {
public:
    AbstractGeneratorBackend(const CmdOptions & options): m_options(options) {}
    virtual ~AbstractGeneratorBackend() override = default;

protected:
    const CmdOptions & m_options;
};


#endif // IGENERATORBACKEND_H
