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

	const std::string& key = fields_data.substr(0, pos);
	const std::string& value = fields_data.substr(pos + 1);

	size_t num_of_control_char = 0;
	for (auto iter = value.crbegin(); iter != value.crend(); iter++)
	{
		if (iscntrl(*iter))
			num_of_control_char++;
		else
			break;
	}

	size_t num_of_space = 0;
	for (auto iter = value.cbegin(); iter != value.cend(); iter++)
	{
		if (isspace(*iter))
			num_of_space++;
		else
			break;
	}

	fields_[key] = value.substr(num_of_space, value.size() - num_of_space - num_of_control_char);
	return true;
}

bool HttpRequest::check_valid_proxy_request(const std::string& password)
{
	// first check the format of the request
	if (!valid_proxy_request_)
		return false;

	/* check if the request contains proper password
	note that though here we use "Proxy-Authorization" field
	it doesn't necessarily mean that we'll strict follow the
	http standard ( Basic <base64 encoded string>), instead, 
	we here use a clear password directly for simplicity as
	well as security reasons (the transfer is encrypted
	under secure tls1.3 traffic)
	*/
	if (get_field("Proxy-Authorization") != password)
		return false;

	// check finished, everything's ok
	return true;
	
}

std::string HttpRequest::get_200_ok_message()
{
	return "HTTP/1.1 200 OK\r\n\r\n";
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

std::string HttpRequest::get_proxy_message(const std::string& password)
{
	if (host_.empty() && port_.empty())
	{
		std::string host_port_data = get_field("Host");
		auto pos = host_port_data.find(":");
		if (pos == std::string::npos)
		{
			host_ = host_port_data;
			port_ = "80";
		}
	}

	std::string req("CONNECT ");
	req += get_host() + ":" + get_port() + " ";
	req += "HTTP/1.1\r\n";
	req += "Proxy-Authorization: " + password;
	req += "\r\n\r\n";
	return req;
}