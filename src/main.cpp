#include "file_reader.hpp"
#include "parser.hpp"

#include <iostream>

int main(int argc, char **argv) {
    // std::ios::sync_with_stdio(false);
    // std::cin.tie(0);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream input_file = read_file(filename);

    parse_content(input_file);

    input_file.close();

    return 0;
}
