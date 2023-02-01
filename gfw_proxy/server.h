#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "config.h"
#include "server_session.h"
#include "client_session.h"

class Server
{
public:
    Server(boost::asio::io_context& context, const Config& config);
    Server(const Server&) = delete;
    void run() { do_accept(); }
private:
    void do_accept();
    const Config& config_;
    boost::asio::io_context& context_;
    boost::asio::ssl::context ssl_server_context_;
    boost::asio::ssl::context ssl_client_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
};