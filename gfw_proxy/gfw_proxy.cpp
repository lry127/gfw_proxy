// gfw_proxy.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "config.h"
#include "server.h"

int main(int argc, char** argv)
{
	try
	{
		Config config{};
		if (argc < 2)
		{
			std::cerr << "usage: gfw-proxy <path_to_configure_file>\n";
			config = Config("C:\\Users\\lry127\\source\\repos\\gfw_proxy\\x64\\Debug\\client.json");
		}
		else 
			config = Config(argv[1]);
		std::cerr << config.get_run_type() << std::endl;
		std::cerr << config.get_certificate_path() << std::endl;
		std::cerr << config.get_listening_port() << std::endl;
		std::cerr << config.get_private_key_path() << std::endl;
		std::cerr << config.get_listening_address() << std::endl;

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

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
