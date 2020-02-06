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
#define ROOTFOLDERENV "BCOMDEVROOT"
#endif

class Constants {
public:
    static constexpr const char * REMAKENDEVROOT = ROOTFOLDERENV;
    static constexpr const char * REMAKEN_FOLDER = ".remaken";
    static constexpr const char * REMAKEN_CACHE_FILE = ".remaken-cache";
    static constexpr const char * ARTIFACTORY_API_KEY = "artifactoryApiKey";
};

#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>


namespace fs = boost::filesystem;
#ifdef WIN32
#include <stdlib.h>
#else
#include <pwd.h>
#include <sys/types.h>
#endif

inline fs::path getUTF8PathObserver(const char * sourcePath)
{
    fs::detail::utf8_codecvt_facet utf8;
    fs::path utf8ObservedPath(sourcePath, utf8);
    return utf8ObservedPath;
}

inline fs::path getHomePath()
{
    char * homePathStr;
    fs::path homePath;
#ifdef WIN32
    homePathStr = getenv("USERPROFILE");
    if (homePathStr == nullptr) {
        homePathStr = getenv("HOMEDRIVE");
        if (homePathStr) {
            homePath = getUTF8PathObserver(homePathStr);
            homePath /= getenv("HOMEPATH");
        }
    }
    else {
        homePath = getUTF8PathObserver(homePathStr);
    }
#else
    struct passwd* pwd = getpwuid(getuid());
    if (pwd) {
        homePathStr = pwd->pw_dir;
    }
    else {
        // try the $HOME environment variable
        homePathStr = getenv("HOME");
    }
    homePath = getUTF8PathObserver(homePathStr);
#endif
    return homePath;
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
