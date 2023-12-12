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
    std::string finalRepo = repo;
    if (repo == "system") {
        if (depType != Dependency::Type::SYSTEM) {
            finalRepo = to_string(depType);
        }
    }
    if (!mapContains(m_handlers, finalRepo)) {
        if ((finalRepo == "github") ||
            (finalRepo == "http")) {
            m_handlers[finalRepo] = make_shared<HttpFileRetriever>(options);
        }
        if ((finalRepo == "artifactory") || (finalRepo == "nexus")) {
            m_handlers[finalRepo] = make_shared<CredentialsFileRetriever>(options);
        }
        if (finalRepo == "path") {
            m_handlers[finalRepo] = make_shared<FSFileRetriever>(options);
        }
        if (finalRepo == "conan") {
            m_handlers[repo] = make_shared<ConanFileRetriever>(options);
        }
        if (finalRepo == "vcpkg") {
            m_handlers[finalRepo] = make_shared<SystemFileRetriever>(options, depType);
        }
        if (repo == "system") {
            m_handlers[finalRepo] = make_shared<SystemFileRetriever>(options, depType);
        }
    }
    if (mapContains(m_handlers, finalRepo)) {
        return m_handlers.at(finalRepo);
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
        //throw std::runtime_error("Unknown repository type " + repoType);
        std::cout << "[INFO]: No alternate handler found" << std::endl;
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
