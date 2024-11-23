#include "threadManager.hpp"

ThreadManager::ThreadManager() = default;
ThreadManager::~ThreadManager() {
  for (auto &thread : threads) {
    if (thread != nullptr && thread->joinable()) {
      thread->join();
    }
  }
}

size_t ThreadManager::getThreadCount() const {
  return threads.size();
}

std::shared_ptr<std::thread> ThreadManager::getThread(size_t index) {
  if (index >= threads.size()) {
    return nullptr;
  }
  return threads[index];
}

void ThreadManager::joinAll() {
  for (auto &thread : threads) {
    if (thread != nullptr && thread->joinable()) {
      thread->join();
    }
  }
}
