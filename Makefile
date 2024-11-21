CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -O3
INCLUDE = -I./include

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = pflgsum

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	-rm -f $(OBJS) $(TARGET)
