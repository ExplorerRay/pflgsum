#include "file_reader.hpp"

std::ifstream read_file(const std::string &filename) {
  std::ifstream input_file;
  input_file.open(filename);
  if (!input_file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  return input_file;
}
