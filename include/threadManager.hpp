#pragma once

#include <thread>
#include <vector>
#include <memory>

#include <system_error>
#include "utils.hpp"

class ThreadManager {
  std::vector<std::shared_ptr<std::thread>> threads;
public:
  ThreadManager(); 
  ~ThreadManager();

// putting implementation in header file because it need to be known at compile time
// using && for universal reference
// using forward to preserve lvalue/rvalue-ness
  template <typename Function, typename... Args>
  void add_thread(Function &&f, Args &&... args) {
    try {
      threads.emplace_back(std::make_shared<std::thread>(
        std::forward<Function>(f), 
        std::forward<Args>(args)...
      ));
    } catch (const std::system_error &e) {
      printException(e, "std::system_error", __FUNCTION__);
      throw e;
    }
  }

  size_t getThreadCount() const;

  std::shared_ptr<std::thread> getThread(size_t index);
  void joinAll();
};
