#include "../include/threadPool.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>


#define THREADINCREMENT 10
#define THREADDECREMENT 10

ThreadPool::ThreadPool(int min, int max){
    m_minThreadNum = min;
    m_maxThreadNum = max;
    m_curThreadNum = min;
    m_busyTHreadNum = 0;

    int res = pthread_mutex_init(&m_mutex, NULL);

    if(res){
        perror("ThreadPool pthread_mutex init failed");
        exit(1);
    }

    res = pthread_cond_init(&m_cond, NULL);
    if(res){
        perror("ThreadPool pthread_cond init failed");
        exit(1);
    }
}
ThreadPool::~ThreadPool(){
    m_bQuit = true;
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

bool ThreadPool::init(){
    //创建管理者线程
    int res = pthread_create(&m_ManagerId, NULL, ThreadManager, this);

    if(res){
        perror("ThreadPool::init pthread_create manager failed");
        return false;
    }

    pthread_t tid;
    //创建消费者线程
    for(int i = 0; i < m_minThreadNum; ++i){
        res = pthread_create(&tid, NULL, ThreadFunc, this);

        if(res){
            perror("ThreadPool::init pthread_create consumer failed");
            return false;
        }
        m_lThreadId.push_back(tid);

    }

    return true;
}
bool ThreadPool::addTask(task t, void *arg){
    auto ta = std::make_pair(t, arg);
    //加锁
    pthread_mutex_lock(&m_mutex);
    m_qTask.push(ta);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    return true;
}

void* ThreadPool::ThreadFunc(void *arg){
    //将线程设置成分离态
    int res = pthread_detach(pthread_self());
    if(res){
        perror("ThreadPool::ThreadFunc pthread detach failed");
        exit(1);
    }
    ThreadPool* This = (ThreadPool*)arg;
    while(!This->m_bQuit){
        //有任务从任务队列取任务，无任务挂起
        pthread_mutex_lock(&This->m_mutex);
        while(This->m_qTask.empty()){
            //上锁
            int res = pthread_cond_wait(&This->m_cond, &This->m_mutex);
            if(res){
                perror("ThreadPool::ThreadFunc pthread cond wait fail");
            }

            //检查退出码
            if(This->m_threadExitCode > 0){
                cout << "thread[" << pthread_self() << "]exit\n";
                --This->m_curThreadNum;
                for(auto it = This->m_lThreadId.begin(); it != This->m_lThreadId.end(); ++it){
                    if(*it == pthread_self()){
                        This->m_lThreadId.erase(it);
                        break;
                    }
                }
                pthread_mutex_unlock(&This->m_mutex);
                pthread_exit(NULL);
            }
        }
        auto t = This->m_qTask.front();
        This->m_qTask.pop();
        ++This->m_busyTHreadNum;
        pthread_mutex_unlock(&This->m_mutex);
        //执行任务
        t.first(t.second);
        pthread_mutex_lock(&This->m_mutex);
        --This->m_busyTHreadNum;
        pthread_mutex_unlock(&This->m_mutex);
    }

    return NULL;
}
void* ThreadPool::ThreadManager(void *arg){
    ThreadPool *This = (ThreadPool*)arg;
    while(!This->m_bQuit){
        pthread_mutex_lock(&This->m_mutex);
        int busy = This->m_busyTHreadNum; 
        int cur = This->m_curThreadNum;
        pthread_mutex_unlock(&This->m_mutex);
        //空闲线程
        int free = This->m_maxThreadNum - busy;
        //如果忙碌线程大于等于当前拥有线程的70%：一次增加十个
        int percent = (float)busy / cur * 100;
        if(percent >= 70 && This->m_curThreadNum < This->m_maxThreadNum){
            pthread_t tid;
            int tmp = min(THREADINCREMENT, This->m_maxThreadNum - cur);
            This->m_curThreadNum += tmp;
            for(int i = 0; i < tmp; ++i){
                int res = pthread_create(&tid, NULL, ThreadFunc, This);

                if(res){
                    perror("ThreadPool::Manager pthread_create consumer failed");
                    return NULL;
                }
                This->m_lThreadId.push_back(tid);
            }
            cout << "ThreadPool::ThreadManager increment " << tmp << "thread\n";
        }
        //如果空闲线程是忙碌线程的两倍：空闲线程缩减十个
        if(free >= busy*2 && This->m_curThreadNum >= This->m_minThreadNum){
            This->m_threadExitCode = min(THREADDECREMENT, This->m_curThreadNum - This->m_minThreadNum); 
            //唤醒退出线程
            for(int i = 0; i < This->m_threadExitCode; ++i){
                pthread_cond_signal(&This->m_cond);
            }
        }
        pthread_mutex_lock(&This->m_mutex);
        cout << "total:" << This->m_maxThreadNum << "   busy: " << This->m_busyTHreadNum << "   cur: " << This->m_curThreadNum << endl;
        pthread_mutex_unlock(&This->m_mutex);
        sleep(1);

    }

    return NULL;

}
