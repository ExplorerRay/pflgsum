#pragma once

#include <fstream>
#include <string>
#include <unistd.h>
#include "threadContext.hpp"

std::ifstream read_file(const std::string &filename);
void readFileContents(std::ifstream& file, ThreadContext* context);
