#pragma once

#include <functional>
#include <iostream>
#include "session.h"

class Client_session final : public Session
{
public:
	Client_session(boost::asio::io_context& context, boost::asio::ssl::context& ssl_context, boost::asio::ip::tcp::socket socket, const Config& config) :
		Session(context, ssl_context, std::move(socket), config), ssl_socket_(std::move(socket_), ssl_context_), in_read_buffer_(){}
	void start() override;
	~Client_session() override {}

private:
	enum read_state { BAD, REQUEST, HEAD, FORWARD} read_state_ = REQUEST;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
	boost::asio::streambuf in_read_buffer_;
	void do_in_async_read();
	void on_in_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len);
};