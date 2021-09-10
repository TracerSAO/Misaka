#include "../../net/InetAddress.h"
#include "../../net/SocketsOps.h"
#include "../../net/EndianT.h"
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>

using namespace std;
using namespace Misaka;
using namespace net;

int main()
{
    struct sockaddr_in addr_in;
    bzero(&addr_in, sizeof addr_in);
    struct sockaddr_in6 addr_in6;
    bzero(&addr_in6, sizeof addr_in6);
    
    {
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = sockets::hostToNetwork16(2233);
        ::inet_pton(AF_INET, "127.0.0.1", &addr_in.sin_addr);
    }
    {
        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_port = sockets::hostToNetwork16(2233);
        ::inet_pton(AF_INET6, "127.0.0.1", &addr_in6.sin6_addr);
    }

    InetAddress addr1(2233, true, false);
    InetAddress addr2("192.168.1.233", 2233, false);
    InetAddress addr3(addr_in);
    InetAddress addr4(addr_in6);

    {
        auto& temp = addr1;
        cout << "addr1:\n";
        
        cout << "family(): ";
        if (temp.family() == AF_INET)
            cout << "==AF_INET\n";
        else cout << "!=AF_INET\n";

        cout << "toIp(): ";
        cout << temp.toIp() << endl;

        cout << "toIpPort(): ";
        cout << temp.toIpPort() << endl;

        cout << "port(): ";
        cout << temp.port() << endl;
    }
    {
        auto& temp = addr2;
        cout << "addr2:\n";
        
        cout << "family(): ";
        if (temp.family() == AF_INET)
            cout << "==AF_INET\n";
        else cout << "!=AF_INET\n";

        cout << "toIp(): ";
        cout << temp.toIp() << endl;

        cout << "toIpPort(): ";
        cout << temp.toIpPort() << endl;

        cout << "port(): ";
        cout << temp.port() << endl;
    }
    {
        auto& temp = addr3;
        cout << "addr3:\n";
        
        cout << "family(): ";
        if (temp.family() == AF_INET)
            cout << "==AF_INET\n";
        else cout << "!=AF_INET\n";

        cout << "toIp(): ";
        cout << temp.toIp() << endl;

        cout << "toIpPort(): ";
        cout << temp.toIpPort() << endl;

        cout << "port(): ";
        cout << temp.port() << endl;
    }
    {
        auto& temp = addr4;
        cout << "addr4:\n";
        
        cout << "family(): ";
        if (temp.family() == AF_INET)
            cout << "==AF_INET\n";
        else cout << "!=AF_INET\n";

        cout << "toIp(): ";
        cout << temp.toIp() << endl;

        cout << "toIpPort(): ";
        cout << temp.toIpPort() << endl;

        cout << "port(): ";
        cout << temp.port() << endl;
    }

}