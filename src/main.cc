#include "../include/threadPool.h"
#include <iostream>
#include <unistd.h>

struct ad{
    int a;
    int b;
};

void add(void* arg){
    ad* t = (ad*)arg;
    //std::cout << "result: "<< " a = " << t->a << " b = " << t->b << "     a + b = " << t->a + t->b << std::endl;
    sleep(1);
}

int main(){
    ThreadPool pool(8, 100);
    pool.init();

    for(int i = 0; i < 1000; ++i){
        ad* a = new ad;
        a->a = i;
        a->b = i + 10;
        pool.addTask(add, (void*)a);
    }


    while(1);

    return 0;
}
