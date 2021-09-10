cur_mkfile := $(abspath $(lastword $(MAKEFILE_LIST)))
MISAKA_INCLUDE :=$(patsubst %/, %, $(dir $(cur_mkfile)))
SRC = .
ALLFILE = $(MISAKA_INCLUDE)/base/*.cc

CXXFLAGS = -g -Wall -std=c++17 -Wextra -Werror \
		-Wconversion -Wno-unused-parameter \
		-Wold-style-cast -Woverloaded-virtual \
		-Wpointer-arith -Wshadow -Wwrite-strings \
		-march=native -rdynamic \
		-I$(MISAKA_INCLUDE)
LDFLAGS = -lpthread -lrt

thread_t : $(SRC)/test_thread.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

AsyncLogging_t : $(SRC)/test_AsyncLogging.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

LogStream_t : $(SRC)/test_LogStream.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

Logger_t : $(SRC)/test_Logger.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY:
clear:
	rm -rf 	*_t