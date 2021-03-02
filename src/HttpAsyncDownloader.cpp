#include "HttpAsyncDownloader.h"

#include <iostream>
#include <regex>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>

using namespace boost::asio;
using namespace boost::asio::ip;

namespace network {
uri::uri(const std::string &url)
{
    parseURL(url);
}

void uri::parseURL(const std::string & url)
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
            m_scheme = "http";
        }
        else {
            m_port="443";
            m_scheme = "https";
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
}

HttpAsyncDownloader::HttpAsyncDownloader(boost::asio::io_service& ioservice)
    : ioservice(ioservice), resolv(ioservice) {}

HttpAsyncDownloader::future_type HttpAsyncDownloader::download_async(const std::string& url) {
    std::promise<response_type> promise;
    auto future = promise.get_future();

    download_async(url, std::move(promise));

    return future;
}

void HttpAsyncDownloader::download_async(const std::string& url,
                                         std::promise<response_type>&& promise) {
    auto state = std::make_shared<State>(std::move(promise),
                                         boost::asio::ip::tcp::socket{ioservice});
    try {
        state->uri = network::uri{url};
    } catch(...) {
        state->promise.set_exception(std::current_exception());
    }

    download_async(state);
}

void HttpAsyncDownloader::download_async(state_ptr state) {
    ip::tcp::resolver::query query(state->uri.host(),
                                   state->uri.scheme());

    resolv.async_resolve(
                query,
                [this, state](const boost::system::error_code& ec,
                ip::tcp::resolver::iterator it) { on_resolve(state, ec, it); });
}

void HttpAsyncDownloader::on_resolve(state_ptr state,
                                     const boost::system::error_code& ec,
                                     tcp::resolver::iterator it) {
    if(ec) {
        state->promise.set_exception(
                    std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    state->socket.async_connect(*it,
                                [this, state](const boost::system::error_code& ec) {
        this->on_connect(state, ec);
    });
}

void HttpAsyncDownloader::on_connect(state_ptr state, const boost::system::error_code& ec) {
    if(ec) {
        state->promise.set_exception(
                    std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    http::request<http::empty_body> req;
    req.method(http::verb::get);
    req.target(state->uri.target().empty() ? "/" : state->uri.target());
    req.version(11);
    req.set(http::field::host, state->uri.host());
    req.set(http::field::user_agent, "Beast");
    req.prepare_payload();

    if(state->uri.scheme() == "https") {
        boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12};
        state->ssl_stream =
                std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>(
                                                                                             state->socket, ctx);
        state->ssl_stream->set_verify_mode(boost::asio::ssl::verify_fail_if_no_peer_cert);
        try {
            state->ssl_stream->handshake(boost::asio::ssl::stream_base::client);
        } catch(...) {
            state->promise.set_exception(std::current_exception());
            return;
        }

        std::function<void(const boost::system::error_code&)> onSent = [this, state](const boost::system::error_code& ec) { this->on_request_sent(state, ec); };

        http::async_write(*state->ssl_stream,
                          std::move(req),
                          onSent);
    } else {
        std::function<void(const boost::system::error_code&)> onSent = [this, state](const boost::system::error_code& ec) { this->on_request_sent(state, ec); };
        http::async_write(state->socket,
                          std::move(req),
                          onSent);
    }
}

void HttpAsyncDownloader::on_request_sent(state_ptr state,
                                          const boost::system::error_code& ec) {
    if(ec) {
        state->promise.set_exception(
                    std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    state->response =
            std::make_unique<http::response<http::string_body>>();
    state->streambuf = std::make_unique<boost::asio::streambuf>();

    std::function<void(const boost::system::error_code&)> onRead = [this, state](const boost::system::error_code& ec) { this->on_read(state, ec); };
    if(state->ssl_stream) {
        http::async_read(*state->ssl_stream,
                         *state->streambuf,
                         *state->response,
                         onRead);

    } else {
        http::async_read(state->socket,
                         *state->streambuf,
                         *state->response,
                         onRead);
    }
}

enum HttpStatus {
    SUCCESS = 0,
    FAILURE = -1,
    MOVED = 1
};

static const std::map<http::status,HttpStatus> httpStatusConverter = {
     { http::status::moved_permanently, HttpStatus::MOVED },
     { http::status::found, HttpStatus::MOVED },
     { http::status::see_other, HttpStatus::MOVED },
     { http::status::temporary_redirect, HttpStatus::MOVED },
     { http::status::permanent_redirect, HttpStatus::MOVED },
     { http::status::ok, HttpStatus::SUCCESS }
};

inline HttpStatus convertStatus(const http::status & status)
{
    if (httpStatusConverter.find(status) == httpStatusConverter.end()) {
        return HttpStatus::FAILURE;
    }
    return httpStatusConverter.at(status);
}

void HttpAsyncDownloader::on_read(state_ptr state, const boost::system::error_code& ec) {
    if(ec) {
        state->promise.set_exception(
                    std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    if((convertStatus(state->response->result()) == HttpStatus::MOVED)
            && (state->response->find("location") != state->response->basic_fields::end())) {
            download_async(state->response->find("Location")->value().to_string(),
                std::move(state->promise));
        return;
    }

    state->promise.set_value(std::move(*state->response));
}
