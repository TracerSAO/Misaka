#include "../../base/Logging.h"
#include "../../base/LogFile.h"
#include "../../base/AsyncLogging.h"

#include <cstdio>
#include <unistd.h>

int g_total;
FILE* g_file;
std::unique_ptr<Misaka::LogFile> g_logFile;

void dummyOutput(const char* msg, int len)
{
	g_total += len;
	if (g_file)
	{
		fwrite(msg, 1, len, g_file);
	}
	else if (g_logFile)
	{
		g_logFile->append(msg, len);
	}
}

const int kRollSize = 5000 * 1000;

Misaka::AsyncLogging* g_asynclog = nullptr;

void asyncOutput(const char* logline, int len)
{
  	g_asynclog->append(logline, len);
}

void bench(const char* type)
{
	if (Misaka::string(type, strlen(type)) == Misaka::string("AsyncLogging"))
		Misaka::Logger::setOutput(asyncOutput);
	else
		Misaka::Logger::setOutput(dummyOutput);

	Misaka::Timestamp start(Misaka::Timestamp::now());
	g_total = 0;

	int n = 1000 * 1000;
	const bool kLongLog = false;
	Misaka::string empty = " ";
	Misaka::string longStr(3000, 'X');
	longStr += " ";
	for (int i = 0; i < n; ++i)
	{
		LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
				<< (kLongLog ? longStr : empty)
				<< i;
	}
	Misaka::Timestamp end(Misaka::Timestamp::now());
	double seconds = timeDifference(end, start);
	printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
			type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

int main(int argc, char* argv[])
{
	getppid(); // for ltrace and strace

	Misaka::Logger::setLogLevel(Misaka::Logger::LogLevel::INFO);
	
	Misaka::AsyncLogging async(::basename(argv[0]), kRollSize);
	g_asynclog = &async;
	async.start();
	bench("AsyncLogging");

	g_file = nullptr;
	g_logFile.reset(new Misaka::LogFile("test_log_st", 500*1000*1000, false));
	bench("test_log_st");

	// g_logFile.reset(new Misaka::LogFile("test_log_mt", 500*1000*1000, true));
	// bench("test_log_mt");

	g_logFile.reset();
}


//   LOG_TRACE << "trace";
//   LOG_DEBUG << "debug";
//   LOG_INFO << "Hello";
//   LOG_WARN << "World";
//   LOG_ERROR << "Error";
//   LOG_INFO << sizeof(Misaka::Logger);
//   LOG_INFO << sizeof(Misaka::LogStream);
//   LOG_INFO << sizeof(Misaka::Fmt);
//   LOG_INFO << sizeof(Misaka::LogStream::Buffer);



// #include "../Logging.h"
// #include "../Thread.h"

// using namespace Misaka;

// void bench()
// {
//     // Logger::setLogLevel(Logger::LogLevel::INFO);
//     Logger::setLogLevel(Logger::LogLevel::TRACE);

//     LOG_TRACE << "Bilibili 2233" << " hello world" << " " << 2233;
//     LOG_DEBUG << "Bilibili 2233" << " hello world" << " " << 2233;
//     LOG_INFO << "Bilibili 2233" << " hello world" << " " << 2233;
//     LOG_ERROR << "Bilibili 2233" << " hello world" << " " << 2233;
//     // LOG_FATA << "Bilibili 2233" << " hello world" << " " << 2233;
//     LOG_SYSERR << "Bilibili 2233" << " hello world" << " " << 2233;
//     LOG_SYSFATAL << "Bilibili 2233" << " hello world" << " " << 2233;
// }


// int main()
// {
//     bench();
// }