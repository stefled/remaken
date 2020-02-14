#include "FileHandlerFactory.h"
#include "CredentialsFileRetriever.h"
#include "ConanFileRetriever.h"
#include "FSFileRetriever.h"
#include "SystemFileRetriever.h"
#include "VCPKGFileRetriever.h"
#include "HttpFileRetriever.h"

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


std::shared_ptr<IFileRetriever> FileHandlerFactory::getFileHandler(const CmdOptions & options, bool useAlternateRepo)
{
     std::string repoType = options.getRepositoryType();
     if (useAlternateRepo) {
         repoType = options.getAlternateRepoType();
     }
    if (repoType == "github") {
        return make_shared<HttpFileRetriever>(options);
    }
    if ((repoType == "artifactory") || (repoType == "nexus")) {
        return make_shared<CredentialsFileRetriever>(options);
    }
    if (repoType == "path") {
        return make_shared<FSFileRetriever>(options);
    }
    // This should never happen, as command line options are validated in CmdOptions after parsing
    throw std::runtime_error("Unkwown repository type " + repoType);
}

std::shared_ptr<IFileRetriever> FileHandlerFactory::getFileHandler(const Dependency & dependency,const CmdOptions & options)
{
    if (dependency.getRepositoryType() == "github") {
        return make_shared<HttpFileRetriever>(options);
    }
    if ((dependency.getRepositoryType() == "artifactory") || (dependency.getRepositoryType() == "nexus")) {
        return make_shared<CredentialsFileRetriever>(options);
    }
    if (dependency.getRepositoryType() == "path") {
        return make_shared<FSFileRetriever>(options);
    }
    if (dependency.getRepositoryType() == "conan") {
        return make_shared<ConanFileRetriever>(options);
    }
    if (dependency.getRepositoryType() == "vcpkg") {
        return make_shared<VCPKGFileRetriever>(options);
    }
    if (dependency.getRepositoryType() == "system") {
        return make_shared<SystemFileRetriever>(options);
    }
    return getFileHandler(options);
}
