// gfw_proxy.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "config.h"
#include "server.h"

int main(int argc, char** argv)
{
	try
	{
		if (argc < 2)
		{
			std::cerr << "usage: gfw-proxy <path_to_configure_file>\n";
			std::exit(-1);
		}

		auto config = Config(argv[1]);
		std::cerr << "gfw-proxy start running...\n";
		std::cerr << "run type: " << config.get_run_type() << std::endl;
		std::cerr << "listening on: " << config.get_listening_address() << ":" << config.get_listening_port() << std::endl;
		if (config.get_run_type() == "server")
		{
			std::cerr << "using costum certificate: " << config.get_certificate_path() << std::endl;
			std::cerr << "using costum private key: " << config.get_private_key_path() << std::endl;
			std::cerr << "fallback http service is running on: " << config.get_http_service_address() << ":" << config.get_http_service_port() << std::endl;
		}
		else
		{
			std::cerr << "server is running on: " << config.get_server_address() << ":" << config.get_server_port() << std::endl;
			std::cerr << "using costum ca file to verify server: " << config.get_ca_path() << std::endl;
		}

		boost::asio::io_context context;
		Server server(context, config);
		server.run();
		context.run();
	}
	catch (const boost::system::system_error& e)
	{
		std::cerr << e.what();
	}
}
