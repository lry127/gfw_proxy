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

    try
    {
        certificate_path_ = check_non_null_or_throw("certificate_path")->as_string().c_str();
        private_key_path_ = check_non_null_or_throw("private_key")->as_string().c_str();
        listening_address_ = check_non_null_or_throw("listening_address")->as_string().c_str();
        run_type_ = check_non_null_or_throw("run_type")->as_string().c_str();
        long long listening_port_long = check_non_null_or_throw("listening_port")->as_int64();

        // check port in range
        if (listening_port_long > 65535 || listening_port_long < 0) {
            throw incorrect_data_exception(std::string("unexpected port number: ") + std::to_string(listening_port_long));
        }
        listening_port_ = static_cast<unsigned short>(listening_port_long);
    }
    catch (std::invalid_argument& e)
    {
        // todo: improve error message by showing which key goes wrong
        throw incorrect_data_exception(std::string("incorrect json data: ") + e.what());
    }

}
