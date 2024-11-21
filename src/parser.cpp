#include "parser.hpp"
#include "utils.hpp"

// domain to message count
int domainID = 0;
std::unordered_map<std::string, int> DomainToID;
std::vector<std::string> IDToDomain;
std::vector<std::pair<int, int>> domainRecvCount;
std::vector<std::pair<int, int>> domainDlivCount;
// user to message count
int userID = 0;
std::unordered_map<std::string, int> UserToID;
std::vector<std::string> IDToUser;
std::vector<std::pair<int, int>> userRecvCount;
std::vector<std::pair<int, int>> userDlivCount;

// warnings
int warningID = 0;
std::unordered_map<std::string, int> WarningToID;
std::vector<std::string> IDToWarning;
std::vector<std::pair<int, int>> warningCount;

int deliverCount = 0; // status=sent
int recvCount = 0;
int rejectCount = 0;
int deferredCount = 0;
int bouncedCount = 0;
int discardedCount = 0;


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
            if (WarningToID.find(detail) == WarningToID.end()) {
                WarningToID[detail] = warningID;
                IDToWarning.emplace_back(detail);
                warningCount.emplace_back(std::make_pair(1, warningID));
                warningID++;
            }
            else{
                warningCount[WarningToID[detail]].first++;
            }
        }

        return;
    }
    else if(std::regex_match(msg, match, msgPtrn_noColon)) {
        std::string type = "NONE";
        std::string detail = match[0].str();

        // if (mailInfo.find(type) == mailInfo.end()) {
        //     mailInfo[type] = std::vector<std::string>();
        // }
        // else{
        //     mailInfo[type].emplace_back(detail);
        // }
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
            // For user
            if (UserToID.find(recipient) == UserToID.end()) {
                UserToID[recipient] = userID;
                IDToUser.emplace_back(recipient);
                userDlivCount.emplace_back(std::make_pair(1, userID));
                userID++;
            }
            else{
                userDlivCount[UserToID[recipient]].first++;
            }

            // For domain
            // parse email to domain with @
            std::string recp_dom = recipient.substr(recipient.find('@') + 1);
            if (DomainToID.find(recp_dom) == DomainToID.end()) {
                DomainToID[recp_dom] = domainID;
                IDToDomain.emplace_back(recp_dom);
                domainDlivCount.emplace_back(std::make_pair(1, domainID));
                domainID++;
            }
            else{
                domainDlivCount[DomainToID[recp_dom]].first++;
            }

            deliverCount++;
        }
    }
}

void parse_smtpd_msg(std::string &msg) {
    // std::regex msgPtrn_withColon(R"((\w+):\s+(.*))");

    std::regex clientPtrn(R"(\w+: client=(.+?)(,|$).+sasl_username=(\w+))");
    // std::regex fromPtrn(R"(from=<([^>]*)>, size=(\d+)");

    std::regex exceptPtrn(R"(\w+: (reject(?:_warning)?|hold|discard):.*)");
    std::regex warnPtrn(R"(warning: (.+))");

    std::smatch match;
    if(std::regex_match(msg, match, clientPtrn)) {
        recvCount++;
        // For domain
        std::pair<std::string, std::string> dom_ip = gimme_domain(match[1].str());
        if (DomainToID.find(dom_ip.first) == DomainToID.end()) {
            DomainToID[dom_ip.first] = domainID;
            IDToDomain.emplace_back(dom_ip.first);
            domainRecvCount.emplace_back(std::make_pair(1, domainID));
            domainID++;
        }
        else{
            domainRecvCount[DomainToID[dom_ip.first]].first++;
        }

        // For user
        std::string username = match[3].str();
        username = username + "@" + dom_ip.first;
        if (UserToID.find(username) == UserToID.end()) {
            UserToID[username] = userID;
            IDToUser.emplace_back(username);
            userRecvCount.emplace_back(std::make_pair(1, userID));
            userID++;
        }
        else{
            userRecvCount[UserToID[username]].first++;
        }
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
        std::string detail = match[1].str();
        detail = string_trimmer(detail, 66);
        if (WarningToID.find(detail) == WarningToID.end()) {
            WarningToID[detail] = warningID;
            IDToWarning.emplace_back(detail);
            warningCount.emplace_back(std::make_pair(1, warningID));
            warningID++;
        }
        else{
            warningCount[WarningToID[detail]].first++;
        }
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
    print_summary();

    return;
}


void print_summary() {
    std::cout << "Grand Totals\n";
    std::cout << "------------\n";
    std::cout << "messages\n\n";
    std::cout << "Received: " << recvCount << '\n';
    std::cout << "Delivered: " << deliverCount << '\n';
    std::cout << "Rejected: " << rejectCount << "\n\n\n";

    std::cout << "Send users by message count\n";
    std::cout << "------------\n";
    std::sort(userDlivCount.begin(), userDlivCount.end(), std::greater<std::pair<int, int>>());
    for (auto &user : userDlivCount) {
        std::cout << user.first << "\t" << IDToUser[user.second] << '\n';
    }
    std::cout << '\n';

    std::cout << "Send domains by message count\n";
    std::cout << "------------\n";
    std::sort(domainDlivCount.begin(), domainDlivCount.end(), std::greater<std::pair<int, int>>());
    for (auto &dom : domainDlivCount) {
        std::cout << dom.first << "\t" << IDToDomain[dom.second] << '\n';
    }
    std::cout << '\n';

    std::cout << "Recieve users by message count\n";
    std::cout << "------------\n";
    std::sort(userRecvCount.begin(), userRecvCount.end(), std::greater<std::pair<int, int>>());
    for (auto &user : userRecvCount) {
        std::cout << user.first << "\t" << IDToUser[user.second] << '\n';
    }
    std::cout << '\n';

    std::cout << "Recieve domains by message count\n";
    std::cout << "------------\n";
    std::sort(domainRecvCount.begin(), domainRecvCount.end(), std::greater<std::pair<int, int>>());
    for (auto &dom : domainRecvCount) {
        std::cout << dom.first << "\t" << IDToDomain[dom.second] << '\n';
    }
    std::cout << '\n';

    std::cout << "Warnings\n";
    std::cout << "------------\n";
    std::sort(warningCount.begin(), warningCount.end(), std::greater<std::pair<int, int>>());
    for (auto &warn : warningCount) {
        std::cout << warn.first << "\t" << IDToWarning[warn.second] << '\n';
    }

    return;
}
