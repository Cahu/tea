#include "utils/splitstr.hh"

#include <sstream>


int splitstr(const char *str, std::vector<std::string> &dst, char delim)
{
	int nentries = 0;
	std::string item;
	std::stringstream ss(str);

	while (std::getline(ss, item, delim)) {
		dst.push_back(item);
		nentries++;
	}

	return nentries;
}


int splitstr(const std::string &str, std::vector<std::string> &dst, char delim)
{
	return splitstr(str.c_str(), dst, delim);
}
