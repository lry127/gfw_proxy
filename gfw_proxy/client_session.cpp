#include "client_session.h"

void Client_session::do_out_async_connect()
{
	auto self = shared_from_this();
	auto query = std::make_shared<boost::asio::ip::tcp::resolver::query>(config_.get_server_address(), std::to_string(config_.get_server_port()));

	resolver_.async_resolve(*query, [this, self, query](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results) {
		if (!ec)
		{
			boost::asio::async_connect(ssl_socket_.lowest_layer(), results, [this, self](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& ep) {
				do_out_async_handshake();
				});
		}
		else
		{
			std::cerr << "can't connect to server, failed to handle http proxy request\n";
			destroy();
		}
		});
}

void Client_session::do_out_async_handshake()
{
	auto self = shared_from_this();
	ssl_socket_.async_handshake(boost::asio::ssl::stream_base::client, [this, self](const boost::system::error_code& ec) {
		if (ec)
		{
			std::cerr << ec.message() << std::endl;
			destroy();
			return;
		}
	do_in_async_read();
		});
}

void Client_session::do_in_async_read()
{
	auto self = shared_from_this();
	out_socket_.async_read_some(boost::asio::buffer(in_data_buf_, BUFFER_SIZE), [this, self](const boost::system::error_code& ec, size_t length) {
		if (ec == boost::asio::error::operation_aborted)
		return;
	if (ec)
	{
		destroy();
		return;
	}
	raw_data_ptr raw_read_data = std::make_shared<boost::asio::const_buffer>(in_data_buf_, length);
	on_in_received(raw_read_data);
		});
}


void Client_session::on_in_received(raw_data_ptr data)
{
	auto self = shared_from_this();
	switch (read_state_)
	{
	case Client_session::BAD:
		break;
	case Client_session::REQUEST:
	{
		std::string recv_data{ (const char*)data->data() };
		conn_esta_msg_ += recv_data;
		if (conn_esta_msg_.find("\r\n\r\n") == std::string::npos)
		{
			do_in_async_read();
			return;
		}
		std::istringstream data_stream(conn_esta_msg_);
		std::string head;
		std::getline(data_stream, head);
		request_.read_head(head);
		std::string field;
		while (std::getline(data_stream, field))
			if (field != "\r")
				request_.read_fields(field);
		data_ptr proxy_auth_ptr = std::make_shared<std::string>(request_.get_proxy_message(config_.get_password()));
		std::cerr << "accepted connection to " + request_.get_host() + ":" + request_.get_port() + "\n";
		boost::asio::async_write(ssl_socket_, boost::asio::buffer(*proxy_auth_ptr), [this, self, proxy_auth_ptr](const boost::system::error_code& ec, size_t length) {
			if (ec)
			{
				destroy();
				return;
			}
		read_state_ = FORWARD;
		on_out_sent();
			});

		break;
	}
	case Client_session::FORWARD:
	{
		do_out_async_write(data);
		break;
	}
	default:
		break;
	}
}

void Client_session::do_in_async_write(raw_data_ptr data)
{
	auto self = shared_from_this();
	boost::asio::async_write(out_socket_, boost::asio::buffer(*data), [this, self, data](const boost::system::error_code& ec, size_t length) {
		if (ec)
		{
			destroy();
			return;
		}
	on_in_sent();
		});
}

void Client_session::do_out_async_write(raw_data_ptr data)
{
	auto self = shared_from_this();
	boost::asio::async_write(ssl_socket_, boost::asio::buffer(*data), [this, self, data](const boost::system::error_code& ec, size_t length) {
		if (ec)
		{
			destroy();
			return;
		}
	on_out_sent();
		});
}

void Client_session::on_in_sent()
{
	switch (read_state_)
	{
	case Client_session::HEAD_FINISHED:
	{
		read_state_ = FORWARD;
		do_in_async_read();
		break;
	}
	case Client_session::FORWARD:
	{
		do_out_async_read();
		break;
	}
	default:
		break;
	}
}

void Client_session::on_out_sent()
{
	do_in_async_read();
	if (first_out_write_)
	{
		first_out_write_ = false;
		do_out_async_read();
	}
}

void Client_session::do_out_async_read()
{
	auto self = shared_from_this();
	ssl_socket_.async_read_some(boost::asio::buffer(out_data_buf_, BUFFER_SIZE), [this, self](const boost::system::error_code& ec, size_t length)
		{
			if (ec)
			{
				destroy();
				return;
			}
	raw_data_ptr data = std::make_shared<boost::asio::const_buffer>(out_data_buf_, length);
	on_out_received(data);
		});
}

void Client_session::on_out_received(raw_data_ptr data)
{
	do_in_async_write(data);
}

void Client_session::destroy()
{
	if (read_state_ == BAD)
		return;
	read_state_ = BAD;

	boost::system::error_code ec;

	resolver_.cancel();

	if (ssl_socket_.next_layer().is_open())
	{
		auto self = shared_from_this();
		auto ssl_shutdown_cb = [this, self](const boost::system::error_code error) {
			if (error == boost::asio::error::operation_aborted)
				return;
			boost::system::error_code ec;
			ssl_shutdown_timer.cancel();
			ssl_socket_.next_layer().cancel(ec);
			ssl_socket_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			ssl_socket_.next_layer().close(ec);
		};
		ssl_socket_.next_layer().cancel(ec);
		ssl_socket_.async_shutdown(ssl_shutdown_cb);
		ssl_shutdown_timer.expires_after(std::chrono::seconds(120));
		ssl_shutdown_timer.async_wait(ssl_shutdown_cb);
	}

	if (out_socket_.is_open())
	{
		out_socket_.cancel(ec);
		out_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		out_socket_.close(ec);
	}

	std::cerr << "connection to " << ssl_socket_.next_layer().remote_endpoint(ec).address() << " closed.\n";
}