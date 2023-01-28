#include "client_session.h"

void Client_session::start()
{
	auto self = shared_from_this();
	do_in_async_read();
}

void Client_session::do_in_async_read()
{
	auto self = shared_from_this();
	switch (read_state_)
	{
	case Client_session::BAD:
		return;
	case Client_session::REQUEST:
		boost::asio::async_read_until(ssl_socket_, in_read_buffer_, "\r\n",
			std::bind(&Client_session::on_in_async_read_finished, this, self, std::placeholders::_1, std::placeholders::_2));
		break;
	case Client_session::HEAD:
		boost::asio::async_read_until(ssl_socket_, in_read_buffer_, "\r\n",
			std::bind(&Client_session::on_in_async_read_finished, this, self, std::placeholders::_1, std::placeholders::_2));
		break;
	case Client_session::HEAD_FINISHED:
	{
		break;
	}
	case Client_session::FORWARD:
	{
		boost::asio::streambuf::mutable_buffers_type buf = in_read_buffer_.prepare(BUFFER_SIZE);
		ssl_socket_.async_read_some(buf, std::bind(&Client_session::on_in_async_read_finished, this, self, std::placeholders::_1, std::placeholders::_2));
		break;
	}
	default:
		break;
	}
}

void Client_session::on_in_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len)
{
	if (!ec)
	{
		std::istream data_stream(&in_read_buffer_);
		std::string data;
		switch (read_state_)
		{
		case Client_session::REQUEST:
		{
			std::getline(data_stream, data);
			request_.read_head(data);
			if (!request_.is_valid_proxy_request())
			{
				read_state_ = BAD;
				do_in_async_write(self_p);
				return;
			}
			read_state_ = HEAD;
			do_in_async_read();
			break;
		}
		case Client_session::HEAD:
		{			
			std::getline(data_stream, data);
			if (data != "\r")
			{
				request_.read_fields(data);
				do_in_async_read();
				break;
			}
			else 
				read_state_ = HEAD_FINISHED; // head read finished, do not break here, continue to HEAD FINISHED
		}
		case Client_session::HEAD_FINISHED:
		{
			do_in_async_write(self_p);
			break;
		}
		case Client_session::FORWARD:
		{
			in_read_buffer_.commit(read_len);
			auto read_data = consume_buffer(&in_read_buffer_);
			do_out_async_write(self_p, read_data);
			do_in_async_read();
		}
		default:
			break;
		}
	}
}

void Client_session::do_in_async_write(self_ptr self_p)
{
	switch (read_state_)
	{
	case Client_session::BAD:
	{
		auto error_data_ptr = std::make_shared<std::string>(HttpRequest::get_405_not_allowed_message());
		in_write_helper(self_p, error_data_ptr);
		break;
	}
	case Client_session::REQUEST:
		break;
	case Client_session::HEAD:
	{
		break;
	}
	case Client_session::HEAD_FINISHED:
	{
		// DEVELOPMENT NOTICE: rather, this should happen at server side in real application, we now place this at client side only for development purpose
		boost::asio::ip::tcp::resolver::query query(request_.get_host(), request_.get_port());
		resolver_.async_resolve(query, [this, self_p, &query](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results) {
			if (!ec)
			{
				boost::asio::async_connect(out_socket_, results, [this, self_p](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint& ep) {
					if (!ec)
					{
						data_ptr ok_data = std::make_shared<std::string>(HttpRequest::get_200_ok_message());
						read_state_ = FORWARD;
						in_write_helper(self_p, ok_data);
						do_in_async_read();
					}
					else
						read_state_ = BAD;
					});
			}
			else
				read_state_ = BAD;
			});
	}
	case Client_session::FORWARD:
	{
		data_ptr in_write_data = consume_buffer(&out_read_buffer_);
		in_write_helper(self_p, in_write_data);
		do_in_async_read();
	}
	default:
		break;
	}
}

void Client_session::in_write_helper(self_ptr self_p,std::shared_ptr<std::string> data_ptr)
{
	boost::asio::async_write(ssl_socket_, boost::asio::buffer(data_ptr->c_str(), data_ptr->size()), 
		std::bind(&Client_session::on_in_async_write_finished, this, self_p, data_ptr, std::placeholders::_1, std::placeholders::_2));
}

void Client_session::on_in_async_write_finished(self_ptr self_p, data_ptr data_p, const boost::system::error_code& ec, size_t write_len)
{
	do_in_async_read();
}

Client_session::data_ptr Client_session::consume_buffer(boost::asio::streambuf* buf)
{
	data_ptr buf_data_ptr = std::make_shared<std::string>();
	std::string tmp;
	std::istream buf_stream(buf);
	while (buf_stream >> tmp)
		*buf_data_ptr += tmp;
	return buf_data_ptr;
}

void Client_session::do_out_async_write(self_ptr self_p, data_ptr data)
{
	boost::asio::async_write(out_socket_, boost::asio::buffer(data->c_str(), data->size()), [this, self_p](const boost::system::error_code& ec, size_t write_len) {
		do_out_async_read(self_p);
		});
}

void Client_session::do_out_async_read(self_ptr self_p)
{
	boost::asio::streambuf::mutable_buffers_type buf = in_read_buffer_.prepare(BUFFER_SIZE);
	out_socket_.async_read_some(buf, std::bind(&Client_session::on_out_async_read_finished, this, self_p, std::placeholders::_1, std::placeholders::_2));
	
}

void Client_session::on_out_async_read_finished(self_ptr self_p, const boost::system::error_code& ec, size_t read_len)
{
	out_read_buffer_.commit(read_len);
	do_in_async_write(self_p);
	do_out_async_read(self_p);
}