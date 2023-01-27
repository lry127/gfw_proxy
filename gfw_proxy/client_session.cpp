#include "client_session.h"

void Client_session::start()
{
	auto self = shared_from_this();
	ssl_socket_.async_handshake(boost::asio::ssl::stream_base::server, [this, self](const boost::system::error_code& ec) {
		if (!ec)
			do_in_async_read();
		});
}

void Client_session::do_in_async_read()
{
	auto self = shared_from_this();
	switch (read_state_)
	{
	case Client_session::BAD:
		return;
	case Client_session::REQUEST:
	case Client_session::HEAD:
		boost::asio::async_read_until(ssl_socket_, in_read_buffer_, "\r\n", std::bind(&Client_session::on_in_async_read_finished, this, self, std::placeholders::_1, std::placeholders::_2));
	case Client_session::FORWARD:
		break;
	default:
		break;
	}
}

void Client_session::on_in_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len)
{
	if (!ec)
	{
		switch (read_state_)
		{
		case Client_session::REQUEST:
		case Client_session::HEAD:
		{
			std::istream data_stream(&in_read_buffer_);
			std::string data;
			std::getline(data_stream, data);
			std::cerr << data << '\n';
			do_in_async_read();
		}
		case Client_session::FORWARD:
			break;
		default:
			break;
		}
		// do_in_async_write();
	}
}
