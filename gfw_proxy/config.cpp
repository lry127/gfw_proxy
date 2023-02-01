#include "config.h"

Config::Config(const std::string& file_name)
{
    std::ifstream config_file(file_name);
    if (!config_file.is_open()) {
        throw no_such_file_exception("can't open the file: " + file_name);
    }

    boost::json::object config_data;
    try
    {
        boost::json::stream_parser parser;
        std::string buf;
        while (config_file >> buf) {
            parser.write(buf);
        }
        config_data = parser.release().as_object();
    }
    catch (std::exception&)
    {
        throw incorrect_data_exception("bad config file(json format error): " + file_name);
    }

    read_object_(config_data);

}

void Config::read_object_(boost::json::object& data)
{
    const auto check_non_null_or_throw = [&data](const char* key) -> boost::json::value* {
        boost::json::value* temp = data.if_contains(key);
        if (!temp) {
            // temp is nullptr and no such key exsits.
            throw incorrect_data_exception(std::string("no such key: ") + key);
        }
        return temp;
    };

    const auto check_internet_port_in_range = [](int64_t port) -> unsigned
    {
        if (port > 65535 || port < 0) {
            throw incorrect_data_exception(std::string("unexpected port number: ") + std::to_string(port));
        }
        return static_cast<unsigned>(port);
    };

    try
    {
        /*
        required fields for both server and client:
            [password, listening_address, listening_port]
        specific for server:
            [certificate_path, private_key_path, http_service_address, http_service_port]
        specific for client:
            [server_address, server_port, ca_path]
        */
        password_ = check_non_null_or_throw("password")->as_string().c_str();
        
        listening_address_ = check_non_null_or_throw("listening_address")->as_string().c_str();
        int64_t listening_port_long = check_non_null_or_throw("listening_port")->as_int64();
        listening_port_ = check_internet_port_in_range(listening_port_long);

        run_type_ = check_non_null_or_throw("run_type")->as_string().c_str();
        if (run_type_ == "server")
        {
            certificate_path_ = check_non_null_or_throw("certificate_path")->as_string().c_str();
            private_key_path_ = check_non_null_or_throw("private_key")->as_string().c_str();

            http_service_address_ = check_non_null_or_throw("http_service_address")->as_string().c_str();
            int64_t http_service_port_long = check_non_null_or_throw("http_service_port")->as_int64();
            http_service_port_ = check_internet_port_in_range(http_service_port_long);
        }
        else if (run_type_ == "client")
        {
            ca_path_ = check_non_null_or_throw("ca_path")->as_string().c_str();

            server_address_ = check_non_null_or_throw("server_address")->as_string().c_str();
            int64_t server_port_long = check_non_null_or_throw("server_port")->as_int64();
            server_port_ = check_internet_port_in_range(server_port_long);
        }
        else
        {
            std::cerr << "unknown run type: " << run_type_ << std::endl;
            std::exit(-1);
        }
    }
    catch (std::invalid_argument& e)
    {
        // todo: improve error message by showing which key goes wrong
        throw incorrect_data_exception(std::string("incorrect json data: ") + e.what());
    }

}
