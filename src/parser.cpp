#include "parser.hpp"
#include "record.hpp"
#include "utils.hpp"

#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <re2/re2.h>
#include <mpi.h>


Record record;

re2::RE2 g_pattern_with_colon(R"((\w+):\s+(.*))");
re2::RE2 g_pattern_no_colon(R"(.+)");
re2::RE2 g_smtp_pattern(R"(\w+: to=<([^>]*)>, (?:orig_to=<[^>]*>, )?relay=([^,]+), (?:conn_use=[^,]+, )?delay=([^,]+), (?:delays=[^,]+, )?(?:dsn=[^,]+, )?status=(\S+).*)");
re2::RE2 g_qmgr_pattern(R"(\w+: from=<([^>]*)>, .*)");
re2::RE2 g_client_pattern(R"(\w+: client=(.+?)(,|$).+sasl_username=(\w+))");
re2::RE2 g_except_pattern(R"(\w+: (reject(?:_warning)?|hold|discard):.*)");
re2::RE2 g_warn_pattern(R"(warning: (.+))");

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
        else if (proc == "qmgr") {
            parse_qmgr_msg(msg);
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
    if (re2::RE2::FullMatch(msg, g_pattern_with_colon, &type, &detail)) {
        if (type == "warning") {
            detail = string_trimmer(detail, 66);
            record.increment_warning(detail);
        }
    } else if (re2::RE2::FullMatch(msg, g_pattern_no_colon, &detail)) {
        type = "NONE";
    } else {
        std::cout << "No message match found" << '\n';
        exit(1);
    }
}

void parse_qmgr_msg(std::string &msg) {
    std::string sender;

    if (re2::RE2::FullMatch(msg, g_qmgr_pattern, &sender)) {
        std::string send_dom = sender.substr(sender.find('@') + 1);
        record.increment_sender(sender, send_dom);
    }
}

void parse_smtp_msg(std::string &msg) {
    std::string recp, relay, delay, status;

    if (re2::RE2::FullMatch(msg, g_smtp_pattern, &recp, &relay, &delay, &status)) {
        if (status == "sent") {
            record.increment_deliver();
            std::string recp_dom = recp.substr(recp.find('@') + 1);
            record.increment_recipient(recp, recp_dom);
        }
    }
}

void parse_smtpd_msg(std::string &msg) {
    std::string client, domain, username, detail, status;

    if (re2::RE2::FullMatch(msg, g_client_pattern, &client, &domain, &username)) {
        std::string user = username + "@" + gimme_domain(client).first;
        std::string user_domain = gimme_domain(client).first;
        record.increment_receive();
    } else if (re2::RE2::FullMatch(msg, g_except_pattern, &status)) {
        if (status == "reject") record.increment_reject();
        else if (status == "hold") record.increment_deferred();
        else if (status == "discard") record.increment_discard();
    } else if (re2::RE2::FullMatch(msg, g_warn_pattern, &detail)) {
        detail = string_trimmer(detail, 66);
        record.increment_warning(detail);
    }
}

