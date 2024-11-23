#pragma once

#include <string>
#include <regex>
#include <exception>

std::string string_trimmer(const std::string& inputString, const size_t maxLen);
std::pair<std::string, std::string> gimme_domain(const std::string& input);
void printException(const std::exception& e, std::string exceptionName,
                    std::string functionName, std::string message = "");
