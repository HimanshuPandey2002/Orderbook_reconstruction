# CXX = g++
# CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native
# TARGET = reconstruction_$(USER)
# SOURCES = main.cpp orderbook.cpp
# OBJECTS = $(SOURCES:.cpp=.o)

# .PHONY: all clean test

# all: $(TARGET)

# $(TARGET): $(OBJECTS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# %.o: %.cpp orderbook.h
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJECTS) $(TARGET) output_mbp.csv

# test: $(TARGET)
# 	./$(TARGET) mbo.csv
# 	@echo "Test completed. Check output_mbp.csv"

# install:
# 	@echo "No installation needed. Binary is ready to use."

# # Debug build
# debug: CXXFLAGS = -std=c++17 -g -Wall -Wextra -DDEBUG
# debug: $(TARGET)

# # Performance build with profiling
# profile: CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -pg
# profile: $(TARGET)


CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native
TARGET = reconstruction_$(USER)

# Source files - check both current directory and src/ directory
SRCDIR = src
SOURCES = main.cpp orderbook.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Try to find sources in src/ directory if they exist
ifneq ($(wildcard $(SRCDIR)/main.cpp),)
    VPATH = $(SRCDIR)
    SOURCES_WITH_PATH = $(addprefix $(SRCDIR)/, $(SOURCES))
else
    SOURCES_WITH_PATH = $(SOURCES)
endif

.PHONY: all clean test debug profile

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule for object files - handles both src/ and current directory
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure we can find the header file
main.o: orderbook.h
orderbook.o: orderbook.h

clean:
	rm -f $(OBJECTS) $(TARGET) output_mbp.csv *.o

test: $(TARGET)
	./$(TARGET) mbo_dummy.csv
	@echo "Test completed. Check output_mbp.csv"

install:
	@echo "No installation needed. Binary is ready to use."

# Debug build
debug: CXXFLAGS = -std=c++17 -g -Wall -Wextra -DDEBUG
debug: $(TARGET)

# Performance build with profiling
profile: CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -pg
profile: $(TARGET)