#include "./http/HttpServer.h"
#include "./http/HttpRequest.h"
#include "./http/HttpResponse.h"
#include "./base/AsyncLogging.h"
#include "./base/Logging.h"
#include "./net/EventLoop.h"

#include <iostream>
#include <map>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

using namespace Misaka;
using namespace net;
using namespace net::http;

AsyncLogging* g_async = nullptr;
string arknights;

class GetDirPath
{
public:
	GetDirPath()
	{
		char DIRPATH[4096];
		assert(getcwd(DIRPATH, sizeof DIRPATH));
		arknights.assign(DIRPATH, strlen(DIRPATH));
	}
};
GetDirPath theOne;

void onRequest(const HttpRequest& req, HttpResponse* resp);

void logOutput(const char* msg, int len)
{
  	g_async->append(msg, len);
}


int main(int argc, char* argv[])
{
	Logger::setLogLevel(Logger::LogLevel::INFO);
	Logger::setOutput(logOutput);

	char name[256] = "\0";
	strncpy(name, argv[0], sizeof name - 1);
	AsyncLogging asyncLog(::basename(name), 64*1024*16);
	g_async = &asyncLog;
	asyncLog.start();

	int numThreads = 0;
	if (argc > 1)
	{
		numThreads = atoi(argv[1]);
	}

	EventLoop loop;
	HttpServer server(&loop, "Arknights", InetAddress(2233));
	server.setHttpCallback(onRequest);
	server.setThreadNum(numThreads);
	server.start();
	loop.loop();
}


HttpResponse::HttpStatusCode checkFile(const char* path, struct stat* fileStat)
{
	using httpCode = HttpResponse::HttpStatusCode;

	if (0 > stat(path, fileStat))
		return httpCode::k404NotFound;
	if ( !(fileStat->st_mode & S_IROTH) )
		return httpCode::k403Forbidden;
	if (S_ISDIR(fileStat->st_mode))
		return httpCode::k400BadRequest;
	return httpCode::k200OK;
}

void onRequest(const HttpRequest& req, HttpResponse* resp)
{
	// Get file path
	string filePath = arknights + req.path();
	LOG_DEBUG << "filePath: " << filePath;

	// check file
	struct stat fileStat;
	auto code = checkFile(filePath.c_str(), &fileStat);
	resp->setStatusCode(code);

	// get file content
	if (HttpResponse::HttpStatusCode::k200OK == code)
	{
		int fd = open(filePath.c_str(), O_RDONLY);
		if (0 > fd)
		{
		LOG_ERROR << " path: " << filePath.c_str() << " error: " << strerror_tl(errno);
		}
		assert(0 <= fd);
		char* bufAddr = static_cast<char*>(mmap(nullptr,
												fileStat.st_size,
												PROT_READ,
												MAP_PRIVATE,
												fd,
												0));
		assert(nullptr != bufAddr);
		resp->setBody(bufAddr, fileStat.st_size);
		close(fd);
		munmap(static_cast<void*>(bufAddr), fileStat.st_size);
	}
	else
	{
		resp->setCloseConnection(true);
		// 也可以试着增加一些 “失败宣言”，不过这是由 usr 提供，而非 Misaka 网络库的职责！
		// 具体实现方式为：增加一个 function OR 设定一组 <HttpResponseStatuCode:失败宣言> 的映射
	}

	resp->addHeader("Server", "Arknights");
}