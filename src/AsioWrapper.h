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

#ifndef BOOSTASIOWRAPPER_H
#define BOOSTASIOWRAPPER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/connect.hpp>
#include <regex>
#include "root_certificates.hpp"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;

template < typename Q, typename R>
class AsioWrapper {
public:
    AsioWrapper(boost::asio::io_context & ioc, const std::string &url);
    virtual ~AsioWrapper() = default;
    virtual void connect() = 0;
    virtual void shutdown() = 0;
    virtual std::size_t write(Q & req) = 0;
    virtual std::size_t read(boost::beast::flat_buffer & buffer,R & res) = 0;

    const std::string & getHost() { return m_host; }
    const std::string & getPort() { return m_port; }
    const std::string & getTarget() { return m_target; }

protected:
    boost::asio::io_context & m_ioc;
    void parseURL(const std::string & url);
    template < typename T>
    inline std::size_t write(T& comLayer, Q & req);
    template < typename T>
    inline std::size_t read(T& comLayer, boost::beast::flat_buffer & buffer,R & res);

    private:
    std::string m_port;
    std::string m_host;
    std::string m_target;


};

template < typename Q, typename R>
AsioWrapper<Q,R>::AsioWrapper(boost::asio::io_context & ioc, const std::string &url):m_ioc(ioc)
{
    parseURL(url);
}

template < typename Q, typename R>
void
AsioWrapper<Q,R>::parseURL(const std::string & url)
{
    // howto manage i18n on env vars ? : env vars don't support accented characters
    std::string httpProtocolRegexStr="^(http[s]*)";
    std::string defaultHttpRegexStr="^(http[s]*://)([^/]*)(/.*)";
    std::string httpRegexStr="^(http[s]*://)([^:]*):([^/]*)(/.*)";
    std::regex httpProtocolRegex(httpProtocolRegexStr, std::regex_constants::extended);
    std::regex defaultHttpRegex(defaultHttpRegexStr, std::regex_constants::extended);
    std::regex httpRegex(httpRegexStr, std::regex_constants::extended);
    std::smatch sm;
    if (std::regex_search(url, sm, httpProtocolRegex)) {
        if (sm.str(1) == "http") {
            m_port="80";
        }
        else {
            m_port="443";
        }
    }
    if (std::regex_search(url, sm, httpRegex)) {
        m_target = sm.str(4);
        m_port = sm.str(3);
        m_host = sm.str(2);
    } else if (std::regex_search(url, sm, defaultHttpRegex)) {
        m_target = sm.str(3);
        m_host = sm.str(2);
    }
    else {
        throw std::runtime_error("Invalid URL format : " + url);
    }
}

template < typename Q, typename R>
template < typename T>
std::size_t AsioWrapper<Q,R>::write(T& comLayer, Q & req)
{
    return http::write(comLayer, req);
}

template < typename Q, typename R>
template < typename T>
std::size_t AsioWrapper<Q,R>::read(T& comLayer, boost::beast::flat_buffer & buffer, R & res)
{
    return http::read(comLayer, buffer, res);
}

template < typename Q, typename R>
class AsioSocketWrapper: public AsioWrapper<Q,R> {
public:
    inline AsioSocketWrapper(boost::asio::io_context & ioc, const std::string &url);
    ~AsioSocketWrapper() override = default;
    inline void connect() override;
    inline void shutdown() override;
    inline std::size_t write(Q & req) override;
    inline std::size_t read(boost::beast::flat_buffer & buffer,R & res) override;


private:
    using AsioWrapper<Q,R>::m_ioc;
    using AsioWrapper<Q,R>::getHost;
    using AsioWrapper<Q,R>::getPort;
    using AsioWrapper<Q,R>::getTarget;
    tcp::socket m_comLayer;
};

template < typename Q, typename R>
AsioSocketWrapper<Q,R>::AsioSocketWrapper(boost::asio::io_context & ioc, const std::string &url):AsioWrapper<Q,R>(ioc,url),m_comLayer(tcp::socket{ioc})
{

}

template < typename Q, typename R>
void AsioSocketWrapper<Q,R>::connect()
{
    tcp::resolver resolver{m_ioc};
     // Look up the domain name
    auto const results = resolver.resolve(getHost(), getPort());

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(m_comLayer, results.begin(), results.end());
}

