/**
 * @mainpage Andromeda range library
 * @brief AMFuture - std::async wrapper for both multithreaded and singlethreaded system
 *
 * Let's have same source for multithreaded and non-multithreaded system. Enables std::async call on non-multithreaded system.
 *
 * Look to EasyTest class. This three functions are an interface, that replaces sigle call od function *T function(U...)*.
 *
 * When defined <b>__EMSCRIPTEN__</b> and not  defined <b>__EMSCRIPTEN_PTHREADS__</b>, **AMAsync** simple calls
 *
 *
 * \code
 * T getData(prepeareData(U...));
 * \endcode
 *
 * otherwise, **AMAsync** is a wrapper for **std::async**.
 *
 * In emscripten, deleting of std::future causes a crash. As a prevent of it, AMFuture destructor only moves future to backup data structure
 * and threse futures should be released, before program ends. At the finishing of the program, you should call function **checkZombies()** that
 * returns true, if program can be completely destroyed.
 *
 * Usage
 * =====
 *
 * Let's have easy usage: class EasyTest is spawned only once.
 *
 * \code
 *
 *    using namespace AMCore;
 *
 *    int returnValue;
 *
 *    class EasyTest {
 *
 *
 *    public:
 *        int getData(void *mem)
 *        {
 *            return returnValue;
 *        }
 *
 *        bool isDataAvail(void *mem)
 *        {
 *            return true;
 *        }
 *
 *        void *prepareData(int parameter)
 *        {
 *            returnValue = parameter;
 *            return nullptr;
 *        }
 *    };
 *
 *    int parameter = 11;
 *    EasyTest e;
 *    AMFuture<int> future = AMAsync(
 *        AMLaunch::async,
 *        &EasyTest::getData,
 *        &EasyTest::isDataAvail,
 *        &EasyTest::prepareData,
 *        e,
 *        parameter
 *    );
 *
 *    int res = future.get(); //returns 11
 *
 * \endcode
 *
 * Parallel ose of little more difficult. You need synchronization
 *
 * \code
 *
 *    std::atomic<int> atomicIdMem(1);
 *    std::map<int, int> retvals;
 *    std::mutex g_mutex;
 *
 *    class ParallelTest {
 *    public:
 *        int getData(void *mem)
 *        {
 *            std::lock_guard<std::mutex> lockGuard(g_mutex);
 *            int id = (char *) mem - (char *) nullptr;
 *            std::map<int, int>::iterator it = retvals.find(id);
 *            assert(it != retvals.end());
 *            int rv = it->second;
 *            retvals.erase(it);
 *            return rv;
 *        }
 *
 *        bool isDataAvail(void *mem)
 *        {
 *            std::lock_guard<std::mutex> lockGuard(g_mutex);
 *            int id = (char *) mem - (char *) nullptr;
 *            std::map<int, int>::iterator it = retvals.find(id);
 *            return it != retvals.end();
 *        }
 *
 *        void *prepareData(int parameter)
 *        {
 *            int idMem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
 *            if (idMem == 0) {
 *                idMem = atomicIdMem.fetch_add(1, std::memory_order_relaxed);
 *            }
 *            std::lock_guard<std::mutex> lockGuard(g_mutex);
 *            std::map<int, int>::iterator it = retvals.find(idMem);
 *            assert(it == retvals.end());
 *            retvals.insert({idMem, parameter});
 *            return (void *) (uintptr_t) idMem;
 *        }
 *    };
 *
 *    int parameter1 = 9;
 *    int parameter2 = 5;
 *    ParallelTest p;
 *    AMFuture<int> future1 = AMAsync(
 *    AMLaunch::async,
 *        &ParallelTest::getData,
 *        &ParallelTest::isDataAvail,
 *        &ParallelTest::prepareData,
 *        p,
 *        parameter1
 *    );
 *    AMFuture<int> future2 = AMAsync(
 *    AMLaunch::async,
 *        &ParallelTest::getData,
 *        &ParallelTest::isDataAvail,
 *        &ParallelTest::prepareData,
 *        p,
 *        parameter2
 *    );
 *
 *    int res1 = future1.get(); //returns 9
 *    int res2 = future2.get(); //returns 5
 *
 *    while (!checkZombies()) {
 *        //do something, what can release futures
 *        sleep(1);
 *    }
 * }
 *
 * \endcode
 *
 *  For more, see \ref AMFuture.h
 *
 * Sources
 * =======
 *
 * Download at [GitHUB](https://github.com/robotea/amfuture)
 *
 * Building AMFuture
 * ==================
 *
 * Getting sources
 * ---------------
 *
 * \code
 * git submodule update
 * \endcode
 *
 * Compiling
 * ---------
 *
 * \code
 * mkdir cmake-build-debug
 *
 * cd cmake-build-debug
 *
 * cmake ..
 *
 * make
 * \endcode
 *
 * Output Library
 * --------------
 *
 * \code
 * /lib/libAMFuture.so
 * \endcode
 *
 * Single test (not necessary)
 * ---------------------------
 *
 * \code
 * ./TEST_AMFuture
 * \endcode
 *
 * License
 * =======
 *
 * This library is under GNU GPL v3 license. If you need business license, don't hesitate to contact [me](mailto:zdenek.skulinek@robotea.com?subject=License for AMFuture).
 *
 * Contribute
 * ==========
 *
 * Please contact [me](mailto:zdenek.skulinek@robotea.com?subject=License for AMFuture).
 *
 * Dependencies
 * ============
 *
 * - [Google test](https://github.com/google/googletest.git)
 */
