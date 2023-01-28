#include "httprequest.h"


void HttpRequest::read_head(const std::string& head_data)
{
	std::istringstream head_stream(head_data);
	std::string method, target, version;
	
	head_stream >> method >> target >> version;

	if (method == "CONNECT")
		method_ = CONNECT;
	else 
	{
		method_ = OTHERS;
		valid_proxy_request_ = false;
		return;
	}
		
	// now read the target
	auto pos = target.find(':');
	if (pos == std::string::npos)
	{
		valid_proxy_request_ = false;
		return;
	}
	host_ = target.substr(0, pos);
	port_ = target.substr(pos + 1);

	if (version == "HTTP/1.1")
	{
		version_ = HTTP_1_1;
	}
	else
	{
		version_ = HTTP_OTHERS;
		valid_proxy_request_ = false;
		return;
	}

	valid_proxy_request_ = true;
}

bool HttpRequest::read_fields(const std::string& fields_data)
{
	auto pos = fields_data.find(':');
	if (pos == std::string::npos)
		return false;

	size_t num_of_control_char = 0;
	for (auto iter = fields_data.crbegin(); iter != fields_data.crend(); iter++)
	{
		if (iscntrl(*iter))
			num_of_control_char++;
		else
			break;
	}

	fields_[fields_data.substr(0, pos)] = fields_data.substr(pos + 1, fields_data.size() - pos - 1 - num_of_control_char);
	return true;
}

std::string HttpRequest::get_200_ok_message()
{
	return "HTTP/1.1 200 OK\r\nConnection: Keep-alive\r\n\r\n";
}

std::string HttpRequest::get_404_not_found_message()
{
	return "HTTP/1.1 404 Not Found\r\nConnection: Close\r\n\r\n";
}

std::string HttpRequest::get_405_not_allowed_message()
{
	static const std::string content{ "<html><h1>405 Method Not Allowed.</h1></html>" };
	return std::string("HTTP/1.1 405 Method Not Allowed\r\nConnection: Close\r\nContent-Length: ") + std::to_string(content.size())
		+ "\r\n\r\n" + content;
}