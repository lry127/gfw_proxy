#include "session.h"

std::string Session::bytes_to_readable(size_t size_in_bytes)
{
	const auto get_byte_string = [](double data) -> std::string {
		std::string bs = std::to_string(data);
		auto pos = bs.find('.');
		return bs.substr(0, pos + 2);
	};

	double size_in_double = static_cast<double>(size_in_bytes);

	if (size_in_double < 1024)
		return get_byte_string(size_in_double) + " bytes";

	auto kilo_bytes = size_in_double / 1024;
	if (kilo_bytes < 1024)
		return get_byte_string(kilo_bytes) + " KB";

	auto mega_bytes = kilo_bytes / 1024;
	if (mega_bytes < 1024)
		return get_byte_string(mega_bytes) + " MB";

	auto giga_bytes = mega_bytes / 1024;
	return get_byte_string(giga_bytes) + " GB";
	
}