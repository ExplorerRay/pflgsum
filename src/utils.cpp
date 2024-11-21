#include "utils.hpp"
#include <re2/re2.h>

std::string string_trimmer(const std::string &inputString, const size_t maxLen) {
    std::string trimmedString = inputString;

    if (trimmedString.length() > maxLen) {
        trimmedString = trimmedString.substr(0, maxLen - 3) + "...";
    }

    return trimmedString;
}

static re2::RE2::Options options;
static re2::RE2 pattern1(R"(^([^\[]+)\[((?:\d{1,3}\.){3}\d{1,3})\])");
static re2::RE2 pattern2(R"(^([^\/]+)\/([0-9a-fA-F.:]+))");
static re2::RE2 pattern3(R"(^([^\[\(\/]+)[\[\(\/]([^\]\)]+)[\]\)]?:?\s*$)");
std::pair<std::string, std::string> gimme_domain(const std::string& input) {
    std::string domain, ipAddr;

    re2::StringPiece input_sp(input);
    re2::StringPiece domain_sp, ipAddr_sp;

    if (re2::RE2::FullMatch(input_sp, pattern1, &domain_sp, &ipAddr_sp)) {
        domain = std::string(domain_sp);
        ipAddr = std::string(ipAddr_sp);
    } else if (re2::RE2::FullMatch(input_sp, pattern2, &domain_sp, &ipAddr_sp)) {
        domain = std::string(domain_sp);
        ipAddr = std::string(ipAddr_sp);
    } else if (re2::RE2::FullMatch(input_sp, pattern3, &domain_sp, &ipAddr_sp)) {
        domain = std::string(domain_sp);
        ipAddr = std::string(ipAddr_sp);
    }

    // "mach.host.dom"/"mach.host.do.co" to "host.dom"/"host.do.co"
    if (domain == "unknown") {
        // Additional processing can be added here if needed
    }

    return std::make_pair(domain, ipAddr);
}
