#include "file_reader.hpp"
#include <string> 
#include <vector>
std::ifstream read_file(const std::string &filename) {
  std::ifstream input_file;
  input_file.open(filename);
  if (!input_file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  return input_file;
}

void readFileContents(std::ifstream& file, ThreadContext* context) {
  std::string line;
  std::vector<std::string>& contents = context->getContents();
  while (std::getline(file, line)) {
    contents.emplace_back(std::move(line));
  }
}
