#include "client_session.h"

void Client_session::do_in_async_read()
{
	auto self = shared_from_this();
	ssl_socket_.async_read_some(boost::asio::buffer(in_data_buf_, BUFFER_SIZE), [this, self](const boost::system::error_code& ec, size_t length) {
		if (ec == boost::asio::error::operation_aborted)
			return;
	if (ec)
	{
		destroy();
		return;
		}
	raw_data_ptr raw_read_data = std::make_shared<boost::asio::const_buffer>(in_data_buf_, length);
	data_ptr read_data = std::make_shared<std::string>((const char*)in_data_buf_, length);
	on_in_received(read_data);
		});
}


void Client_session::on_in_received(data_ptr data)
{
	auto self = shared_from_this();
	std::istringstream data_stream(*data);
	switch (read_state_)
	{
	case Client_session::BAD:
		break;
	case Client_session::REQUEST:
	{
		std::string head;
		std::getline(data_stream, head);
		request_.read_head(head);
		// continue to process field, do not break here
	}
	case Client_session::HEAD:
	{
		std::string field;
		while (std::getline(data_stream, field))
			if (field != "\r")
				request_.read_fields(field);
		if (!request_.is_valid_proxy_request())
		{
			destroy();
			return;
		}
		// continue to establish connection to the server, do not break here.
	}
	case Client_session::HEAD_FINISHED:
	{
		boost::asio::ip::tcp::resolver::query query(request_.get_host(), request_.get_port());
		resolver_.async_resolve(query, [this, self, &query](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results) {
			if (!ec)
			{
				boost::asio::async_connect(out_socket_, results, [this, self](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& ep) {
					if (!ec)
					{
						std::cerr << "accepted connection to " << request_.get_host() << ":" << request_.get_port() << std::endl;
						read_state_ = HEAD_FINISHED;
						data_ptr ok_data = std::make_shared<std::string>(HttpRequest::get_200_ok_message());
						do_in_async_write(ok_data);
					}
					else
					{
						destroy();
						return;
					}
					});
			}
			else
			{
				destroy();
				return;
			}
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

void Client_session::do_in_async_write(data_ptr data)
{
	auto self = shared_from_this();
	boost::asio::async_write(ssl_socket_, boost::asio::buffer(*data), [this, self, data](const boost::system::error_code& ec, size_t length) {
		if (ec)
		{
			destroy();
			return;
		}
	on_in_sent();
		});
}

void Client_session::do_out_async_write(data_ptr data)
{
	auto self = shared_from_this();
	boost::asio::async_write(out_socket_, boost::asio::buffer(*data), [this, self, data](const boost::system::error_code& ec, size_t length) {
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
	out_socket_.async_read_some(boost::asio::buffer(out_data_buf_, BUFFER_SIZE), [this, self](const boost::system::error_code& ec, size_t length)
		{
			if (ec)
			{
				destroy();
				return;
			}
	data_ptr data = std::make_shared<std::string>((const char*)in_data_buf_, length);
	on_out_received(data);
		});
}

void Client_session::on_out_received(data_ptr data)
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
	if (ssl_socket_.is_open()) {
		ssl_socket_.cancel(ec);
		ssl_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		ssl_socket_.close(ec);
	}

	if (out_socket_.is_open()) {
		out_socket_.cancel(ec);
		out_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		out_socket_.close(ec);
	}

	std::cerr << "connection to " + request_.get_host() + ":" + request_.get_port() << " closed.\n";
}