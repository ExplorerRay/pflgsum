#include "file_reader.hpp"
#include "parser.hpp"
#include "threadContext.hpp"
#include "threadManager.hpp"

#include <iostream>

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream input_file = read_file(filename);
  
    ThreadContext* threadContext = ThreadContext::getInstance();
    ThreadManager threadManager;

    threadContext->setThreadCount(2);
    readFileContents(input_file, threadContext);
    int contentSize = threadContext->getContents().size();

    threadManager.add_thread(parse_content, 0, 0, contentSize/2, threadContext);
    threadManager.add_thread(parse_content, 1, contentSize/2, contentSize, threadContext);
    threadManager.joinAll();
   // parse_content(0, 0, threadContext->getContents().size(), threadContext);
    input_file.close();

    return 0;
}
