#include "record.hpp"

#include <algorithm>
#include <string>
#include <iostream>

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

UserEntry& Record::create_or_get_user(std::string& identifier) {
    if (user_map.find(identifier) == user_map.end()) {
        struct UserEntry user_entry;
        user_entry.identifier = identifier;
        user_entry.receive_count = 0;
        user_entry.deliver_count = 0;
        user_map[identifier] = user_entry;
    }

    return user_map[identifier];
}

DomainEntry& Record::create_or_get_domain(std::string& identifier) {
    if (domain_map.find(identifier) == domain_map.end()) {
        struct DomainEntry domain_entry;
        domain_entry.identifier = identifier;
        domain_entry.receive_count = 0;
        domain_entry.deliver_count = 0;
        domain_map[identifier] = domain_entry;
    }

    return domain_map[identifier];
}

uint64_t& Record::create_or_get_warning(std::string& identifier) {
    if (warning_map.find(identifier) == warning_map.end()) {
        warning_map[identifier] = 0;
    }
    return warning_map[identifier];
}

void Record::increment_deliver(std::string& user_identifier, std::string& domain_identifier) {
    this->create_or_get_user(user_identifier).deliver_count++;
    this->create_or_get_domain(domain_identifier).deliver_count++;
    this->deliver_count++;
}

void Record::increment_receive(std::string& user_identifier, std::string& domain_identifier) {
    this->create_or_get_user(user_identifier).receive_count++;
    this->create_or_get_domain(domain_identifier).receive_count++;
    this->receive_count++;
}

void Record::increment_warning(std::string& identifier) {
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
