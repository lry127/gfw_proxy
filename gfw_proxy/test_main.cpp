#include "httprequest.h"
#include <iostream>

int main()
{
	HttpRequest req;
	std::string tes1{ "Conne: refus." };
	std::string tes2{ "wow: amazing" };
	std::cerr << req.read_fields(tes1);
	std::cerr << req.read_fields(tes2);

	std::cerr << req.get_field("Conne");
}