# AMFuture - std::async wrapper for both multithreaded and singlethreaded system

Let's have same source for multithreaded and non-multithreaded system. Enables std::async call on non-multithreaded system.

Look to EasyTest class. This three functions are an interface, that replaces sigle call od function *T function(U...)*.

When defined <b>__EMSCRIPTEN__</b> and not  defined <b>__EMSCRIPTEN_PTHREADS__</b>, **AMAsync** simple calls

    T getData(prepeareData(U...));

otherwise, **AMAsync** is a wrapper for **std::async**.

In emscripten, deleting of std::future causes a crash. As a prevent of it, AMFuture destructor only moves future to backup data structure
and threse futures should be released, before program ends. At the finishing of the program, you should call function **checkZombies()** that
returns true, if program can be completely destroyed.

## Usage

Let's have easy usage: class EasyTest is spawned only once.

    using namespace AMCore;

    int returnValue;

    class EasyTest {
        
    
    public:
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

Parallel use of little more difficult. You need synchronization

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
            int idMem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
            if (idMem == 0) {
                idMem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
            }
            std::lock_guard<std::mutex> lockGuard(g_mutex);
            std::map<int, int>::iterator it = retvals.find(idMem);
            assert(it == retvals.end());
            retvals.insert({idMem, parameter});
            return (void *) (uintptr_t) idMem;
        }
    };


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

    while (!checkZombies()) {
        //do something, what can release futures
        sleep(1);
    }
}

## Documetation

There are doxygen generated documentation [here on libandromeda.org](http://libandromeda.org/amfuture/latest/).

## Building AMFuture

### Getting sources

```bash
git submodule update
```

### Compiling

```bash

mkdir cmake-build-debug

cd cmake-build-debug

cmake ..

make
```

### Single test (not necessary)

```bash
./TEST_AMFuture
```

## License

This library is under GNU GPL v3 license. If you need business license, don't hesitate to contact [me](mailto:zdenek.skulinek\@robotea.com\?subject\=License%20for%20AMFuture).

## Contribute

Please contact [me](mailto:zdenek.skulinek\@robotea.com\?subject\=License%20for%20AMFuture).

## Ask for hepĺp

Please contact [me](mailto:zdenek.skulinek\@robotea.com\?subject\=License%20for%20AMFuture).

## Dependencies

- [Google test](https://github.com/google/googletest.git)
