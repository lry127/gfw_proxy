#include "httprequest.h"
#include <iostream>

int main()
{
	HttpRequest req;
	std::string tes1{ "Conne: csdarefus.\r\r\r\r\r" };
	std::string test2{ "Proxy-Authorization:   i_love_cppE\r\r\n\r\n" };

	req.read_fields(tes1);
	req.read_fields(test2);
	std::cerr << req.get_field("Proxy-Authorization");
}