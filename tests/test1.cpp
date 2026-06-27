#include <iostream>
#include "../include/MemoryPool.h"
#include <chrono>

#define ELEMS 1000000
#define REPS  50

class P1
{
    int id_;
};

class P2
{
    int id_[5];
};

class P3
{
    int id_[10];
};

class P4
{
    int id_[20];
};

// 计时器
class Timer
{
private:
    std::chrono::high_resolution_clock::time_point start;
public:
    Timer()
        : start(std::chrono::high_resolution_clock::now())
    {}
    double elapsed()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        return duration.count() * 1000;
    }
};

int main()
{
    // 内存池:
    MemoryPool<P1> mem1;
    MemoryPool<P2> mem2;
    MemoryPool<P3> mem3;
    MemoryPool<P4> mem4;
    Timer timer1;
    for (int i = 0; i < REPS; ++i)
    {
        for (int j = 0; j < ELEMS; ++j)
        {
            P1* p1 = mem1.newElement();
            mem1.deleteElement(p1);
            P2* p2 = mem2.newElement();
            mem2.deleteElement(p2);
            P3* p3 = mem3.newElement();
            mem3.deleteElement(p3);
            P4* p4 = mem4.newElement();
            mem4.deleteElement(p4);
        }
    }
    std::cout << "newElement/deleteElement: " << timer1.elapsed() << "ms" << std::endl;
    
    // 默认:
    Timer timer2;
    for (int i = 0; i < REPS; ++i)
    {
        for (int j = 0; j < ELEMS; ++j)
        {
            P1* p1 = new P1;
            delete p1;
            P2* p2 = new P2;
            delete p2;
            P3* p3 = new P3;
            delete p3;
            P4* p4 = new P4;
            delete p4;
        }
    }
    std::cout << "new/delete: " << "              " << timer2.elapsed() << "ms" << std::endl;

    return 0;
}