#include "server.h"

Server::Server(boost::asio::io_context& context, const Config& config) :
    context_(context), config_(config), ssl_context_(boost::asio::ssl::context::tlsv13_server),
    acceptor_(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config_.get_listening_address()), config_.get_listening_port()))
{
    ssl_context_.use_certificate_chain_file(config.get_certificate_path());
    ssl_context_.use_private_key_file(config_.get_private_key_path(), boost::asio::ssl::context::pem);
}

void Server::do_accept()
{
    acceptor_.async_accept([this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket) {
        std::shared_ptr<Session> session;
    if (!error) {
        /*
        if (config_.get_run_type() == "server")
            session = std::make_shared<Server_session>();
            */
        if (config_.get_run_type() == "client")
            session = std::make_shared<Client_session>(context_, ssl_context_, std::move(socket), config_);
        session->start();
    }
    do_accept();
        });
}