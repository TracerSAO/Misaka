#include "../../net/TcpConnection.h"
#include "../../net/Acceptor.h"
#include "../../net/Socket.h"
#include "../../net/SocketsOps.h"
#include "../../net/InetAddress.h"
#include "../../net/EventLoop.h"
#include "../../base/Timestamp.h"
#include "../../base/Logging.h"

#include <cstdio>
#include <vector>
#include <map>

using namespace Misaka;
using namespace net;

EventLoop* g_loop = nullptr;
// std::vector<std::shared_ptr<TcpConnection>> connVec;
std::map<std::string, std::shared_ptr<TcpConnection>> connVec;


void connectionCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO << "ConnectionCallback() - (＃°Д°) connection closed???";
    
    // 能进到 connectionCallback() 只有两种状态:
    // 1. connected | 2. disconnected
    if (conn->connected())
    {
        conn->forceCloseWithDelay(3);
    }
    else
    {
        conn->forceCloseWithDelay(1);
    }
    
}

void closeCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO << "CloseCallback() -#SYS# closed connection: ";
    connVec.erase(conn->name());
    conn->connectionDestroyed();
}

void messageCallback(const TcpConnectionPtr& conn,  Buffer* meg, Timestamp receiveTime)
{
    LOG_INFO << "MessageCallback() - "
             << receiveTime.toFormattedString()
             << "meg: " << meg->toStringPiece();
    conn->send(StringPiece("(｡･∀･)ﾉﾞ嗨\n"));
}

void writeCompleteCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO << "WriteCompleteCallback() - (～￣▽￣)～";
}

void newConnection(int sockfd, const InetAddress& peeraddr)
{
    LOG_INFO << "Acceptor::handleRead() - newConnection()";

    const InetAddress localaddr(sockets::getPeerAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(g_loop,
                        "the choose One",
                        sockfd,
                        localaddr,
                        peeraddr));

    conn->setConnectionCallback(connectionCallback);
    conn->setCloseCallback(closeCallback);
    conn->setMessageCallback(messageCallback);
    conn->setWriteCompleteCallback(writeCompleteCallback);
    conn->connectionEstablished();

    // connVec.push_back(conn);
    connVec[conn->name()] = conn;
}

int main()
{
    Logger::setLogLevel(Logger::LogLevel::TRACE);

    EventLoop loop;
    g_loop = &loop;

    // InetAddress addr(2233);
    InetAddress addr("127.0.0.1", 2233);
    Acceptor acceptor(&loop, addr, true);
    acceptor.setNewConnectionCallback(newConnection);
    assert(!acceptor.listening());
    acceptor.listen();

    loop.loop();
}