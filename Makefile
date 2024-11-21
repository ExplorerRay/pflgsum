CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -Ofast
INCLUDE = -I./include

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))
TARGET = pflgsum

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	-rm -f $(OBJS) $(TARGET)
