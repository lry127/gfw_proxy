#include "server.h"

Server::Server(boost::asio::io_context& io_context, const Config& config) :
    context_(io_context), config_(config), ssl_server_context_(boost::asio::ssl::context::tlsv13_server), ssl_client_context_(boost::asio::ssl::context::tlsv13_client),
    acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config_.get_listening_address()), config_.get_listening_port()))
{
    ssl_server_context_.use_certificate_chain_file(config.get_certificate_path());
    ssl_server_context_.use_private_key_file(config_.get_private_key_path(), boost::asio::ssl::context::pem);
    auto* context = ssl_server_context_.native_handle();
    int res = SSL_CTX_set_ciphersuites(context, "TLS_AES_256_GCM_SHA384:TLS_AES_128_GCM_SHA256:TLS_CHACHA20_POLY1305_SHA256");
    if (res != 1)
    {
        std::cerr << "failed to set desired cipher suites\n";
        std::exit(-1);
    }

    ssl_client_context_.load_verify_file(config_.get_ca_path());
    auto* client_context = ssl_client_context_.native_handle();
    int cl_res = SSL_CTX_set_ciphersuites(client_context, "TLS_AES_256_GCM_SHA384:TLS_AES_128_GCM_SHA256:TLS_CHACHA20_POLY1305_SHA256");
    if (cl_res != 1)
    {
        std::cerr << "failed to set desired cipher suites\n";
        std::exit(-1);
    }
}

void Server::do_accept()
{
    acceptor_.async_accept([this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket) {
        std::shared_ptr<Session> session;
    if (!error) {
        if (config_.get_run_type() == "server")
            session = std::make_shared<Server_session>(context_, ssl_server_context_, std::move(socket), config_);
        else if(config_.get_run_type() == "client")
            session = std::make_shared<Client_session>(context_, ssl_client_context_, std::move(socket), config_);
        session->start();
    }
    do_accept();
        });
}