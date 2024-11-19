#include "utils.hpp"

std::string string_trimmer(const std::string &inputString, const size_t maxLen) {
    std::string trimmedString = inputString;

    if (trimmedString.length() > maxLen) {
        trimmedString = trimmedString.substr(0, maxLen - 3) + "...";
    }

    return trimmedString;
}

std::pair<std::string, std::string> gimme_domain(const std::string& input) {
    std::string domain, ipAddr;
    std::smatch match;

    // Newer versions of Postfix have them "dom.ain[i.p.add.ress]"
    std::regex pattern1(R"(^([^\[]+)\[((?:\d{1,3}\.){3}\d{1,3})\])");
    // Older versions of Postfix have them "dom.ain/i.p.add.ress"
    std::regex pattern2(R"(^([^\/]+)\/([0-9a-f.:]+))", std::regex::icase);
    // More exhaustive method
    std::regex pattern3(R"(^([^\[\(\/]+)[\[\(\/]([^\]\)]+)[\]\)]?:?\s*$)");

    if (std::regex_match(input, match, pattern1)) {
        domain = match[1];
        ipAddr = match[2];
    } else if (std::regex_match(input, match, pattern2)) {
        domain = match[1];
        ipAddr = match[2];
    } else if (std::regex_match(input, match, pattern3)) {
        domain = match[1];
        ipAddr = match[2];
    }

    // "mach.host.dom"/"mach.host.do.co" to "host.dom"/"host.do.co"
    if (domain == "unknown") {
        // Additional processing can be added here if needed
    }

    return std::make_pair(domain, ipAddr);
}
