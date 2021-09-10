#ifndef MISAKA_LOCKER_HPP
#define MISAKA_LOCKER_HPP

#include "CurrentThread.h"
#include "noncopyable.h"

#include <pthread.h>

#include <cassert>

namespace Misaka
{

class MutexLock : noncopyable
{
public:
    MutexLock() :
        mutex_(PTHREAD_MUTEX_INITIALIZER),
        holder_(0)
    {
    }
    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex_);
    }

    bool isLockByThisThread()
    {
        return holder_ == CurrentThread::tid();
    }

    void assertLockInThread()
    {
        assert(isLockByThisThread());
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
        assignedHolder();
    }

    void unlock()
    {
        unassignedHolder();
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getMutexPointer()
    {
        return &mutex_;
    }

private:
    void assignedHolder()
    {
        holder_ = CurrentThread::tid();
    }

    void unassignedHolder()
    {
        holder_ = 0;
    }

private:
    class UnassignedGuard {
    public:
        UnassignedGuard(MutexLock& mutex) :
            mutex_(mutex)
        {
            mutex_.unassignedHolder();
        }
        ~UnassignedGuard()
        {
            mutex_.assignedHolder();
        }
    private:
        MutexLock& mutex_;
    };

private:
    pthread_mutex_t mutex_;
    pid_t holder_;

    friend class Condition;
};

class MutexLockGuard : noncopyable
{
public:
    MutexLockGuard(MutexLock& mutex) :
        mutex_(mutex)
    {
        mutex_.lock();
    }
    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

#define MutexLockGuard(x) static_assert(false, "Missing guard object name");

namespace GARBAGE
{
    //class Sem
    //{
    //public:
    //    Sem() {
    //        if (sem_init(&_sem, 0, 0))
    //        {
    //            throw std::exception();
    //        }
    //    }
    //    ~Sem() {
    //        sem_destroy(&_sem);
    //    }

    //    bool wait() {
    //        return sem_wait(&_sem) == 0;
    //    }

    //    bool post() {
    //        return sem_post(&_sem) == 0;
    //    }

    //private:
    //    sem_t _sem;
    //};

    //class Locker
    //{
    //public:
    //    Locker() {
    //        if (pthread_mutex_init(&_mutex, nullptr) != 0)
    //        {
    //            throw std::exception();
    //        }
    //    }
    //    ~Locker() {
    //        pthread_mutex_destroy(&_mutex);
    //    }

    //    bool lock() {
    //        return pthread_mutex_lock(&_mutex) == 0;
    //    }

    //    bool unlock() {
    //        return pthread_mutex_unlock(&_mutex) == 0;
    //    }

    //private:
    //    pthread_mutex_t _mutex;
    //};

    //class Cond
    //{
    //public:
    //    Cond() {

    //        if (pthread_mutex_init(&_mutex, nullptr) != 0)
    //        {
    //            throw std::exception();
    //        }

    //        if (pthread_cond_init(&_cond, nullptr) != 0)
    //        {
    //            pthread_mutex_destroy(&_mutex);
    //            throw std::exception();
    //        }
    //    }
    //    ~Cond() {

    //        pthread_mutex_destroy(&_mutex);
    //        pthread_cond_destroy(&_cond);
    //    }

    //    bool wait() {

    //        int res = 0;
    //        pthread_mutex_lock(&_mutex);
    //        res = pthread_cond_wait(&_cond, &_mutex);
    //        pthread_mutex_unlock(&_mutex);
    //        return 0 == res;
    //    }

    //    bool signal() {
    //        return pthread_cond_signal(&_cond) == 0;
    //    }

    //private:
    //    pthread_mutex_t _mutex;
    //    pthread_cond_t _cond;
    //};
}

}   // namespace Misaka

#endif