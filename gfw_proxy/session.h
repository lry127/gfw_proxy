#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "config.h"

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(boost::asio::io_context& context, boost::asio::ssl::context& ssl_context, const Config& config) :
		context_(context), ssl_context_(ssl_context), config_(config), ssl_shutdown_timer(context) {}
	virtual void start() = 0;
	virtual ~Session() {}

protected:
	typedef std::shared_ptr<Session> self_ptr;
	static std::string bytes_to_readable(size_t size_in_byte);
	boost::asio::io_context& context_;
	boost::asio::ssl::context& ssl_context_;
	boost::asio::steady_timer ssl_shutdown_timer;
	const Config& config_;
	size_t recevied_size_ = 0;
	size_t upload_size_ = 0;
};
