#pragma once

#include <string>
#include <regex>

std::string string_trimmer(const std::string& inputString, const size_t maxLen);
std::pair<std::string, std::string> gimme_domain(const std::string& input);
