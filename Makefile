SHELL := bash
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

COMMON_SRCS := stats.cpp date_utils.cpp
COMMON_OBJS := $(COMMON_SRCS:.cpp=.o)

SAMPLE_PROGRAMS := x_basic x_arithmetic x_stats x_indexing x_io x_construct x_intraday
PROGRAMS := df_demo $(SAMPLE_PROGRAMS)

all: $(PROGRAMS)

df_demo: main.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SAMPLE_PROGRAMS): %: %.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

main.o: main.cpp dataframe.h sample_utils.h print_utils.h stats.h date_utils.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp dataframe.h sample_utils.h print_utils.h stats.h date_utils.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	@for prog in $(PROGRAMS); do \
		printf "\n--- running %s ---\n" "$$prog"; \
		./$$prog; \
	done

clean:
	rm -f main.o $(SAMPLE_PROGRAMS:%=%.o) $(COMMON_OBJS) $(PROGRAMS)

.PHONY: all run clean
