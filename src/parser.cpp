#include "parser.hpp"
#include "utils.hpp"

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <regex>


using UserEntry = Record::UserEntry;
using DomainEntry = Record::DomainEntry;

Record::Record() {
    deliver_count = 0;
    receive_count = 0;
    reject_count = 0;
    deferred_count = 0;
    bounce_count = 0;
    discard_count = 0;
}

UserEntry& Record::create_or_get_user(std::string identifier) {
    if (user_map.find(identifier) == user_map.end()) {
        struct UserEntry user_entry;
        user_entry.identifier = identifier;
        user_entry.receive_count = 0;
        user_entry.deliver_count = 0;
        user_map[identifier] = user_entry;
    }

    return user_map[identifier];
}

DomainEntry& Record::create_or_get_domain(std::string identifier) {
    if (domain_map.find(identifier) == domain_map.end()) {
        struct DomainEntry domain_entry;
        domain_entry.identifier = identifier;
        domain_entry.receive_count = 0;
        domain_entry.deliver_count = 0;
        domain_map[identifier] = domain_entry;
    }

    return domain_map[identifier];
}

uint64_t& Record::create_or_get_warning(std::string identifier) {
    if (warning_map.find(identifier) == warning_map.end()) {
        warning_map[identifier] = 0;
    }
    return warning_map[identifier];
}

void Record::increment_user_receive(std::string identifier) {
    this->create_or_get_user(identifier).receive_count++;
    this->receive_count++;
}

void Record::increment_user_deliver(std::string identifier) {
    this->create_or_get_user(identifier).deliver_count++;
    this->deliver_count++;
}

void Record::increment_domain_receive(std::string identifier) {
    this->create_or_get_domain(identifier).receive_count++;
    this->receive_count++;
}

void Record::increment_domain_deliver(std::string identifier) {
    this->create_or_get_domain(identifier).deliver_count++;
    this->deliver_count++;
}

void Record::increment_warning(std::string identifier) {
    this->create_or_get_warning(identifier)++;
}

void Record::increment_reject() {
    this->reject_count++;
}

void Record::increment_deferred() {
    this->deferred_count++;
}

void Record::increment_bounce() {
    this->bounce_count++;
}

void Record::increment_discard() {
    this->discard_count++;
}

void Record::print_summary() {
    std::cout << "Grand Totals\n";
    std::cout << "------------\n";
    std::cout << "Messages\n\n";
    std::cout << "Received: " << this->receive_count << '\n';
    std::cout << "Delivered: " << this->deliver_count << '\n';
    std::cout << "Deferred: " << this->deferred_count << '\n';
    std::cout << "Bounced: " << this->bounce_count << '\n';
    std::cout << "Discarded: " << this->discard_count << '\n';
    std::cout << "Rejected: " << this->reject_count << "\n\n\n";

    std::cout << "Send users by message count\n";
    std::cout << "------------\n";
    std::vector<struct UserEntry> user_entries;
    for (auto& entry : this->user_map) {
        user_entries.push_back(entry.second);
    }
    std::sort(user_entries.begin(), user_entries.end(), [](const struct UserEntry& a, const struct UserEntry& b) {
        return a.deliver_count > b.deliver_count;
    });
    for (auto& user : user_entries) {
        if (user.deliver_count != 0)
            std::cout << user.deliver_count << "\t" << user.identifier << '\n';
    }
    std::cout << '\n';

    std::cout << "Send domains by message count\n";
    std::cout << "------------\n";
    std::vector<struct DomainEntry> domain_entries;
    for (auto& entry : this->domain_map) {
        domain_entries.push_back(entry.second);
    }
    std::sort(domain_entries.begin(), domain_entries.end(), [](const struct DomainEntry& a, const struct DomainEntry& b) {
        return a.deliver_count > b.deliver_count;
    });
    for (auto& domain : domain_entries) {
        std::cout << domain.deliver_count << "\t" << domain.identifier << '\n';
    }
    std::cout << '\n';

    std::cout << "Recieve users by message count\n";
    std::cout << "------------\n";
    std::sort(user_entries.begin(), user_entries.end(), [](const struct UserEntry& a, const struct UserEntry& b) {
        return a.receive_count > b.receive_count;
    });
    for (auto& user : user_entries) {
        if (user.receive_count != 0)
            std::cout << user.receive_count << "\t" << user.identifier << '\n';
    }
    std::cout << '\n';

    std::cout << "Recieve domains by message count\n";
    std::cout << "------------\n";
    std::sort(domain_entries.begin(), domain_entries.end(), [](const struct DomainEntry& a, const struct DomainEntry& b) {
        return a.receive_count > b.receive_count;
    });
    for (auto& domain : domain_entries) {
        std::cout << domain.receive_count << "\t" << domain.identifier << '\n';
    }
    std::cout << '\n';

    std::cout << "Warnings\n";
    std::cout << "------------\n";
    std::vector<std::pair<std::string, uint64_t>> warning_entries;
    for (auto& entry : this->warning_map) {
        warning_entries.push_back(std::make_pair(entry.first, entry.second));
    }
    std::sort(warning_entries.begin(), warning_entries.end(), [](const std::pair<std::string, uint64_t>& a, const std::pair<std::string, uint64_t>& b) {
        return a.second > b.second;
    });

    for (auto& warning : warning_entries) {
        std::cout << warning.second << "\t" << warning.first << '\n';
    }
    return;
}
Record record;

