cur_mkfile := $(abspath $(lastword $(MAKEFILE_LIST)))
MISAKA_INCLUDE :=$(patsubst %/, %, $(dir $(cur_mkfile)))
SRC = .
ALLFILE = 	$(MISAKA_INCLUDE)/net/*.cc \
			$(MISAKA_INCLUDE)/base/*.cc \
			$(MISAKA_INCLUDE)/http/*.cc

CXXFLAGS = -g -Wall -std=c++17 -Wextra -Werror \
		-Wconversion -Wno-unused-parameter \
		-Wold-style-cast -Woverloaded-virtual \
		-Wpointer-arith -Wshadow -Wwrite-strings \
		-march=native -rdynamic \
		-I$(MISAKA_INCLUDE)
LDFLAGS = -lpthread -lrt

arknights : $(SRC)/arknights.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY:
clear:
	rm -rf 	arknights