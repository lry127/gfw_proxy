#include "httprequest.h"


void HttpRequest::read_head(const std::string& head_data)
{
	std::istringstream head_stream(head_data);
	std::string method, target, version;
	
	head_stream >> method >> target >> version;

	if (method == "CONNECT")
	{
		is_https_proxy = true;
		auto pos = target.find(":");
		host_ = target.substr(0, pos);
		port_ = target.substr(pos + 1);
	}
	else
		is_https_proxy = false;

	method_str_ = method;

	target_ = target;

	if (version == "HTTP/1.1")
		version_ = HTTP_1_1;
	else
	{
		version_ = HTTP_OTHERS;
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
	/* check if the request contains proper password
	note that though here we use "Proxy-Authorization" field
	it doesn't necessarily mean that we'll strict follow the
	http standard ( Basic <base64 encoded string>), instead, 
	we here use a clear password directly for simplicity as
	well as security reasons (the transfer is encrypted
	under secure tls1.3 traffic)
	*/
	if (!valid_proxy_request_ || get_field("Proxy-Authorization") != password)
		return false;

	// now this is a valid proxy request
	// we need to know which server it wants to connect to
	read_host_port_info();

	// check finished, everything's ok
	return true;
	
}

std::string HttpRequest::get_proxy_message(const std::string& password)
{
	read_host_port_info();
	std::string req;

	if (!is_https_proxy)
	{
		// since the request will be sent to the real server
		// other than only other proxy server, we need to
		// add the original field info to the req
		req = std::string(method_str_);
		
		auto pos = target_.find("://");
		pos = target_.find('/', pos + 3);
		req += " " + target_.substr(pos);
		req += " HTTP/1.1\r\n";

		for (auto& [field, data] : fields_)
			if (field != "Proxy-Connection")
				req += field + ": " + data + "\r\n";
			else
				req += "Connection: " + data + "\r\n";
	}
	else
	{
		req = std::string("CONNECT ");
		req += get_host() + ":" + get_port() + " ";
		req += "HTTP/1.1\r\n";
		req += "Host: " + get_host() + ":" + get_port() + "\r\n";
	}

	req += "Proxy-Authorization: " + password;
	req += "\r\n";

	return req + "\r\n";
}

void HttpRequest::read_host_port_info()
{
	if (is_https_proxy)
		return;

	auto& host_port_data = get_field("Host");
	auto pos = host_port_data.find(':');

	if (pos == std::string::npos)
	{
		host_ = host_port_data;
		port_ = "80";
	}
	else
	{
		host_ = host_port_data.substr(0, pos);
		port_ = host_port_data.substr(pos + 1);
	}
}

std::string HttpRequest::parse_plain_http_request()
{
	std::string req(method_str_);
	req += " " + target_ + " HTTP/1.1\r\n";
	for (auto& [field, data] : fields_)
		if (field != "Proxy-Authorization")
			req += field + ": " + data + "\r\n";
	return req + "\r\n";
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