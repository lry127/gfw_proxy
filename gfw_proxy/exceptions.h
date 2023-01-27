#pragma once
#include <stdexcept>

class no_such_file_exception : public std::runtime_error
{
public:
    explicit no_such_file_exception(const std::string& message) : runtime_error(message) {}
    explicit no_such_file_exception(const char* message) : runtime_error(message) {}
};

class incorrect_data_exception : public std::runtime_error
{
public:
    explicit incorrect_data_exception(const std::string& message) : runtime_error(message) {}
    explicit incorrect_data_exception(const char* message) : runtime_error(message) {}
};
