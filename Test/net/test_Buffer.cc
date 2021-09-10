#include "../../net/Buffer.h"

#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;


using namespace Misaka;
using namespace net;

void bench()
{
    Buffer buf;

    int fd = open("xxx.jpg", O_RDONLY);
    assert(0 <= fd);

    struct stat fileStat;
    fstat(fd, &fileStat);
    char* file_mmap_addr = static_cast<char*>(mmap(nullptr, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    cout << "file.size() -> " << fileStat.st_size << endl;

    string body;
    body.assign(file_mmap_addr, fileStat.st_size);
    cout << "body.size() -> " << body.size() << endl;
    buf.append(body);
    cout << "buf.readableBytes() -> " << buf.readableBytes() << endl;

    int newFd = open("newXXX.jpg", O_WRONLY | O_CREAT);
    assert(0 <= newFd);

    write(newFd, buf.peek(), buf.readableBytes());
    buf.retrieveAll();

    close(fd);
    close(newFd);

    // buf.append(StringPiece("bilibili 2233"));
    // cout << "buf.readableBytes() -> " << buf.readableBytes() << endl;
    // cout << "buf.writableBytes() -> " << buf.writableBytes() << endl;
    // cout << "buf.prependableBytes() -> " << buf.prependableBytes() << endl;
    // cout << buf.peek() << endl;

    // buf.append(StringPiece("xxx\nxxx"));
    // cout << "buf.readableBytes() -> " << buf.readableBytes() << endl;
    // cout << "buf.writableBytes() -> " << buf.writableBytes() << endl;
    // cout << "buf.prependableBytes() -> " << buf.prependableBytes() << endl;
    // cout << buf.peek() << endl;

    // buf.append(StringPiece("HHH\r\nHHH"));
    // cout << buf.peek() << endl;

    // cout << buf.toStringPiece().as_string() << endl;

    // const char* ptrCRLF = buf.findCRLF();
    // const char* ptrEOL = buf.findEOL();
    // cout << ptrCRLF << endl;
    // cout << ptrEOL << endl;
}

int main()
{
    bench();
}