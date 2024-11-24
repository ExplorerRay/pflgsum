#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

class Record;
class MPI_Record;

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

    // converting to another class for MPI send and receive
    friend class MPI_Record;

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
    UserEntry& create_or_get_user(std::string& identifier);

    /**
     * @brief Create or get a domain entry by identifier.
     * @param identifier The domain identifier.
     * @return Reference to the DomainEntry object.
     */
    DomainEntry& create_or_get_domain(std::string& identifier);

    /**
     * @brief Create or get a warning entry by identifier.
     * @param identifier The warning identifier.
     * @return Reference to the warning count.
     */
    uint64_t& create_or_get_warning(std::string& identifier);

    /**
     * @brief Increment the deliver count for a user and domain.
     */
    void increment_deliver(std::string& user_identifier, std::string& domain_identifier);


    /**
     * @brief Increment the receive count for a user and domain
     */
    void increment_receive(std::string& user_identifier, std::string& domain_identifier);

    /**
     * @brief Increment the warning count for a specific warning.
     * @param identifier The warning identifier.
     */
    void increment_warning(std::string& identifier);

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
     * @brief Convert MPI_Record object to Record object.
     */
    Record convert(MPI_Record& mpi_record);

    /**
     * @brief Increment the discard count.
     */
    void increment_discard();

    /**
     * @brief Aggregate two Record objects.
     */
    void aggregate(Record& other_record);

    /**
     * @brief operator+= overload to aggregate two Record objects.
     */
    void operator+=(Record& other_record);

    /**
     * @brief Print a summary of all recorded data.
     */
    void print_summary();
};

// for MPI send and receive
class MPI_Record {
public:
    /**
     * @brief Constructor to initialize the MPI_Record object.
     */
    MPI_Record() = default;

    /**
     * @brief Convert Record object to MPI_Record object.
     */
    MPI_Record convert(Record& record);

// private:
    std::vector<std::string> user_identifiers;
    std::vector<uint64_t> user_receive_counts;
    std::vector<uint64_t> user_deliver_counts;
    std::vector<std::string> domain_identifiers;
    std::vector<uint64_t> domain_receive_counts;
    std::vector<uint64_t> domain_deliver_counts;
    std::vector<std::string> warning_identifiers;
    std::vector<uint64_t> warning_counts;

    std::vector<uint64_t> total_counts;
    // index 0: deliver_count, 1: receive_count, 2: reject_count, 3: deferred_count, 4: bounce_count, 5: discard_count
};