void parse_log_oneline(std::string &line) {
    // timestamp hostname postfix/proc[pid]: message
    // use stringstream to split to four parts with space
    std::stringstream ss(line);
    std::string timestamp, hostname, prog_proc, msg, proc;
    ss >> timestamp >> hostname >> prog_proc;
    std::getline(ss, msg);
    msg = msg[0] == ' ' ? msg.substr(1) : msg;

    std::regex progProcPtrn(R"(postfix\/(\w+)\[(\d+)\]:)");
    std::smatch match;
    if(std::regex_match(prog_proc, match, progProcPtrn)) {
        proc = match[1].str();
    }
    else {
        std::cout << "No postfix/proc match found" << '\n';
        exit(1);
    }

    if (proc == "smtpd") parse_smtpd_msg(msg);
    else if (proc == "smtp") parse_smtp_msg(msg);
    else parse_msg(msg);

    return;
}

void parse_msg(std::string &msg) {
    // split message to two conditions with another colon(:) at the start or not
    std::regex msgPtrn_withColon(R"((\w+):\s+(.*))");
    std::regex msgPtrn_noColon(R"(.+)");

    std::smatch match;
    if(std::regex_match(msg, match, msgPtrn_withColon)) {
        // smtp, anvil, qmgr, cleanup, scache, master

        std::string type = match[1].str();
        std::string detail = match[2].str();
        if (type == "warning"){
            detail = string_trimmer(detail, 66);
            record.increment_warning(detail);
        }

        return;
    }
    else if(std::regex_match(msg, match, msgPtrn_noColon)) {
        std::string type = "NONE";
        std::string detail = match[0].str();
    }
    else {
        std::cout << "No message match found" << '\n';
        exit(1);
    }
}

void parse_smtp_msg(std::string &msg) {
    std::regex smtpPtrn(R"(\w+: to=<([^>]*)>, (?:orig_to=<[^>]*>, )?relay=([^,]+), (?:conn_use=[^,]+, )?delay=([^,]+), (?:delays=[^,]+, )?(?:dsn=[^,]+, )?status=(\S+).*)");
    std::smatch match;
    if(std::regex_match(msg, match, smtpPtrn)){
        std::string recipient = match[1].str();
        std::string status = match[4].str();

        if (status == "sent") {
            record.increment_user_deliver(recipient);

            // For domain
            // parse email to domain with @
            std::string recp_dom = recipient.substr(recipient.find('@') + 1);
            record.increment_domain_deliver(recp_dom);
        }
    }
}

void parse_smtpd_msg(std::string &msg) {
    std::regex clientPtrn(R"(\w+: client=(.+?)(,|$).+sasl_username=(\w+))");
    std::regex exceptPtrn(R"(\w+: (reject(?:_warning)?|hold|discard):.*)");
    std::regex warnPtrn(R"(warning: (.+))");

    std::smatch match;
    if(std::regex_match(msg, match, clientPtrn)) {
        record.increment_user_receive(match[3].str() + "@" + gimme_domain(match[1].str()).first);
        record.increment_domain_receive(gimme_domain(match[1].str()).first);
    }
    else if(std::regex_match(msg, match, exceptPtrn)){
        std::string status = match[1].str();

        if (status == "reject") {
            record.increment_reject();
        }
        else if (status == "hold") {
            record.increment_deferred();
        }
        else if (status == "discard") {
            record.increment_discard();
        }
    }
    else if(std::regex_match(msg, match, warnPtrn)){
        std::string detail = match[1].str();
        detail = string_trimmer(detail, 66);
        record.increment_warning(detail);
    }
    else {
        // std::cout << "No smtpd message match found" << '\n';
        // exit(1);
    }
}

void parse_content(std::ifstream &input_file) {
    std::string line;

    while (std::getline(input_file, line)) {
        parse_log_oneline(line);
    }
    record.print_summary();

    return;
}
