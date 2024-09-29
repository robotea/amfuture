#define __EMSCRIPTEN__

#include "../../AMFuture.h"
#include "gtest/gtest.h"
#include <set>



using namespace AMCore;

class EasyTest {

public:
    int returnValue;

    int getData(void *mem)
    {
        return returnValue;
    }

    bool isDataAvail(void *mem)
    {
        return true;
    }

    void *prepareData(int parameter)
    {
        returnValue = parameter;
        return nullptr;
    }
};

TEST(AMFuture, easyTest)
{
    int parameter = 11;
    EasyTest e;
    AMFuture<int> future = AMAsync(
        AMLaunch::async,
        &EasyTest::getData,
        &EasyTest::isDataAvail,
        &EasyTest::prepareData,
        e,
        parameter
        );

    int res = future.get(); //returns 11

    EXPECT_EQ(res, 11);

}

std::atomic<int> atomicIdMem(1);
std::map<int, int> retvals;
std::mutex g_mutex;

class ParallelTest {
public:
    int getData(void *mem)
    {
        std::lock_guard<std::mutex> lockGuard(g_mutex);
        int id = (char *) mem - (char *) nullptr;
        std::map<int, int>::iterator it = retvals.find(id);
        assert(it != retvals.end());
        int rv = it->second;
        retvals.erase(it);
        return rv;
    }

    bool isDataAvail(void *mem)
    {
        std::lock_guard<std::mutex> lockGuard(g_mutex);
        int id = (char *) mem - (char *) nullptr;
        std::map<int, int>::iterator it = retvals.find(id);
        return it != retvals.end();
    }

    void *prepareData(int parameter)
    {
        int id_mem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
        if (id_mem == 0) {
            id_mem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
        }
        std::lock_guard<std::mutex> lockGuard(g_mutex);
        std::map<int, int>::iterator it = retvals.find(id_mem);
        assert(it == retvals.end());
        retvals.insert({id_mem, parameter});
        return (void *) (uintptr_t) id_mem;
    }
};

TEST(AMFuture, parallelTest)
{
    int parameter1 = 9;
    int parameter2 = 5;
    ParallelTest p;
    AMFuture<int> future1 = AMAsync(
            AMLaunch::async,
            &ParallelTest::getData,
            &ParallelTest::isDataAvail,
            &ParallelTest::prepareData,
            p,
            parameter1
            );
    AMFuture<int> future2 = AMAsync(
        AMLaunch::async,
        &ParallelTest::getData,
        &ParallelTest::isDataAvail,
        &ParallelTest::prepareData,
        p,
        parameter2
        );

    int res1 = future1.get(); //returns 9
    int res2 = future2.get(); //returns 5

    EXPECT_EQ(res1, 9);
    EXPECT_EQ(res2, 5);


    while (!checkZombies()) {
        //do something, what can relase futures
        sleep(1);
    }
}

int main(int argc, char **argv) {

     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
}
