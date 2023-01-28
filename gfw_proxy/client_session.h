#pragma once

#include <functional>
#include <iostream>

#include "session.h"
#include "httprequest.h"

class Client_session final : public Session
{
public:
	Client_session(boost::asio::io_context& context, boost::asio::ssl::context& ssl_context, boost::asio::ip::tcp::socket socket, const Config& config) :
		Session(context, ssl_context, std::move(socket), config), ssl_socket_(std::move(socket_)),out_socket_(context)
	, resolver_(context), in_read_buffer_(), out_read_buffer_(){}
	void start() override;
	~Client_session() override {}

private:
	using data_ptr = std::shared_ptr<std::string>;
	enum { BUFFER_SIZE = 1024};
	enum read_state { BAD, REQUEST, HEAD, HEAD_FINISHED, FORWARD} read_state_ = REQUEST;
	boost::asio::ip::tcp::socket ssl_socket_;
	boost::asio::ip::tcp::socket out_socket_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::asio::streambuf in_read_buffer_;
	boost::asio::streambuf out_read_buffer_;
	HttpRequest request_;
	void do_in_async_read();
	void on_in_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len);
	void do_in_async_write(self_ptr self_p);
	void on_in_async_write_finished(self_ptr self_p, data_ptr data_p, const boost::system::error_code& ec, size_t write_len);
	void in_write_helper(self_ptr self_p, data_ptr data);
	void do_out_async_write(self_ptr self_p, data_ptr data);
	void do_out_async_read(self_ptr self_p);
	void on_out_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len);
	data_ptr consume_buffer(boost::asio::streambuf* buf);
};