template < typename Q, typename R>
void AsioSocketWrapper<Q,R>::shutdown()
{
    boost::system::error_code ec;
    m_comLayer.shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if(ec && ec != boost::system::errc::not_connected)
        throw boost::system::system_error{ec};

}

template < typename Q, typename R>
std::size_t AsioSocketWrapper<Q,R>::write(Q & req)
{
    return AsioWrapper<Q,R>::template write<tcp::socket>(m_comLayer,req);
}

template < typename Q, typename R>
std::size_t AsioSocketWrapper<Q,R>::read(boost::beast::flat_buffer & buffer,R & res)
{
    return AsioWrapper<Q,R>::template read<tcp::socket>(m_comLayer,buffer,res);
}

template < typename Q, typename R>
class AsioStreamWrapper: public AsioWrapper<Q,R> {
public:
    inline AsioStreamWrapper(boost::asio::io_context & ioc,const std::string &url, ssl::context & ctx);
    ~AsioStreamWrapper() override = default;
    inline void connect() override;
    inline void shutdown() override;
    inline std::size_t write(Q & req) override;
    inline std::size_t read(boost::beast::flat_buffer & buffer,R & res) override;

private:
    using AsioWrapper<Q,R>::m_ioc;
    using AsioWrapper<Q,R>::getHost;
    using AsioWrapper<Q,R>::getPort;
    using AsioWrapper<Q,R>::getTarget;
    ssl::stream<tcp::socket> m_comLayer;
    //std::shared_ptr< ssl::stream<tcp::socket> > m_comLayer;
    ssl::context & m_ctx;
};



template < typename Q, typename R>
AsioStreamWrapper<Q,R>::AsioStreamWrapper(boost::asio::io_context & ioc, const std::string &url, ssl::context  & ctx):AsioWrapper<Q,R>(ioc,url),m_comLayer(ssl::stream<tcp::socket>{ioc, ctx}),m_ctx(ctx)

//AsioStreamWrapper<Q,R>::AsioStreamWrapper(boost::asio::io_context & ioc, const std::string &url, ssl::context  && ctx):AsioWrapper<Q,R>(ioc,url),
  //  m_ctx{ssl::context::sslv23_client}
    //m_comLayer(ssl::stream<tcp::socket>{ioc, m_ctx}),

{
    //m_comLayer = std::make_shared<ssl::stream<tcp::socket>>(m_ioc,m_ctx);
}

template < typename Q, typename R>
void AsioStreamWrapper<Q,R>::connect()
{
    load_root_certificates(m_ctx);

    // Verify the remote server's certificate
    m_ctx.set_verify_mode(ssl::verify_peer);

    // These objects perform our I/O
    tcp::resolver resolver{m_ioc};
            // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(m_comLayer.native_handle(), getHost().c_str()))
    {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        throw boost::system::system_error{ec};
    }

    // Look up the domain name
    auto const results = resolver.resolve(getHost(), getPort());

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(m_comLayer.next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    m_comLayer.handshake(ssl::stream_base::client);
}

template < typename Q, typename R>
void AsioStreamWrapper<Q,R>::shutdown()
{
    boost::system::error_code ec;
    m_comLayer.shutdown(ec);
    int reason = ERR_GET_REASON(ec.value());
    if (((ec.category() == boost::asio::error::get_ssl_category()) || (ec.category() == boost::asio::ssl::error::get_stream_category()))
         && (reason == ssl::error::stream_truncated))
    {
      // Remote peer failed to send a close_notify message.
      ec = {};
//       m_comLayer.lowest_layer().close();
    }

    if(ec == boost::asio::error::operation_aborted)
    {
        ec = {};
    }
    if(ec == boost::asio::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if(ec)
        throw boost::system::system_error{ec};
}

template < typename Q, typename R>
std::size_t AsioStreamWrapper<Q,R>::write(Q & req)
{
    return AsioWrapper<Q,R>::template write<ssl::stream<tcp::socket>>(m_comLayer,req);
}

template < typename Q, typename R>
std::size_t AsioStreamWrapper<Q,R>::read(boost::beast::flat_buffer & buffer,R & res)
{
    return AsioWrapper<Q,R>::template read<ssl::stream<tcp::socket>>(m_comLayer,buffer,res);
}


#endif
