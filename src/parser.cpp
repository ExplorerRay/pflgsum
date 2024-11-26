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

void parse_smtp_msg(std::string &msg) {
    std::string recipient, relay, delay, status;

    if (re2::RE2::FullMatch(msg, g_smtp_pattern, &recipient, &relay, &delay, &status)) {
        if (status == "sent") {
            std::string recp_dom = recipient.substr(recipient.find('@') + 1);
            record.increment_deliver(recipient, recp_dom);
        }
    }
}

void parse_smtpd_msg(std::string &msg) {
    std::string client, domain, username, detail, status;

    if (re2::RE2::FullMatch(msg, g_client_pattern, &client, &domain, &username)) {
        std::string user = username + "@" + gimme_domain(client).first;
        std::string user_domain = gimme_domain(client).first;
        record.increment_receive(user, user_domain);
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

    double start_read = MPI_Wtime();
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input_file, line)) {
        lines.push_back(line);
    }
    double end_read = MPI_Wtime();
    if (rank == 0) {
        std::cout << "Time taken to read file: " << end_read - start_read << " seconds" << std::endl;
    }

    double start_parse = MPI_Wtime();
    int num_lines = lines.size();
    int lines_per_proc = num_lines / size;
    int remainder = num_lines % size;

    int start = rank * lines_per_proc + std::min(rank, remainder);
    int end = start + lines_per_proc + (rank < remainder ? 1 : 0);

    for (int i = start; i < end; ++i) {
        parse_log_oneline(lines[i]);
    }
    double end_parse = MPI_Wtime();
    std::cout << "Rank " << rank << " time taken to parse lines: " << end_parse - start_parse << " seconds" << std::endl;

    double start_mpi = MPI_Wtime();
    Record other_record;
    MPI_Record mpi_record;
    MPI_Record *other_mpi_records = (MPI_Record *)malloc(size * sizeof(MPI_Record));
    std::vector<size_t> user_entry_sizes(size), domain_entry_sizes(size), warning_entry_sizes(size);
    std::vector<std::vector<size_t>> user_id_sizes(size, std::vector<size_t>()), domain_id_sizes(size, std::vector<size_t>()), warning_id_sizes(size, std::vector<size_t>());
    if (rank == 0) {
        std::vector<MPI_Request> requests;
        for (int i = 1; i < size; ++i) {
            // recv user ID vector size
            MPI_Irecv(&user_entry_sizes[i], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD,&requests.emplace_back(MPI_Request()));
            // recv domain ID vector size
            MPI_Irecv(&domain_entry_sizes[i], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            // recv warning ID vector size
            MPI_Irecv(&warning_entry_sizes[i], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
        }
        MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);

        for (int i = 1; i < size; ++i) {
            user_id_sizes[i].resize(user_entry_sizes[i]);
            domain_id_sizes[i].resize(domain_entry_sizes[i]);
            warning_id_sizes[i].resize(warning_entry_sizes[i]);
        }

        for (int i = 1; i < size; ++i) {
            // recv user id size
            for (size_t j = 0; j < user_entry_sizes[i]; ++j) {
                MPI_Irecv(&user_id_sizes[i][j], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }

            // recv domain data
            for (size_t j = 0; j < domain_entry_sizes[i]; ++j) {
                MPI_Irecv(&domain_id_sizes[i][j], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }

            // recv warning data
            for (size_t j = 0; j < warning_entry_sizes[i]; ++j) {
                MPI_Irecv(&warning_id_sizes[i][j], 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }
        }
        MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);

        // resize vectors
        for (int j = 1; j < size; ++j) {
            // resize total counts
            other_mpi_records[j].total_counts.resize(6);

            other_mpi_records[j].user_identifiers.resize(user_entry_sizes[j]);
            other_mpi_records[j].user_receive_counts.resize(user_entry_sizes[j]);
            other_mpi_records[j].user_deliver_counts.resize(user_entry_sizes[j]);

            other_mpi_records[j].domain_identifiers.resize(domain_entry_sizes[j]);
            other_mpi_records[j].domain_receive_counts.resize(domain_entry_sizes[j]);
            other_mpi_records[j].domain_deliver_counts.resize(domain_entry_sizes[j]);

            other_mpi_records[j].warning_identifiers.resize(warning_entry_sizes[j]);
            other_mpi_records[j].warning_counts.resize(warning_entry_sizes[j]);

            for (size_t k = 0; k < user_entry_sizes[j]; ++k) {
                other_mpi_records[j].user_identifiers[k].resize(user_id_sizes[j][k]);
            }
            for (size_t k = 0; k < domain_entry_sizes[j]; ++k) {
                other_mpi_records[j].domain_identifiers[k].resize(domain_id_sizes[j][k]);
            }
            for (size_t k = 0; k < warning_entry_sizes[j]; ++k) {
                other_mpi_records[j].warning_identifiers[k].resize(warning_id_sizes[j][k]);
            }
        }
    } else {
        mpi_record.convert(record);

        // send user ID vector size
        size_t user_entry_size = mpi_record.user_identifiers.size();
        MPI_Send(&user_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
        // send domain ID vector size
        size_t domain_entry_size = mpi_record.domain_identifiers.size();
        MPI_Send(&domain_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
        // send warning ID vector size
        size_t warning_entry_size = mpi_record.warning_identifiers.size();
        MPI_Send(&warning_entry_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);

        for(auto user_id : mpi_record.user_identifiers) {
            size_t user_id_len = user_id.length() + 1;
            MPI_Send(&user_id_len, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
        }

        for(auto domain_id : mpi_record.domain_identifiers) {
            size_t domain_id_len = domain_id.length() + 1;
            MPI_Send(&domain_id_len, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
        }

        for(auto warning_id : mpi_record.warning_identifiers) {
            size_t warning_id_len = warning_id.length() + 1;
            MPI_Send(&warning_id_len, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        // Use non-blocking communication to receive data from other ranks
        std::vector<MPI_Request> requests;
        for (int i = 1; i < size; ++i) {
            for (size_t j = 0; j < other_mpi_records[i].user_identifiers.size(); ++j) {
                MPI_Irecv(&other_mpi_records[i].user_identifiers[j][0], other_mpi_records[i].user_identifiers[j].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }
            MPI_Irecv(&other_mpi_records[i].user_receive_counts[0], other_mpi_records[i].user_receive_counts.size(), MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            MPI_Irecv(&other_mpi_records[i].user_deliver_counts[0], other_mpi_records[i].user_deliver_counts.size(), MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));

            for (size_t j = 0; j < other_mpi_records[i].domain_identifiers.size(); ++j) {
                MPI_Irecv(&other_mpi_records[i].domain_identifiers[j][0], other_mpi_records[i].domain_identifiers[j].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }
            MPI_Irecv(&other_mpi_records[i].domain_receive_counts[0], other_mpi_records[i].domain_receive_counts.size(), MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            MPI_Irecv(&other_mpi_records[i].domain_deliver_counts[0], other_mpi_records[i].domain_deliver_counts.size(), MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));

            for (size_t j = 0; j < other_mpi_records[i].warning_identifiers.size(); ++j) {
                MPI_Irecv(&other_mpi_records[i].warning_identifiers[j][0], other_mpi_records[i].warning_identifiers[j].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
            }
            MPI_Irecv(&other_mpi_records[i].warning_counts[0], other_mpi_records[i].warning_counts.size(), MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));

            MPI_Irecv(&other_mpi_records[i].total_counts[0], 6, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, &requests.emplace_back(MPI_Request()));
        }
        MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);

        for (int i = 1; i < size; ++i) {
            // remove null terminator
            for (size_t j = 0; j < other_mpi_records[i].user_identifiers.size(); ++j) {
                other_mpi_records[i].user_identifiers[j].pop_back();
            }
            for (size_t j = 0; j < other_mpi_records[i].domain_identifiers.size(); ++j) {
                other_mpi_records[i].domain_identifiers[j].pop_back();
            }
            for (size_t j = 0; j < other_mpi_records[i].warning_identifiers.size(); ++j) {
                other_mpi_records[i].warning_identifiers[j].pop_back();
            }
            other_record.convert(other_mpi_records[i]);
            record.aggregate(other_record);
        }
        record.print_summary();
    } else {
        // send user data
        for(auto user_id : mpi_record.user_identifiers) {
            size_t user_id_len = user_id.length() + 1;
            MPI_Send(user_id.c_str(), user_id_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        size_t user_entry_size = mpi_record.user_identifiers.size();
        MPI_Send(&mpi_record.user_receive_counts[0], user_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&mpi_record.user_deliver_counts[0], user_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
        // send domain data
        for(auto domain_id : mpi_record.domain_identifiers) {
            size_t domain_id_len = domain_id.length() + 1;
            MPI_Send(domain_id.c_str(), domain_id_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        size_t domain_entry_size = mpi_record.domain_identifiers.size();
        MPI_Send(&mpi_record.domain_receive_counts[0], domain_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&mpi_record.domain_deliver_counts[0], domain_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
        // send warning data
        for(auto warning_id : mpi_record.warning_identifiers) {
            size_t warning_id_len = warning_id.length() + 1;
            MPI_Send(warning_id.c_str(), warning_id_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        size_t warning_entry_size = mpi_record.warning_identifiers.size();
        MPI_Send(&mpi_record.warning_counts[0], warning_entry_size, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
        // send total counts
        MPI_Send(&mpi_record.total_counts[0], 6, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
    }

    double end_mpi = MPI_Wtime();
    std::cout << "Rank " << rank << " time taken for MPI communication: " << end_mpi - start_mpi << " seconds" << std::endl;
}
