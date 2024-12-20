#include "file_reader.hpp"
#include "parser.hpp"
#include "threadContext.hpp"
#include "threadManager.hpp"
#include "reducer.hpp"

#include <iostream>
#include <chrono>

int main(int argc, char **argv) {
    using namespace std::chrono;

    time_point<high_resolution_clock> startTime = high_resolution_clock::now();
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <filename> <threadCount> <mode>\n";
        std::cerr << "  mode: 0 - linear reduce, 1 - tree reduce\n";
        return 1;
    }

    
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

    for (int i = 1; i < threadCount; i++) {
      int start = i * contentSize / threadCount;
      int end = (i + 1) * contentSize / threadCount;
      threadManager.add_thread(parse_content, i, start, end, threadContext, mode, &threadManager);
    }
    parse_content(0, 0, contentSize / threadCount, threadContext, mode, &threadManager);
    threadManager.joinAll();

    time_point<high_resolution_clock> parseTime = high_resolution_clock::now();

    threadContext->getRecord(0).print_summary(false);
    input_file.close();

    time_point<high_resolution_clock> outputTime = high_resolution_clock::now();

    duration<double> inputDuration = (inputTime - startTime);
    duration<double> parseDuration = (parseTime - inputTime);
    duration<double> outputDuration = (outputTime - parseTime);
    duration<double> totalDuration = (outputTime - startTime);

    std::cout << "inputDuration = " << inputDuration << std::endl;
    std::cout << "parseDuration = " << parseDuration << std::endl;
    std::cout << "outputDuration = " << outputDuration << std::endl;
    std::cout << "totalDuration = " << totalDuration << std::endl;
    return 0;
}
