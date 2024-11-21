#include "parser.hpp"
#include "record.hpp"
#include "utils.hpp"

#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <re2/re2.h>


Record record;

re2::RE2 msgPtrn_withColon(R"((\w+):\s+(.*))");
re2::RE2 msgPtrn_noColon(R"(.+)");
re2::RE2 smtpPtrn(R"(\w+: to=<([^>]*)>, (?:orig_to=<[^>]*>, )?relay=([^,]+), (?:conn_use=[^,]+, )?delay=([^,]+), (?:delays=[^,]+, )?(?:dsn=[^,]+, )?status=(\S+).*)");
re2::RE2 clientPtrn(R"(\w+: client=(.+?)(,|$).+sasl_username=(\w+))");
re2::RE2 exceptPtrn(R"(\w+: (reject(?:_warning)?|hold|discard))");
re2::RE2 warnPtrn(R"(warning: (.+))");

void parse_log_oneline(std::string &line) {
    // timestamp hostname postfix/proc[pid]: message
    std::stringstream ss(line);
    std::string timestamp, hostname, prog_proc, msg, proc;
    ss >> timestamp >> hostname >> prog_proc;
    std::getline(ss, msg);
    msg = msg[0] == ' ' ? msg.substr(1) : msg;

    size_t pos1 = prog_proc.find("postfix/");
    size_t pos2 = prog_proc.find("[", pos1);
    if (pos1 != std::string::npos && pos2 != std::string::npos) {
        proc = prog_proc.substr(pos1 + 8, pos2 - (pos1 + 8));
        if (proc == "smtpd") {
            parse_smtpd_msg(msg);
        }
        else if (proc == "smtp") {
            parse_smtp_msg(msg);
        }
        else {
            parse_msg(msg);
        }
    } else {
        std::cout << "No postfix/proc match found" << '\n';
        exit(1);
    }
}

void parse_msg(std::string &msg) {
    std::string type, detail;
    if (re2::RE2::FullMatch(msg, msgPtrn_withColon, &type, &detail)) {
        if (type == "warning") {
            detail = string_trimmer(detail, 66);
            record.increment_warning(detail);
        }
    } else if (re2::RE2::FullMatch(msg, msgPtrn_noColon, &detail)) {
        type = "NONE";
    } else {
        std::cout << "No message match found" << '\n';
        exit(1);
    }
}

void parse_smtp_msg(std::string &msg) {
    std::string recipient, relay, delay, status;

    if (re2::RE2::FullMatch(msg, smtpPtrn, &recipient, &relay, &delay, &status)) {
        if (status == "sent") {
            std::string recp_dom = recipient.substr(recipient.find('@') + 1);
            record.increment_deliver(recipient, recp_dom);
        }
    }
}

void parse_smtpd_msg(std::string &msg) {
    std::string client, domain, username, detail, status;

    if (re2::RE2::FullMatch(msg, clientPtrn, &client, &domain, &username)) {
        std::string user = username + "@" + gimme_domain(client).first;
        std::string user_domain = gimme_domain(client).first;
        record.increment_receive(user, user_domain);
    } else if (re2::RE2::FullMatch(msg, exceptPtrn, &status)) {
        if (status == "reject") record.increment_reject();
        else if (status == "hold") record.increment_deferred();
        else if (status == "discard") record.increment_discard();
    } else if (re2::RE2::FullMatch(msg, warnPtrn, &detail)) {
        detail = string_trimmer(detail, 66);
        record.increment_warning(detail);
    }
}

void parse_content(std::ifstream &input_file) {
    std::string line;
    while (std::getline(input_file, line)) {
        parse_log_oneline(line);
    }
    record.print_summary();
}