#pragma once
#include <string>

void parse_log_oneline(std::string &line);
void parse_msg(std::string &msg);
void parse_qmgr_msg(std::string &msg);
void parse_smtp_msg(std::string &msg);
void parse_smtpd_msg(std::string &msg);
void parse_content(std::ifstream &input_file);
