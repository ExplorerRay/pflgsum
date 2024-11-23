#pragma once
#include <string>
#include "threadContext.hpp"

void parse_log_oneline(std::string &line, Record& record);
void parse_msg(std::string &msg, Record& record);
void parse_smtp_msg(std::string &msg, Record& record);
void parse_smtpd_msg(std::string &msg, Record& record);
void parse_content(int threadID, int start, int end, ThreadContext* context);
