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
    virtual std::pair<std::string, fs::path> generate(const std::vector<Dependency> & deps, Dependency::Type depType) = 0;
    virtual void generateIndex(std::map<std::string,fs::path> setupInfos) = 0;
    virtual void generateConfigureConditionsFile(const fs::path &  rootFolderPath, const std::vector<Dependency> & deps) = 0;
    virtual void parseConditionsFile(const fs::path &  rootFolderPath, std::map<std::string,bool> & conditionsMap) = 0;
    virtual void forceConditions(std::map<std::string,bool> & conditionsMap) = 0;
};


class AbstractGeneratorBackend : virtual public IGeneratorBackend {
public:
    AbstractGeneratorBackend(const CmdOptions & options, const std::string & extension): m_options(options), m_extension(extension) {}
    virtual ~AbstractGeneratorBackend() override = default;

    void forceConditions(std::map<std::string,bool> & conditionsMap) override
    {
        // Manage force configure conditions by command line
        for (auto & arg : m_options.getConfigureConditions()) {
            std::vector<std::string> condition;
            boost::split(condition, arg, [](char c){return c == '=';});
            std::string conditionName = condition[0];
            std::string conditionValue;
            if (condition.size() >= 2) {
                conditionValue = condition[1];
            }
            if (!conditionName.empty() && !conditionValue.empty()) {
                if (boost::iequals(conditionValue, "true")) {
                    conditionsMap.insert_or_assign(conditionName, true);
                }
                else if (boost::iequals(conditionValue, "false")) {
                    conditionsMap.insert_or_assign(conditionName, false);
                }
            }
        }
    }

protected:
    std::string getGeneratorFileName(const std::string & file) const
    {
        std::string fileName = file + m_extension;
        return fileName;
    }

    const CmdOptions & m_options;
    std::string m_extension;
};


#endif // IGENERATORBACKEND_H
