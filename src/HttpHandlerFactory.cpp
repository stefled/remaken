#include "HttpHandlerFactory.h"

#include "CredentialsFileRetriever.h"
#include "FSFileRetriever.h"
#include "HttpFileRetriever.h"
#include <regex>

std::atomic<HttpHandlerFactory*> HttpHandlerFactory::m_instance;
std::mutex HttpHandlerFactory::m_mutex;
using namespace std;

HttpHandlerFactory * HttpHandlerFactory::instance()
{

    HttpHandlerFactory* fhInstance= m_instance.load(std::memory_order_acquire);
    if ( !fhInstance ){
        std::lock_guard<std::mutex> myLock(m_mutex);
        fhInstance = m_instance.load(std::memory_order_relaxed);
        if ( !fhInstance ){
            fhInstance= new HttpHandlerFactory();
            m_instance.store(fhInstance, std::memory_order_release);
        }
    }
    return fhInstance;
}

std::shared_ptr<AsioWrapper<httpRequestType,httpResponseType>> HttpHandlerFactory::getHttpHandler(boost::asio::io_context & ioc, const std::string & url)
{
    std::string httpRegexStr="^(http[s]*)://.*";
    std::regex httpRegex(httpRegexStr, std::regex_constants::extended);
    std::smatch sm;
    if (std::regex_search(url, sm, httpRegex)) {
        if (sm.str(1) == "http") {
            return make_shared<AsioSocketWrapper<httpRequestType,httpResponseType>>(ioc,url);
        }
        if (sm.str(1) == "https") {
            return make_shared<AsioStreamWrapper<httpRequestType,httpResponseType>>(ioc,url,m_ctx);
        }
    }
    throw std::runtime_error("Invalid source (neither http nor https) : " + url);
}
