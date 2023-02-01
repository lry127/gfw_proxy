#pragma once

#include <functional>
#include <iostream>
#include <cstring>
#include <sstream>

#include "session.h"
#include "httprequest.h"

class Client_session final : public Session
{
public:
	Client_session(boost::asio::io_context& context, boost::asio::ssl::context& ssl_context, boost::asio::ip::tcp::socket socket, const Config& config) :
		Session(context, ssl_context, config), ssl_socket_(context, ssl_context), out_socket_(std::move(socket))
		, resolver_(context) { ssl_socket_.set_verify_mode(boost::asio::ssl::verify_peer); }
	void start() 
	{ 
		try
		{
			do_out_async_connect();
		}
		catch (const std::exception&)
		{
			destroy();
		}
	}
	~Client_session() override {}

private:
	using data_ptr = std::shared_ptr<std::string>;
	using raw_data_ptr = std::shared_ptr<boost::asio::const_buffer>;
	enum { BUFFER_SIZE = 1024 };
	enum read_state { BAD, REQUEST, HEAD, HEAD_FINISHED, FORWARD } read_state_ = REQUEST;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
	boost::asio::ip::tcp::socket out_socket_;
	boost::asio::ip::tcp::resolver resolver_;
	unsigned char in_data_buf_[BUFFER_SIZE]{};
	unsigned char out_data_buf_[BUFFER_SIZE]{};
	std::string conn_esta_msg_;
	HttpRequest request_;
	void do_out_async_handshake();
	void do_out_async_connect();
	void do_in_async_read();
	void do_in_async_write(raw_data_ptr data);
	void do_out_async_write(raw_data_ptr data);
	void do_out_async_read();
	void destroy();
	void on_in_received(raw_data_ptr data);
	void on_in_sent();
	void on_out_sent();
	void on_out_received(raw_data_ptr data);
	bool first_out_write_ = true;
	/*
	void on_in_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len);
	void do_in_async_write(self_ptr self_p);
	void on_in_async_write_finished(self_ptr self_p, data_ptr data_p, const boost::system::error_code& ec, size_t write_len);
	void in_write_helper(self_ptr self_p, data_ptr data);
	void out_write_helper(self_ptr self_p, data_ptr data);
	void do_out_async_write(self_ptr self_p, size_t buf_len);
	void on_out_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len);
	data_ptr consume_buffer(boost::asio::streambuf* buf);
	*/
};