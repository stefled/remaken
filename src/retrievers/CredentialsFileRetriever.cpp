#include "CredentialsFileRetriever.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <fstream>
#include <iostream>
#include "HttpHandlerFactory.h"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;
using namespace std;
namespace ssl = boost::asio::ssl;


CredentialsFileRetriever::CredentialsFileRetriever(const CmdOptions & options):HttpFileRetriever (options)
{
    if (options.getApiKey().empty()){
        throw std::runtime_error("missing user api key for repository " );
    }
    m_apiKey = options.getApiKey();
}



http::status CredentialsFileRetriever::downloadArtefact (const std::string & source,const fs::path & dest, std::string & newLocation)
{
    //auto const host = "www.github.com";
    //auto const port = "443";
    boost::asio::io_context ioc;
    auto httpWrapper = HttpHandlerFactory::instance()->getHttpHandler(ioc,source);

    httpWrapper->connect();
    // Set up an HTTP GET request message
    //std::string target="/amc-generic-local/HOALib/2.0.2/packagedependencies.txt";
    httpRequestType req{http::verb::get, httpWrapper->getTarget(), m_version};
    req.set(http::field::host, httpWrapper->getHost());
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.insert("X-JFrog-Art-Api", m_apiKey.c_str());
    // Send the HTTP request to the remote host
    httpWrapper->write(req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;
    boost::system::error_code ec;

    // Declare a container to hold the response
    httpResponseType res;
    // Allow for an unlimited body size
    res.body_limit((std::numeric_limits<std::uint64_t>::max)());
    // Open the file the response parser will use to write to
    res.get().body().open(dest.generic_string().c_str(), boost::beast::file_mode::write, ec);

    // Receive the HTTP response
    httpWrapper->read(buffer, res);
    res.get().body().close();

    // Gracefully close the socket
    httpWrapper->shutdown();

    auto locationField = res.get().find("location");
    newLocation = locationField->value().to_string();
    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if(ec && ec != boost::system::errc::not_connected)
        throw boost::system::system_error{ec};

    // If we get here then the connection is closed gracefully
    return std::move(res.get().result());
}

fs::path CredentialsFileRetriever::retrieveArtefact(const std::string & source)
{
    // LOGGER.info(std::string.format("Download file %s", url));
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    fs::path output = this->m_workingDirectory / boost::uuids::to_string(uuid);
    std::string newUrl;
    http::status status = downloadArtefact(source,output,newUrl);
    if (status == http::status::not_found) {
        std::string updatedSource = m_options.getOS()+ "-" + m_options.getBuildToolchain() + "_" + source;
         status = downloadArtefact(source,output,newUrl);
         if (status == http::status::not_found) {
             updatedSource = m_options.getOS()+ "_" + source;
             status = downloadArtefact(source,output,newUrl);
         }
    }
    while (convertStatus(status) == HttpStatus::MOVED) {
        std::string newSource = newUrl;
        status = downloadArtefact(newSource,output,newUrl);
    }
    if (status != http::status::ok) {
        std::cout << source<<std::endl;
        throw std::runtime_error("Bad http response : http error code : " + std::to_string(static_cast<unsigned long>(status)));
    }
    return output;
}


