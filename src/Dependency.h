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

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include <string>
#include <sstream>

class Dependency
{

public:
    enum class Type {
        REMAKEN = 0,
        CONAN,
        VCPKG,
        SYSTEM
    };

    explicit Dependency(const std::string & rawFormat, const std::string & mainMode);
    // Dependency(Dependency && dependency) = default;

    inline const std::string & getPackageName() const  {
        return m_packageName;
    }

    inline const std::string & getName() const  {
        return m_name;
    }

    inline  const std::string & getVersion() const  {
        return m_version;
    }

    inline  const std::string & getChannel() const  {
        return m_packageChannel;
    }

    inline const std::string & getIdentifier() const  {
        return m_identifier;
    }

    inline const std::string & getBaseRepository() const  {
        return m_baseRepository;
    }

    inline void changeBaseRepository(const std::string & otherRepo) {
        m_baseRepository = otherRepo;
    }

    inline const std::string & getMode() const  {
        return m_mode;
    }

    inline const std::string & getRepositoryType() const  {
        return m_repositoryType;
    }

    inline Type getType() const {
        return m_type;
    }

    inline bool hasOptions() const {
        return m_bHasOptions;
    }

    inline bool hasIdentifier() const {
        return m_bHasIdentifier;
    }


    inline const std::string & getToolOptions() const {
        return m_toolOptions;
    }

    bool validate();

    friend std::ostream& operator<< (std::ostream& stream, const Dependency& dep);

private:
    std::string m_packageName;
    std::string m_packageChannel;
    std::string m_name;
    std::string m_version;
    std::string m_baseRepository;
    std::string m_identifier;
    std::string m_repositoryType;
    std::string m_mode;
    std::string m_toolOptions;
    Type m_type;
    bool m_bHasOptions = false;
    bool m_bHasIdentifier = false;
};

template <typename T>
std::string log(const T & t);

template <>
inline std::string log(const Dependency & dep)
{
    std::ostringstream sstr;
    sstr<<"Dependency =>"<<std::endl;
    sstr<<"    "<<"name="<<dep.getName()<<std::endl;
    sstr<<"    "<<"version="<<dep.getVersion()<<std::endl;
    sstr<<"    "<<"type="<<dep.getRepositoryType()<<std::endl;
    sstr<<"    "<<"link mode="<<dep.getMode()<<std::endl;
    sstr <<"    "<<"identifier="<<dep.getIdentifier()<<std::endl;
    sstr<<"    "<<"channel="<<dep.getChannel()<<std::endl;
    return sstr.str();
}

#endif // DEPENDENCY_H
