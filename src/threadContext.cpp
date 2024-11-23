#include "threadContext.hpp"
#include <memory>
#include <stdexcept>


std::unique_ptr<ThreadContext> ThreadContext::instance = nullptr;

ThreadContext::ThreadContext() {
  records = std::vector<Record>();
}

ThreadContext* ThreadContext::getInstance() {
  if (instance == nullptr) {
    instance = std::unique_ptr<ThreadContext>(new ThreadContext());
  }
  return instance.get();
}

void ThreadContext::setThreadCount(size_t count) {
  records.resize(count);
}

Record& ThreadContext::getRecord(size_t index) {
  if (index >= records.size()) {
    throw std::out_of_range("Index out of range");
  }
  return records[index];
}

std::vector<std::string>& ThreadContext::getContents()
{
  return contents;
}
