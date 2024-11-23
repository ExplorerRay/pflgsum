#pragma once

#include "record.hpp"
#include <vector>
#include <memory>

class ThreadContext {
private:
  ThreadContext();
  static std::unique_ptr<ThreadContext> instance;
  ThreadContext(const ThreadContext&) = delete;
  ThreadContext& operator=(const ThreadContext&) = delete;
 
  std::vector<Record> records;
  std::vector<std::string> contents;
public:
  static ThreadContext* getInstance();
  void setThreadCount(size_t count);
  Record& getRecord(size_t index);
  std::vector<Record>& getRecords();
  std::vector<std::string>& getContents();
};

