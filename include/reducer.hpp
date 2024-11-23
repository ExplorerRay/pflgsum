#pragma once

#include <vector>
#include "record.hpp"
#include "threadManager.hpp"
#include "threadContext.hpp"

void linearReduce(std::vector<Record>& records);
void treeReduce(int threadID, ThreadManager* threadManager, ThreadContext* threadContext);
