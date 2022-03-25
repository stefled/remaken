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

#ifndef IFILERETRIEVER_H
#define IFILERETRIEVER_H
#include <boost/filesystem.hpp>
#include <exception>
#include "Dependency.h"
#include "Constants.h"

namespace fs = boost::filesystem;

class IFileRetriever
{
public:
    virtual ~IFileRetriever() = default;
    virtual fs::path installArtefact(const Dependency & dependency) = 0;
    virtual fs::path bundleArtefact(const Dependency & dependency) = 0;
    virtual fs::path retrieveArtefact(const Dependency & dependency) = 0;
    virtual std::vector<fs::path> binPaths(const Dependency & dependency) = 0;
    virtual std::vector<fs::path> libPaths(const Dependency & dependency) = 0;
//    virtual std::vector<fs::path> libs(const Dependency & dependency) = 0;
//    virtual std::vector<fs::path> cflags(const Dependency & dependency) = 0;
    virtual std::string computeSourcePath( const Dependency &  dependency) = 0;
    virtual fs::path computeRootBinDir( const Dependency & dependency) = 0;
    virtual fs::path computeRootLibDir( const Dependency & dependency) = 0;
    virtual fs::path computeLocalDependencyRootDir( const Dependency & dependency) = 0;
    virtual const std::vector<Dependency> & installedDependencies() const = 0;
    virtual std::pair<std::string, fs::path> invokeGenerator(std::vector<Dependency> & deps) = 0;
};

#endif // IFILERETRIEVER_H
