#include "../../net/Acceptor.h"
#include "../../net/InetAddress.h"
#include "../../net/Socket.h"
#include "../../net/SocketsOps.h"
#include "../../net/EventLoop.h"

using namespace Misaka;
using namespace net;

void newConnectionCallback(int sockfd, const InetAddress& addr)
{
    Socket connSock(sockfd);
    char buf[1024 * 8];
    connSock.getTcpInfoString(buf, sizeof buf);
    printf("sockfd: %d\n[%s]\n", connSock.fd(), buf);
    const char* str = "bilibili~ 2233";
    ::write(sockfd, str, strlen(str));
}

int main()
{
    InetAddress addr("127.0.0.1", 2233);

    EventLoop loop;
    Acceptor acceptorSocket(&loop, addr, true);

    // Acceptor::NewConnectionCallback func(newConnectionCallback);
    acceptorSocket.setNewConnectionCallback(
        std::bind(&newConnectionCallback,
                std::placeholders::_1,
                std::placeholders::_2));
    if (!acceptorSocket.listening())
        acceptorSocket.listen();
    loop.loop();
}