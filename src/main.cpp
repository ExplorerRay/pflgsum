#include "file_reader.hpp"
#include "parser.hpp"

#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    MPI_Init(&argc, &argv);

    std::string filename = argv[1];
    std::ifstream input_file = read_file(filename);

    parse_content(input_file);

    input_file.close();

    MPI_Finalize();
    return 0;
}
