#include "parser.hpp"
#include "utils.hpp"

std::unordered_map<std::string, std::vector<std::string>> mailInfo;

// domain to message count
std::unordered_map<std::string, int> domainCount;
// user to message count
std::unordered_map<std::string, int> userCount;
// warnings
std::vector<std::string> warnings;

int deliverCount = 0; // status=sent
int recvCount = 0;
int rejectCount = 0;
int deferredCount = 0;
int bouncedCount = 0;
int discardedCount = 0;


void parse_log_oneline(std::string &line) {
    // timestamp hostname postfix/proc[pid]: message
    std::regex logPattern(R"((\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[\+\-]\d{2}:\d{2})\s+(\S+)\s+postfix\/(\w+)\[\d+\]:\s+(.*))");

    std::smatch match;
    if(std::regex_match(line, match, logPattern)) {
        std::string proc = match[3].str();
        std::string msg = match[4].str();
        if (proc == "smtpd" || proc == "smtp") {
            parse_smtpd_msg(msg);
        }
        else{
            parse_msg(msg);
        }
        return;
    }
    else {
        std::cout << "No syslog match found" << '\n';
        exit(1);
    }
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
            warnings.emplace_back(detail);
        }

        return;
    }
    else if(std::regex_match(msg, match, msgPtrn_noColon)) {
        std::string type = "NONE";
        std::string detail = match[0].str();

        if (mailInfo.find(type) == mailInfo.end()) {
            mailInfo[type] = std::vector<std::string>();
        }
        else{
            mailInfo[type].emplace_back(detail);
        }
    }
    else {
        std::cout << "No message match found" << '\n';
        exit(1);
    }
}

void parse_smtpd_msg(std::string &msg) {
    // std::regex msgPtrn_withColon(R"((\w+):\s+(.*))");

    std::regex clientPtrn(R"(\w+: client=(.+?)(,|$).*)");
    // std::regex fromPtrn(R"(from=<([^>]*)>, size=(\d+)");

    std::regex exceptPtrn(R"(\w+: (reject(?:_warning)?|hold|discard):.*)");
    std::regex smtpPtrn(R"(\w+: to=<([^>]*)>, (?:orig_to=<[^>]*>, )?relay=([^,]+), (?:conn_use=[^,]+, )?delay=([^,]+), (?:delays=[^,]+, )?(?:dsn=[^,]+, )?status=(\S+).*)");
    std::regex warnPtrn(R"(warning: (.+))");

    std::smatch match;
    if(std::regex_match(msg, match, clientPtrn)) {
        recvCount++;

        // DOMAIN NOT YET
        gimme_domain(match[1].str());
    }
    else if(std::regex_match(msg, match, smtpPtrn)){
        std::string recipient = match[1].str();
        std::string status = match[4].str();

        if (status == "sent") {
            deliverCount++;
        }

        // DOMAIN NOT YET
        gimme_domain(recipient);
    }
    else if(std::regex_match(msg, match, exceptPtrn)){
        std::string status = match[1].str();

        if (status == "reject") {
            rejectCount++;
        }
        else if (status == "hold") {
            deferredCount++;
        }
        else if (status == "discard") {
            discardedCount++;
        }
    }
    else if(std::regex_match(msg, match, warnPtrn)){
        warnings.emplace_back(match[1].str());
    }
    else {
        // std::cout << "No smtpd message match found" << '\n';
        // exit(1);
    }
}

void parse_content(std::ifstream &input_file) {
    std::string line;
    // std::string output;

    while (std::getline(input_file, line)) {
        parse_log_oneline(line);
    }

    std::cout << "Received: " << recvCount << '\n';
    std::cout << "Delivered: " << deliverCount << '\n';
    std::cout << "Rejected: " << rejectCount << '\n';

    // for (auto &mail : mailInfo) {
    //     std::cout << mail.first << '\n';
    //     for (auto &detail : mail.second) {
    //         std::cout << detail << '\n';
    //     }
    // }

    // return output;
}


// void print_summary() {
//     std::unordered_map<std::string, TrafficSummary> senderStats;
//     std::unordered_map<std::string, TrafficSummary> recipientStats;
// }
