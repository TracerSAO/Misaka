#include "../../base/AsyncLogging.h"
#include "../../base/Logging.h"
#include "../../base/LogFile.h"
#include "../../base/FileUtil.h"
#include "../../base/Timestamp.h"
#include "../../base/Thread.h"

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

  off_t kRollSize = 500*1000;

Misaka::AsyncLogging* g_asyncLog = NULL;

void asyncOutput(const char* msg, int len)
{
  g_asyncLog->append(msg, len);
}


void bench(bool longLog)
{
  // Misaka::Logger::setOutput(asyncOutput);
  // Misaka::Logger::setLogLevel(Misaka::Logger::LogLevel::ERROR);

  int cnt = 0;
  const int kBatch = 1000;
  Misaka::string empty = " ";
  Misaka::string longStr(3000, 'X');
  longStr += " ";

  for (int t = 0; t < 30; ++t)
  {
    Misaka::Timestamp start = Misaka::Timestamp::now();
    for (int i = 0; i < kBatch; ++i)
    {
      LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
               << (longLog ? longStr : empty)
               << cnt;
      ++cnt;
    }
    Misaka::Timestamp end = Misaka::Timestamp::now();
    printf("%f\n", Misaka::timeDifference(end, start)*1000000/kBatch);
    struct timespec ts = { 0, 500*1000*1000 };
    nanosleep(&ts, NULL);
  }
}

void threadFunc()
{
  bench(false);
}

int main(int argc, char* argv[])
{
  {
    // set max virtual memory to 2GB.
    size_t kOneGB = 1000*1024*1024;
    rlimit rl = { 2*kOneGB, 2*kOneGB };
    setrlimit(RLIMIT_AS, &rl);
  }

  printf("pid = %d\n", getpid());

  char name[256] = { '\0' };
  strncpy(name, argv[0], sizeof name - 1);
  Misaka::AsyncLogging log(::basename(name), kRollSize);
  log.start();
  g_asyncLog = &log;

  // bool longLog = argc > 1;
  // bench(longLog);

  Misaka::Logger::setOutput(asyncOutput);

  Misaka::Thread thread1(&threadFunc, "Thread1");
  Misaka::Thread thread2(&threadFunc, "Thread2");
  Misaka::Thread thread3(&threadFunc, "Thread3");
  Misaka::Thread thread4(&threadFunc, "Thread4");

  thread1.start();
  thread2.start();
  thread3.start();
  thread4.start();

  thread1.join();
  thread2.join();
  thread3.join();
  thread4.join();

  // sleep(20);
}


namespace TEMP2
{
//   int g_total;
// FILE* g_file;
// std::unique_ptr<Misaka::LogFile> g_logFile;
// void dummyOutput(const char* msg, int len)
// {
//   g_total += len;
//   if (g_file)
//   {
//     fwrite(msg, 1, len, g_file);
//   }
//   else if (g_logFile)
//   {
//     g_logFile->append(msg, len);
//   }
// }

// const int kRollSize = 5000 * 1000;
// Misaka::AsyncLogging* g_asynclog = nullptr;
// void asyncOutput(const char* logline, int len)
// {
//   g_asynclog->append(logline, len);
// }

// void bench(const char* type)
// {
//   if (Misaka::string(type, strlen(type)) == Misaka::string("AsyncLogging"))
//     Misaka::Logger::setOutput(asyncOutput);
//   else
//     Misaka::Logger::setOutput(dummyOutput);

//   Misaka::Timestamp start(Misaka::Timestamp::now());
//   g_total = 0;

//   int n = 100 * 1000;
//   const bool kLongLog = false;
//   Misaka::string empty = " ";
//   Misaka::string longStr(3000, 'X');
//   longStr += " ";
//   for (int i = 0; i < n; ++i)
//   {
//     LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
//              << (kLongLog ? longStr : empty)
//              << i;
//   }
//   Misaka::Timestamp end(Misaka::Timestamp::now());
//   double seconds = timeDifference(end, start);
//   printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
//          type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
// }


// int main(int argc, char* argv[])
// {
//   {
//     // set max virtual memory to 2GB.
//     size_t kOneGB = 1000*1024*1024;
//     rlimit rl = { 2*kOneGB, 2*kOneGB };
//     setrlimit(RLIMIT_AS, &rl);
//   }

//   Misaka::Logger::setLogLevel(Misaka::Logger::LogLevel::INFO);
  
//   Misaka::AsyncLogging async(::basename(argv[0]), kRollSize);
//   g_asynclog = &async;
//   async.start();

//   bench("AsyncLogging");

// //   LOG_TRACE << "trace";
// //   LOG_DEBUG << "debug";
// //   LOG_INFO << "Hello";
// //   LOG_WARN << "World";
// //   LOG_ERROR << "Error";
// //   LOG_INFO << sizeof(Misaka::Logger);
// //   LOG_INFO << sizeof(Misaka::LogStream);
// //   LOG_INFO << sizeof(Misaka::Fmt);
// //   LOG_INFO << sizeof(Misaka::LogStream::Buffer);


//     g_file = nullptr;
//   g_logFile.reset(new Misaka::LogFile("test_log_st", kRollSize, false));
//   bench("test_log_st");

//   // g_logFile.reset(new Misaka::LogFile("test_log_mt", 500*1000*1000, true));
//   // bench("test_log_mt");

//   g_logFile.reset();
// }
}

namespace TEMP
{
// Misaka::AsyncLogging* g_asyncLogger = nullptr;

// void asyncLogFunc(const char* logline, int len)
// {
//   g_asyncLogger->append(logline, len);
// }

// void threadFunc()
// {
//   int n = 10000;
//   for (int i = 0; i < n; i++)
//   {
//     LOG_INFO << "qwertyuiopasdfghjkzxcvbnm,." << 2233 << ' ' << i;
//   }
// }

// void bench()
// {
//   Misaka::Logger::setOutput(&asyncLogFunc);

//   Misaka::Thread thread1(&threadFunc, "Thread1");
//   Misaka::Thread thread2(&threadFunc, "Thread2");
//   Misaka::Thread thread3(&threadFunc, "Thread3");
//   Misaka::Thread thread4(&threadFunc, "Thread4");

//   thread1.start();
//   thread2.start();
//   thread3.start();
//   thread4.start();

//   // thread1.join();
//   // thread2.join();
//   // thread3.join();
//   // thread4.join();

//   sleep(2);
//   // 不使用 Pool 的控制多个线程，仍存在严重的问题：
//   // 多个线程，如何确保全部结束
// }

// int main()
// {
//   Misaka::AsyncLogging async(::basename("main"), 5000);
//   g_asyncLogger = &async;
//   async.start();

//   bench();
// }
}