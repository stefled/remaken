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

#ifndef FILEHANDLERFACTORY_H
#define FILEHANDLERFACTORY_H
#include "IFileRetriever.h"
#include "CmdOptions.h"
#include "Dependency.h"
#include <atomic>
#include <memory>
#include <mutex>

class FileHandlerFactory
{
    public:
        static FileHandlerFactory* instance();
        std::shared_ptr<IFileRetriever> getAlternateHandler(const Dependency & dependency,const CmdOptions & options);
        std::shared_ptr<IFileRetriever> getFileHandler(const Dependency & dependency,const CmdOptions & options);

private:
        FileHandlerFactory() = default;
        ~FileHandlerFactory() = default;
        FileHandlerFactory(const FileHandlerFactory&)= delete;
        FileHandlerFactory& operator=(const FileHandlerFactory&)= delete;
        FileHandlerFactory(const FileHandlerFactory&&)= delete;
        FileHandlerFactory&& operator=(const FileHandlerFactory&&)= delete;
        static std::atomic<FileHandlerFactory*> m_instance;
        static std::mutex m_mutex;
        std::shared_ptr<IFileRetriever> getHandler(const Dependency & dependency, const CmdOptions & options, const std::string & repo);
        std::map<std::string,std::shared_ptr<IFileRetriever>> m_handlers;
};


#endif // FILEHANDLERFACTORY_H
