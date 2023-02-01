#pragma once

#include <string>
#include <sstream>
#include <map>

class HttpRequest
{
public:
	enum method { CONNECT, OTHERS };
	enum http_version { HTTP_1_1, HTTP_OTHERS };

	HttpRequest() = default;

	void read_head(const std::string& head_data);
	bool read_fields(const std::string& fields_data);

	method get_method() const { return method_; }
	http_version get_version() const { return version_; }
	const std::string& get_host() const { return host_; }
	const std::string& get_port() const { return port_; }
	bool is_valid_proxy_request(const std::string& password) { return check_valid_proxy_request(password); }
	const std::string& get_field(const std::string& key) { return fields_[key]; } // todo: add boundary checking
	std::string get_proxy_message(const std::string&);
	static std::string get_200_ok_message();
	static std::string get_405_not_allowed_message();
	static std::string get_404_not_found_message();
private:
	bool check_valid_proxy_request(const std::string& password);
	bool valid_proxy_request_ = false;
	method method_;
	http_version version_;
	std::string host_;
	std::string port_;
	std::map<std::string, std::string> fields_;
};

