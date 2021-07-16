#include "FileHandlerFactory.h"
#include "retrievers/CredentialsFileRetriever.h"
#include "retrievers/ConanFileRetriever.h"
#include "retrievers/FSFileRetriever.h"
#include "retrievers/SystemFileRetriever.h"
#include "retrievers/HttpFileRetriever.h"
#include <chrono>

std::atomic<FileHandlerFactory*> FileHandlerFactory::m_instance;
std::mutex FileHandlerFactory::m_mutex;
using namespace std;

FileHandlerFactory * FileHandlerFactory::instance()
{

    FileHandlerFactory* fhInstance = m_instance.load(std::memory_order_acquire);
    if ( !fhInstance ){
        std::lock_guard<std::mutex> myLock(m_mutex);
        fhInstance = m_instance.load(std::memory_order_relaxed);
        if ( !fhInstance ){
            fhInstance= new FileHandlerFactory();
            m_instance.store(fhInstance, std::memory_order_release);
        }
    }
    return fhInstance;
}


std::shared_ptr<IFileRetriever> FileHandlerFactory::getHandler(Dependency::Type depType, const CmdOptions & options, const std::string & repo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!mapContains(m_handlers, repo)) {
        if ((repo == "github") ||
            (repo == "http")) {
            m_handlers[repo] = make_shared<HttpFileRetriever>(options);
        }
        if ((repo == "artifactory") || (repo == "nexus")) {
            m_handlers[repo] = make_shared<CredentialsFileRetriever>(options);
        }
        if (repo == "path") {
            m_handlers[repo] = make_shared<FSFileRetriever>(options);
        }
        if (repo == "conan") {
            m_handlers[repo] = make_shared<ConanFileRetriever>(options);
        }
        if (repo == "vcpkg" || repo == "system") {
            m_handlers[repo] = make_shared<SystemFileRetriever>(options, depType);
        }
    }
    if (mapContains(m_handlers, repo)) {
        return m_handlers.at(repo);
    }
    return nullptr;
}

std::shared_ptr<IFileRetriever> FileHandlerFactory::getAlternateHandler(Dependency::Type depType,const CmdOptions & options)
{
    std::string repoType = options.getAlternateRepoType();
    std::shared_ptr<IFileRetriever> retriever;
    if (!repoType.empty()) {
        retriever = getHandler(depType, options, repoType);
    }
    if (!retriever) {
        // This should never happen, as command line options are validated in CmdOptions after parsing
        throw std::runtime_error("Unknown repository type " + repoType);
    }
    return retriever;
}

std::shared_ptr<IFileRetriever> FileHandlerFactory::getFileHandler(Dependency::Type depType,const CmdOptions & options, const std::string & repo)
{
    std::shared_ptr<IFileRetriever> retriever = getHandler(depType, options, repo);
    if (!retriever) {
        // This should never happen, as command line options are validated in CmdOptions after parsing
        throw std::runtime_error("Unknown repository type " + repo);
    }
    return retriever;
}

std::shared_ptr<IFileRetriever> FileHandlerFactory::getFileHandler(const Dependency & dependency,const CmdOptions & options)
{
    return getFileHandler(dependency.getType(), options, dependency.getRepositoryType());
}
