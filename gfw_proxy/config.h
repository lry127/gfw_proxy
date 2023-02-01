#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <set>
#include <boost/json.hpp>

#include "exceptions.h"

class Config
{
public:
    Config() = default;
    Config(const std::string& file_name);
    const std::string& get_certificate_path() const { return certificate_path_; }
    const std::string& get_private_key_path() const { return private_key_path_; }
    const std::string& get_server_address() const { return server_address_; }
    unsigned short get_listening_port() const { return listening_port_; }
    unsigned short get_server_port() const { return server_port_; }
    const std::string& get_listening_address() const { return listening_address_; }
    const std::string& get_run_type() const { return run_type_; }
    const std::string& get_password() const { return password_; }
    const std::string& get_ca_path() const { return ca_path_; }
    const std::string& get_http_service_address() const { return http_service_address_; }
    unsigned short get_http_service_port() const { return http_service_port_; }private:
    void read_object_(boost::json::object&);
    std::string certificate_path_;
    std::string server_address_;
    std::string private_key_path_;
    unsigned short listening_port_;
    unsigned short server_port_;
    unsigned short http_service_port_;
    std::string listening_address_;
    std::string run_type_;
    std::string password_;
    std::string http_service_address_;
    std::string ca_path_;
};