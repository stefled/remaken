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

#ifndef CACHE_H
#define CACHE_H

#include "Constants.h"
#include "CmdOptions.h"
#include <list>
#include <atomic>
#include <mutex>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class Cache
{
public:
    Cache(const CmdOptions & options);
    ~Cache();
    bool contains(std::string url);
    void add(std::string url);
    void remove(std::string url);

private:
    void write(std::list<std::string> data);
    void load();
    fs::path m_cacheFile;
    std::list<std::string> m_cachedUrls;
    std::atomic<bool> m_loaded = false;
    std::mutex m_mutex;
};

#endif // CACHE_H
