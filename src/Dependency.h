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
#include <vector>
#include "CmdOptions.h"

class Dependency
{

public:
    enum class Type {
        REMAKEN = 0,
        CONAN,
        VCPKG,
        BREW,
        CHOCO,
        SCOOP,
        SYSTEM
    };

    explicit Dependency(const std::string & rawFormat, const std::string & mainMode);
    Dependency(const CmdOptions & options, const std::string & pkgName, const std::string & name, const std::string & version = "1.0.0", Type type = Type::SYSTEM);
    Dependency(const CmdOptions & options, const std::string & pkgName, const std::string & version = "1.0.0", Type type = Type::SYSTEM);
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

    inline void resetBaseRepository() {
        m_baseRepository = m_originalBaseRepository;
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


    inline bool hasConditions() const {
        return m_bHasConditions;
    }

    inline bool hasIdentifier() const {
        return m_bHasIdentifier;
    }

    inline const std::string & getToolOptions() const {
        return m_toolOptions;
    }

    inline const std::vector<std::string> & getConditions() const {
        return m_buildConditions;
    }

    const std::vector<std::string> & cflags() const {
        return m_cflags;
    }

    const std::vector<std::string> & libs() const {
        return m_libs;
    }

    std::vector<std::string> & cflags() {
        return m_cflags;
    }

    const std::string & prefix() const {
        return m_prefix;
    }

     std::string & prefix() {
        return m_prefix;
    }

    const std::vector<std::string> & libdirs() const {
        return m_libdirs;
    }

    std::vector<std::string> & libdirs() {
        return m_libdirs;
    }


    std::vector<std::string> & libs() {
        return m_libs;
    }

    bool isSystemDependency() const;
    bool isSpecificSystemToolDependency() const;
    bool isGenericSystemDependency() const;
    bool needsPriviledgeElevation() const;
    bool validate() const;

    friend std::ostream& operator<< (std::ostream& stream, const Dependency& dep);
    std::string toString() const;

    bool operator==(const Dependency& dep) const;

private:
    std::string parseConditions(const std::string & token);
    std::string m_packageName;
    std::string m_packageChannel;
    std::string m_name;
    std::string m_version;
    std::string m_baseRepository;
    std::string m_originalBaseRepository;
    std::string m_identifier;
    std::string m_repositoryType;
    std::string m_mode;
    std::string m_toolOptions;
    std::vector<std::string> m_buildConditions;
    std::vector<std::string> m_cflags;
    std::vector<std::string> m_libs;
    std::string m_prefix;
    std::vector<std::string> m_libdirs;
    Type m_type;
    bool m_bHasOptions = false;
    bool m_bHasConditions = false;
    bool m_bHasIdentifier = false;
};

std::string to_string(Dependency::Type type);

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
