#include "server_session.h"

void Server_session::do_in_async_handshake()
{
	auto self = shared_from_this();
	ssl_socket_.async_handshake(boost::asio::ssl::stream_base::server, [this, self](const boost::system::error_code& ec) {
		if (ec)
		{
			destroy();
			return;
		}
	do_in_async_read();
		});
}

void Server_session::do_in_async_read()
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
	on_in_received(raw_read_data);
		});
}


void Server_session::on_in_received(raw_data_ptr data)
{
	auto self = shared_from_this();
	switch (read_state_)
	{
	case Server_session::BAD:
		break;
	case Server_session::REQUEST:
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
		std::shared_ptr<boost::asio::ip::tcp::resolver::query> query;
		if (!request_.is_valid_proxy_request(config_.get_password()))
		{
			// the proxy request in invalid (provided incorrect password or no password)
			// we forword this request to a pre-configured real http server to provide service
			query.reset(new boost::asio::ip::tcp::resolver::query(config_.get_http_service_address(), std::to_string(config_.get_http_service_port())));
		}
		else
		{
			// the proxy request is valid, perform a dns look up for the desired endpoint
			query.reset(new boost::asio::ip::tcp::resolver::query(request_.get_host(), request_.get_port()));
		}
		resolver_.async_resolve(*query, [this, self, query](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results) {
			if (!ec)
			{
				boost::asio::async_connect(out_socket_, results, [this, self](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& ep) {
					if (!ec)
					{

						if (request_.is_valid_proxy_request(config_.get_password()))
						{
							std::cerr << "accepted connection to " << request_.get_host() << ":" << request_.get_port() << std::endl;
							if (request_.get_port() == "80")
							{
								data_ptr original_data_ptr = std::make_shared<std::string>(request_.parse_plain_http_request());
								boost::asio::async_write(out_socket_, boost::asio::buffer(*original_data_ptr), [this, self, original_data_ptr](const boost::system::error_code& ec, size_t len) {
									read_state_ = FORWARD;
									on_out_sent();
									});
							}
							else
							{
								data_ptr ok_data_str = std::make_shared <std::string>(HttpRequest::get_200_ok_message());
								boost::asio::async_write(ssl_socket_, boost::asio::buffer(*ok_data_str), [this, self, ok_data_str](const boost::system::error_code& ec, size_t len) {
									read_state_ = HEAD_FINISHED;
								on_in_sent();
									});
							}
						}
						else
						{
							std::cerr << "unrecongnized http request, redirecting to http sercive... (from: " << ssl_socket_.next_layer().remote_endpoint().address() << ")\n";
							// now forward the original http message to the real http server
							data_ptr original_data_ptr = std::make_shared<std::string>(conn_esta_msg_);
							boost::asio::async_write(out_socket_, boost::asio::buffer(*original_data_ptr), [this, self, original_data_ptr](const boost::system::error_code& ec, size_t len) {
								read_state_ = FORWARD;
								on_out_sent();
								});
						}
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
	case Server_session::FORWARD:
	{
		upload_size_ += data->size();
		do_out_async_write(data);
		break;
	}
	default:
		break;
	}
}

void Server_session::do_in_async_write(raw_data_ptr data)
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

void Server_session::do_out_async_write(raw_data_ptr data)
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

void Server_session::on_in_sent()
{
	switch (read_state_)
	{
	case Server_session::HEAD_FINISHED:
	{
		read_state_ = FORWARD;
		do_in_async_read();
		break;
	}
	case Server_session::FORWARD:
	{
		do_out_async_read();
		break;
	}
	default:
		break;
	}
}

void Server_session::on_out_sent()
{
	do_in_async_read();
	if (first_out_write_)
	{
		first_out_write_ = false;
		do_out_async_read();
	}
}

void Server_session::do_out_async_read()
{
	auto self = shared_from_this();
	out_socket_.async_read_some(boost::asio::buffer(out_data_buf_, BUFFER_SIZE), [this, self](const boost::system::error_code& ec, size_t length)
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

void Server_session::on_out_received(raw_data_ptr data)
{
	recevied_size_ += data->size();
	do_in_async_write(data);
}

void Server_session::destroy()
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

	std::cerr << "connection to " << request_.get_host() + ":" + request_.get_port() << " closed. Sent: " << bytes_to_readable(upload_size_)
		<< " Recevied: " << bytes_to_readable(recevied_size_) << std::endl;
}