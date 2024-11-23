#include "file_reader.hpp"
#include "parser.hpp"
#include "threadContext.hpp"
#include "threadManager.hpp"
#include "reducer.hpp"

#include <iostream>
#include <chrono>

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <filename> <threadCount> <mode>\n";
        std::cerr << "  mode: 0 - linear reduce, 1 - tree reduce\n";
        return 1;
    }

    using namespace std::chrono;

    time_point<high_resolution_clock> startTime = high_resolution_clock::now();
    
    std::string filename = argv[1];
    int threadCount = std::stoi(argv[2]);
    int mode = std::stoi(argv[3]);

    if (threadCount <= 0) {
      std::cerr << "Thread count should be greater than 0\n";
      return 1;
    }

    std::ifstream input_file = read_file(filename);
  
    ThreadContext* threadContext = ThreadContext::getInstance();
    ThreadManager threadManager;

    threadContext->setThreadCount(threadCount);
    readFileContents(input_file, threadContext);
    int contentSize = threadContext->getContents().size();

    time_point<high_resolution_clock> inputTime = high_resolution_clock::now();

    for (int i = 0; i < threadCount; i++) {
      int start = i * contentSize / threadCount;
      int end = (i + 1) * contentSize / threadCount;
      std::cout << "Thread " << i << " will parse from " << start << " to " << end << std::endl;
      threadManager.add_thread(parse_content, i, start, end, threadContext);
    }
    threadManager.joinAll();
    threadManager.clear();

    time_point<high_resolution_clock> parseTime = high_resolution_clock::now();

    if (mode == 0) {
      linearReduce(threadContext->getRecords());
    } else if(mode == 1) {
      if (threadCount & (threadCount - 1)) {
        std::cerr << "Thread count should be a power of 2 For tree reduce\n";
        return 1;
      }
      for (int i = 0; i < threadCount; i+=2) {
        threadManager.add_thread(treeReduce, i, &threadManager, threadContext);
      }
      threadManager.joinAll();
    }
    
    time_point<high_resolution_clock> reduceTime = high_resolution_clock::now();

    threadContext->getRecord(0).print_summary();
    input_file.close();

    time_point<high_resolution_clock> outputTime = high_resolution_clock::now();

    duration<double> inputDuration = (inputTime - startTime);
    duration<double> parseDuration = (parseTime - inputTime);
    duration<double> reduceDuration = (reduceTime - parseTime);
    duration<double> outputDuration = (outputTime - reduceTime);

    std::cout << "inputDuration = " << inputDuration << std::endl;
    std::cout << "parseDuration = " << parseDuration << std::endl;
    std::cout << "reduceDuration = " << reduceDuration << std::endl;
    std::cout << "outputDuration = " << outputDuration << std::endl;

    return 0;
}
