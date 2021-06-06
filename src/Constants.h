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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#ifndef ROOTFOLDERENV
#define ROOTFOLDERENV "REMAKEN_PKG_ROOT"
#endif

class Constants {
public:
    static constexpr const char * REMAKENPKGROOT = ROOTFOLDERENV;
    static constexpr const char * REMAKENPKGFILENAME = ".packagespath";
    static constexpr const char * REMAKEN_FOLDER = ".remaken";
    static constexpr const char * REMAKEN_PROFILES_FOLDER = "profiles";
    static constexpr const char * REMAKEN_CACHE_FILE = ".remaken-cache";
    static constexpr const char * ARTIFACTORY_API_KEY = "artifactoryApiKey";
    static constexpr const char * QMAKE_RULES_DEFAULT_TAG = "4.6.3";
    static constexpr const char * PKGINFO_FOLDER = ".pkginfo";
    static constexpr const char * VCPKG_REPOURL = "https://github.com/microsoft/vcpkg";
};

typedef enum {
    cmake = 0x01,
    qmake = 0x20,
    pkg_config = 0x40,
} GeneratorType;

#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>


namespace fs = boost::filesystem;
#ifdef WIN32
#include <stdlib.h>
#else
#include <pwd.h>
#include <sys/types.h>
#endif


template < typename Key, typename T> bool mapContains(const std::map<Key,T> & mapContainer, Key k)
{
    if (mapContainer.find(k) != mapContainer.end()) {
        return true;
    }
    return false;
}

// following range find definitions must be removed once c++20 is here !
template<typename Range, typename Value>
typename Range::iterator find(Range& range, Value const& value)
{
    return std::find(begin(range), end(range), value);
}

template<typename Range, typename Value>
typename Range::const_iterator find(Range const& range, Value const& value)
{
    return std::find(begin(range), end(range), value);
}

#endif // CONSTANTS_H
