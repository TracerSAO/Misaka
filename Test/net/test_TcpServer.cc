#include "../../net/TcpConnection.h"
#include "../../net/Acceptor.h"
#include "../../net/InetAddress.h"
#include "../../net/SocketsOps.h"
#include "../../net/Socket.h"
#include "../../net/TcpServer.h"
#include "../../net/EventLoop.h"
#include "../../base/Timestamp.h"
#include "../../base/Logging.h"

#include <cstdio>

using namespace Misaka;
using namespace net;


void connectionCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO << "TcpConnection::ConnectionCallback() - <(￣ c￣)y▂ξ";
}

void messageCallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    LOG_INFO << "TcpConnection::MessageCallback() - (╬▔皿▔)╯ "
             << receiveTime.toFormattedString()
             << buf->toStringPiece();
    buf->retrieveAll();
    conn->send(StringPiece("┗|｀O′|┛ 嗷~~"));
}

void writeCompleteCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO << "TcpConnection::WriteCompletetCallback() - ♪(´▽｀)";
}

int main()
{
    EventLoop loop;
    
    const InetAddress listenaddr("127.0.0.1", 2233);
    TcpServer echoServer(&loop, "echo-Server", InetAddress(2233), TcpServer::Option::kReuseport);
    echoServer.setConnectionCallback(connectionCallback);
    echoServer.setMessageCallback(messageCallback);
    echoServer.setWriteCompleteCallback(writeCompleteCallback);

    // echoServer.setThreadNum(3);
    echoServer.setThreadNum(0);
    echoServer.start();
    loop.loop();
}
