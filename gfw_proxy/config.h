#pragma once

#include <string>
#include <fstream>
#include <set>
#include <boost/json.hpp>

#include "exceptions.h"

class Config
{
public:
    Config(const std::string& file_name);
    const std::string& get_certificate_path() const { return certificate_path_; }
    const std::string& get_private_key_path() const { return private_key_path_; }
    unsigned short get_listening_port() const { return listening_port_; }
    const std::string& get_listening_address() const { return listening_address_; }
    const std::string& get_run_type() const { return run_type_; }
private:
    void read_object_(boost::json::object&);
    std::string certificate_path_;
    std::string private_key_path_;
    unsigned short listening_port_;
    std::string listening_address_;
    std::string run_type_;
};