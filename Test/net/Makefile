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

# -I$(MISAKA_INCLUDE)	// emmm, 在编译 Miska 整个库时，这个选项可能会存在很多问题, eg: Endian.h 就是一个神奇的栗子

##
# 遇到了一个很棘手的问题，跟自己前置声明的 struct tcp_info 结构体有关
# 在 TcpConnection 中我忘记再次前置声明了。。。
##
TcpServer : $(SRC)/test_TcpServer.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

EventLoopThread : $(SRC)/test_EventLoopThread.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

InetAddress : $(SRC)/test_InetAddress.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

Buffer : $(SRC)/test_Buffer.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

EventLoopThreadPool : $(SRC)/test_EventLoopThreadPool.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

TcpConnection : $(SRC)/test_TcpConnection.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

HttpServer : $(SRC)/test_HttpServer.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

Acceptor : $(SRC)/test_Acceptor.cc $(ALLFILE)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


.PHONY:
clear:
	rm -rf 	EventLoopThread  \
			EventLoopThreadPool \
			TcpConnection \
			TcpServer \
			HttpServer \
			InetAddress \
			Acceptor \
			Buffer