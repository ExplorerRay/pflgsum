#include "reducer.hpp"


void linearReduce(std::vector<Record>& records) {
    if (records.size() == 0) {
        return;
    }
    Record& result = records[0];
    for (size_t i = 1; i < records.size(); i++) {
        result += records[i];
    }
}

void treeReduce(int threadID, ThreadManager* threadManager, ThreadContext* threadContext) {
    std::vector<Record>& records = threadContext->getRecords();
    if (records.size() <= 1) {
        return;
    }
    int level = 0;
    size_t twoPower = 2;
    while (twoPower <= records.size()) {
      if (threadID % twoPower == 0) {
        if (level != 0) {
          auto waitedThread = threadManager->getThread(threadID + twoPower / 4);
          if (waitedThread != nullptr && waitedThread->joinable()) {
            waitedThread->join();
          }
        }
        records[threadID] += records[threadID + twoPower / 2];
        level++;
        twoPower *= 2;
      }
      else break;
    }
}
