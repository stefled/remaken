#ifndef HTTPASYNCDOWNLOADER_H
#define HTTPASYNCDOWNLOADER_H

#include <future>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http/string_body.hpp>

namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;

namespace network {
class uri {
public:
    uri() = default;
    explicit uri(const std::string &url);
    virtual ~uri() = default;
    const std::string & host() { return m_host; }
    const std::string & port() { return m_port; }
    const std::string & target() { return m_target; }
    const std::string & scheme() { return m_target; }


    private:
    void parseURL(const std::string & url);

    std::string m_port;
    std::string m_host;
    std::string m_target;
    std::string m_scheme;
};
}

class HttpAsyncDownloader {
public:
    using response_type = http::response<http::string_body>;
    using future_type = std::future<response_type>;

    explicit HttpAsyncDownloader(boost::asio::io_service &ioservice);
    future_type download_async(const std::string &url);

private:
    struct State {
        std::promise<response_type> promise;
        network::uri uri;
        boost::asio::ip::tcp::socket socket;
        std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket &>>
        ssl_stream;
        std::unique_ptr<http::response<http::string_body>> response;
        std::unique_ptr<boost::asio::streambuf> streambuf;

        State(std::promise<response_type> &&promise,
              boost::asio::ip::tcp::socket &&socket)
            : promise{std::move(promise)}, socket(std::move(socket)) {}
    };
    using state_ptr = std::shared_ptr<State>;

    void download_async(const std::string &url, std::promise<response_type> &&promise);
    void download_async(state_ptr state);

    void on_resolve(state_ptr state,
                    const boost::system::error_code &ec,
                    boost::asio::ip::tcp::resolver::iterator iterator);
    void on_connect(state_ptr state, const boost::system::error_code &ec);
    void on_request_sent(state_ptr state, const boost::system::error_code &ec);
    void on_read(state_ptr state, const boost::system::error_code &ec);

    boost::asio::io_service &ioservice;
    boost::asio::ip::tcp::resolver resolv;
};

#endif // HTTPASYNCDOWNLOADER_H
