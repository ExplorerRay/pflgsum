#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <vector>

// struct MailInfo {
//     long long received;
//     long long delivered;
//     long long deferred;
//     long long bounced;
//     long long rejected;
//     long long discarded;
// };

// struct TrafficSummary {
//     int messageCount;
//     size_t totalSize;
// };

void parse_log_oneline(std::string &line);
void parse_msg(std::string &msg);
void parse_smtp_msg(std::string &msg);
void parse_smtpd_msg(std::string &msg);
void parse_content(std::ifstream &input_file);
void print_summary();
