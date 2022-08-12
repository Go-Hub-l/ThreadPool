#ifndef __MYTHREADPOOL__
#define __MYTHREADPOOL__

#include <pthread.h>
#include <functional>
#include <memory>
#include <queue>
#include <list>

using namespace std;

class ThreadPool{
public:
    typedef std::shared_ptr<ThreadPool> ptr;
    typedef std::function<void(void*)> task;

    //线程池构造函数
    ThreadPool() = default;
    ThreadPool(int min, int max);
    ~ThreadPool();

    bool init();
    bool addTask(task t, void*);

    static void* ThreadFunc(void *);

    static void* ThreadManager(void *);


private:
    bool m_bQuit = false;
    int m_minThreadNum = 8;//如何设计
    int m_maxThreadNum = 100;//如何设计
    int m_curThreadNum = 8;
    int m_busyTHreadNum = 0;
    int m_threadExitCode = 0;
    pthread_t m_ManagerId;
    std::list<pthread_t> m_lThreadId;
    std::queue<pair<task, void*> > m_qTask;

    //线程安全
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif
