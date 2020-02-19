#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <string>

namespace Utils
{
    std::string basename(std::string filename);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
}
#endif