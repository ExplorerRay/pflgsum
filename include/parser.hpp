#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

void parse_log_oneline(std::string &line);
void parse_msg(std::string &msg);
void parse_smtp_msg(std::string &msg);
void parse_smtpd_msg(std::string &msg);
void parse_content(std::ifstream &input_file);

class Record {
public:
    /**
     * @struct UserEntry
     * @brief Structure to store user-specific email transaction data.
     */
    struct UserEntry {
        std::string identifier; ///< User identifier.
        uint64_t receive_count; ///< Count of received messages for the user.
        uint64_t deliver_count; ///< Count of delivered messages for the user.
    };

    /**
     * @struct DomainEntry
     * @brief Structure to store domain-specific email transaction data.
     */
    struct DomainEntry {
        std::string identifier; ///< Domain identifier.
        uint64_t receive_count; ///< Count of received messages for the domain.
        uint64_t deliver_count; ///< Count of delivered messages for the domain.
    };

private:
    std::unordered_map<std::string, UserEntry> user_map; ///< Map of user entries indexed by user identifier.
    std::unordered_map<std::string, DomainEntry> domain_map; ///< Map of domain entries indexed by domain identifier.
    std::unordered_map<std::string, uint64_t> warning_map; ///< Map of warnings indexed by warning identifier.
    uint64_t deliver_count; ///< Total count of delivered messages.
    uint64_t receive_count; ///< Total count of received messages.
    uint64_t reject_count; ///< Total count of rejected messages.
    uint64_t deferred_count; ///< Total count of deferred messages.
    uint64_t bounce_count; ///< Total count of bounced messages.
    uint64_t discard_count; ///< Total count of discarded messages.

public:
    /**
     * @brief Constructor to initialize the Record object.
     */
    Record();

    /**
     * @brief Create or get a user entry by identifier.
     * @param identifier The user identifier.
     * @return Reference to the UserEntry object.
     */
    UserEntry& create_or_get_user(std::string identifier);

    /**
     * @brief Create or get a domain entry by identifier.
     * @param identifier The domain identifier.
     * @return Reference to the DomainEntry object.
     */
    DomainEntry& create_or_get_domain(std::string identifier);

    /**
     * @brief Create or get a warning entry by identifier.
     * @param identifier The warning identifier.
     * @return Reference to the warning count.
     */
    uint64_t& create_or_get_warning(std::string identifier);

    /**
     * @brief Increment the receive count for a user.
     * @param identifier The user identifier.
     */
    void increment_user_receive(std::string identifier);

    /**
     * @brief Increment the deliver count for a user.
     * @param identifier The user identifier.
     */
    void increment_user_deliver(std::string identifier);

    /**
     * @brief Increment the receive count for a domain.
     * @param identifier The domain identifier.
     */
    void increment_domain_receive(std::string identifier);

    /**
     * @brief Increment the deliver count for a domain.
     * @param identifier The domain identifier.
     */
    void increment_domain_deliver(std::string identifier);

    /**
     * @brief Increment the warning count for a specific warning.
     * @param identifier The warning identifier.
     */
    void increment_warning(std::string identifier);

    /**
     * @brief Increment the deliver count.
     */
    void increment_deliver();

    /**
     * @brief Increment the receive count.
     */
    void increment_receive();

    /**
     * @brief Increment the reject count.
     */
    void increment_reject();

    /**
     * @brief Increment the deferred count.
     */
    void increment_deferred();

    /**
     * @brief Increment the bounce count.
     */
    void increment_bounce();

    /**
     * @brief Increment the discard count.
     */
    void increment_discard();

    /**
     * @brief Print a summary of all recorded data.
     */
    void print_summary();
};
