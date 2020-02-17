#include "utils.hpp"

#include <string>
#include <regex>

std::string basename(std::string filename)
{
    size_t basename_start = filename.find_last_of("/\\", std::string::npos);
    if(basename_start != std::string::npos)
        return filename.substr(basename_start + 1);
    else
        return filename;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {

    std::regex e(from);
    while(std::regex_search(str, e))
    {
        str = std::regex_replace(str, e, to);
    }
}