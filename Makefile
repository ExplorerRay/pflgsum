CXX = mpicxx
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -Ofast
DEPENDENCIES = -lre2
INCLUDE = -I./include

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))
TARGET = pflgsum

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) $(DEPENDENCIES)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	-rm -f $(OBJS) $(TARGET)
