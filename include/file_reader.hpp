#pragma once

#include <fstream>
#include <string>
#include <unistd.h>

std::ifstream read_file(const std::string &filename);