void parse_content(std::ifstream &input_file) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input_file, line)) {
        lines.push_back(line);
    }

    int num_lines = lines.size();
    int lines_per_proc = num_lines / size;
    int remainder = num_lines % size;

    int start = rank * lines_per_proc + std::min(rank, remainder);
    int end = start + lines_per_proc + (rank < remainder ? 1 : 0);

    for (int i = start; i < end; ++i) {
        parse_log_oneline(lines[i]);
    }

    Record other_record;
    MPI_Record mpi_record, other_mpi_record;
    if (rank == 0) {
        for (int i = 1; i < size; ++i) {
            // recv user ID vector size
            size_t user_entry_size;
            MPI_Recv(&user_entry_size, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            other_mpi_record.user_identifiers.resize(user_entry_size);
            other_mpi_record.user_receive_counts.resize(user_entry_size);
            other_mpi_record.user_deliver_counts.resize(user_entry_size);
            MPI_Recv(&other_mpi_record.user_receive_counts[0], user_entry_size, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&other_mpi_record.user_deliver_counts[0], user_entry_size, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // recv total user ID
            std::vector<size_t> user_id_lens(user_entry_size);
            size_t total_user_id_len = 0;
            MPI_Recv(&user_id_lens[0], user_entry_size, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (size_t j = 0; j < user_entry_size; ++j) {
                total_user_id_len += user_id_lens[j];
            }
            std::string total_user_id;
            total_user_id.resize(total_user_id_len+1);
            MPI_Recv(&total_user_id[0], total_user_id_len+1, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_user_id.pop_back();
            // split total_user_id into individual user IDs
            other_mpi_record.split_identifiers(total_user_id, user_id_lens, 0);

            // recv domain ID vector size
            size_t domain_entry_size;
            MPI_Recv(&domain_entry_size, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            other_mpi_record.domain_identifiers.resize(domain_entry_size);
            other_mpi_record.domain_receive_counts.resize(domain_entry_size);
            other_mpi_record.domain_deliver_counts.resize(domain_entry_size);
            MPI_Recv(&other_mpi_record.domain_receive_counts[0], domain_entry_size, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&other_mpi_record.domain_deliver_counts[0], domain_entry_size, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // recv total domain ID
            std::vector<size_t> domain_id_lens(domain_entry_size);
            size_t total_domain_id_len = 0;
            MPI_Recv(&domain_id_lens[0], domain_entry_size, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (size_t j = 0; j < domain_entry_size; ++j) {
                total_domain_id_len += domain_id_lens[j];
            }
            std::string total_domain_id;
            total_domain_id.resize(total_domain_id_len+1);
            MPI_Recv(&total_domain_id[0], total_domain_id_len+1, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_domain_id.pop_back();
            // split total_domain_id into individual domain IDs
            other_mpi_record.split_identifiers(total_domain_id, domain_id_lens, 1);

            // recv warning ID vector size
            size_t warning_entry_size;
            MPI_Recv(&warning_entry_size, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            other_mpi_record.warning_identifiers.resize(warning_entry_size);
            other_mpi_record.warning_counts.resize(warning_entry_size);
            MPI_Recv(&other_mpi_record.warning_counts[0], warning_entry_size, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // recv total warning ID
            std::vector<size_t> warning_id_lens(warning_entry_size);
            size_t total_warning_id_len = 0;
            MPI_Recv(&warning_id_lens[0], warning_entry_size, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (size_t j = 0; j < warning_entry_size; ++j) {
                total_warning_id_len += warning_id_lens[j];
            }
            std::string total_warning_id;
            total_warning_id.resize(total_warning_id_len+1);
            MPI_Recv(&total_warning_id[0], total_warning_id_len+1, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_warning_id.pop_back();
            // split total_warning_id into individual warning IDs
            other_mpi_record.split_identifiers(total_warning_id, warning_id_lens, 2);

            // recv total counts
            other_mpi_record.total_counts.resize(6);
            MPI_Recv(&other_mpi_record.total_counts[0], 6, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            other_record.convert(other_mpi_record);

            record.aggregate(other_record);
        }
        record.print_summary();
    } else {
        std::vector<MPI_Request> requests;
        mpi_record.convert(record);

        // send user ID vector size
        size_t user_entry_size = mpi_record.user_identifiers.size();
        MPI_Isend(&user_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(&mpi_record.user_receive_counts[0], user_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(&mpi_record.user_deliver_counts[0], user_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        // combine all user IDs into a single string and send once
        size_t total_user_id_len = 0;
        std::vector<size_t> user_id_lens;
        std::string total_user_id = "";
        for(auto user_id : mpi_record.user_identifiers) {
            size_t user_id_len = user_id.length();
            user_id_lens.emplace_back(user_id_len);
            total_user_id_len += user_id_len;
            total_user_id += user_id;
        }
        MPI_Isend(&user_id_lens[0], user_entry_size, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(total_user_id.c_str(), total_user_id_len+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        // send domain ID vector size
        size_t domain_entry_size = mpi_record.domain_identifiers.size();
        MPI_Isend(&domain_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(&mpi_record.domain_receive_counts[0], domain_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(&mpi_record.domain_deliver_counts[0], domain_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        // combine all domain IDs into a single string and send once
        size_t total_domain_id_len = 0;
        std::vector<size_t> domain_id_lens;
        std::string total_domain_id = "";
        for(auto domain_id : mpi_record.domain_identifiers) {
            size_t domain_id_len = domain_id.length();
            total_domain_id_len += domain_id_len;
            domain_id_lens.emplace_back(domain_id_len);
            total_domain_id += domain_id;
        }
        MPI_Isend(&domain_id_lens[0], domain_entry_size, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(total_domain_id.c_str(), total_domain_id_len+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        // send warning ID vector size
        size_t warning_entry_size = mpi_record.warning_identifiers.size();
        MPI_Isend(&warning_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(&mpi_record.warning_counts[0], warning_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        // combine all warning IDs into a single string and send once
        size_t total_warning_id_len = 0;
        std::vector<size_t> warning_id_lens;
        std::string total_warning_id = "";
        for(auto warning_id : mpi_record.warning_identifiers) {
            size_t warning_id_len = warning_id.length();
            warning_id_lens.emplace_back(warning_id_len);
            total_warning_id_len += warning_id_len;
            total_warning_id += warning_id;
        }
        MPI_Isend(&warning_id_lens[0], warning_entry_size, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());
        MPI_Isend(total_warning_id.c_str(), total_warning_id_len+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        MPI_Isend(&mpi_record.total_counts[0], 6, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD, &requests.emplace_back());

        MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
    }
}
