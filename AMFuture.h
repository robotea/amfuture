/**
 * @file: AMFuture.h
 * std::async wrapper for both multithreaded and singlethreaded system
 *
 * @author Zdeněk Skulínek  &lt;<a href="mailto:zdenek.skulinek@seznam.cz">me@zdenekskulinek.cz</a>&gt;
 */


#ifndef SAW_ALL_AMFUTURE_H
#define SAW_ALL_AMFUTURE_H

#include <functional>
#include <chrono>
#include <type_traits>
#include <cassert>

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)

namespace AMCore {

    enum class AMFutureStatus
    {
        ready,
        timeout
    };

    enum class AMLaunch
    {
        async =  0
    };

    template<class T>
    class AMFuture;
    template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
    AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
    AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args);

    template<class Result>
    class _AMLaunchFnHolderBase
    {
    public:
        _AMLaunchFnHolderBase(Result (*_sget)(void*, void*), bool (*_savail)(void*, void*), void* _holder)
            :sget(_sget),
            savail(_savail),
            holder(_holder)
        {
        }
        Result SGet(void* mem)
        {
            return sget(holder, mem);
        }
        bool SIsAvail(void* mem)
        {
            return savail(holder, mem);
        }
        virtual ~_AMLaunchFnHolderBase()
        {

        }
    protected:
        Result (*sget)(void* holder, void* mem);
        bool (*savail)(void* holder, void* mem);
        void* holder;
    };

    template<class AvailCallback, class Callback, class TObject>
    class _AMLaunchFnHolder: public _AMLaunchFnHolderBase<std::invoke_result_t<std::decay_t<Callback>, TObject, void*>> {
    public:
        _AMLaunchFnHolder(TObject& _obj, AvailCallback&& _ac, Callback&& _c)
            : _AMLaunchFnHolderBase<std::invoke_result_t<std::decay_t<Callback>, TObject, void*>>(_AMLaunchFnHolder::SGetS, _AMLaunchFnHolder::SIsAvailS,(void*)this), obj(_obj), ac(std::move(_ac)), c(std::move(_c))
        {
        }
        ~_AMLaunchFnHolder()
        {
        }
    protected:
        static bool SIsAvailS(void* _holder, void* mem)
        {
            _AMLaunchFnHolder* holder = (_AMLaunchFnHolder*)_holder;
            return std::invoke(holder->ac, holder->obj, mem);
        }
        static std::invoke_result_t<std::decay_t<Callback>, TObject, void*> SGetS(void* _holder, void* mem)
        {
            _AMLaunchFnHolder* holder = (_AMLaunchFnHolder*)_holder;
            return std::invoke(holder->c, holder->obj, mem);
        }
        TObject &obj;
        AvailCallback ac;
        Callback c;
    };

    template<class T> class AMFuture
    {
    public:

        AMFuture() noexcept;
        AMFuture(AMFuture&& other) noexcept;
        AMFuture(const AMFuture& other) = delete;

        AMFuture& operator=(AMFuture&& other) noexcept;
        AMFuture& operator=(const AMFuture& other) = delete;

        bool valid() const noexcept;

        T get();
        //T& get();
        //void get();

        ~AMFuture();

        template< class Rep, class Period >
        AMFutureStatus wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) const;
        void wait() const;

        template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
        friend
        AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
        AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args);

    protected:
        template< class Callback, class AvailCallback, class Function, class TCF, class... Args > friend
        AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void*>>
        AMAsync( AMLaunch policy, Callback&& callback, AvailCallback&& a, Function&& f, TCF&& tcf, Args&&... args );

        void destroy();
        bool validFlag;
        void *mem;
        _AMLaunchFnHolderBase<T>* holder;
        AMFuture(void*, _AMLaunchFnHolderBase<T>* holder) noexcept;
    };

    template<class T> AMFuture<T>::AMFuture() noexcept:
        validFlag(false), mem(), holder(nullptr)
    {
    }

    template<class T> AMFuture<T>::AMFuture(AMFuture&& other) noexcept
    {
        validFlag = other.validFlag;
        other.validFlag = false;
        mem = other.mem;
        other.mem = nullptr;
        holder = other.holder;
        other.holder = nullptr;
    }

    template<class T> AMFuture<T>& AMFuture<T>::operator=(AMFuture&& other) noexcept
    {
        validFlag = other.validFlag;
        other.validFlag = false;
        mem = other.mem;
        other.mem = nullptr;
        holder = other.holder;
        other.holder = nullptr;
        return *this;
    }

    template<class T> AMFuture<T>::AMFuture(void* _mem, _AMLaunchFnHolderBase<T>* _holder) noexcept:
        validFlag(false), mem(_mem), holder(_holder)
    {
    }

    template<class T> AMFuture<T>::~AMFuture()
    {
        destroy();
    }

    template<class T> bool AMFuture<T>::valid() const noexcept
    {
        if (validFlag || (holder && holder->SIsAvail(mem))) {
            return true;
        }
        return false;
    }
    template<class T> T AMFuture<T>::get()
    {
        wait();
        validFlag = false;
        T rv = holder->SGet(mem);
        if (holder) {
            delete holder;
            holder = nullptr;
        }
        return rv;
    }
    /*
    template<class T> T& AMFuture<T>::get()
    {
        wait();
        return AMFuture<T>::data.find(futureId);
    }
     */
    /*
    template<class T> void AMFuture<T>::get()
    {
        wait();
    }
    */
    template<class T> void AMFuture<T>::destroy()
    {
        wait();
        if (holder) {
            delete holder;
            holder = nullptr;
        }
    }

    template<class T> void AMFuture<T>::wait() const
    {
        if (!validFlag) {
            if (holder) {
                assert(holder->SIsAvail(mem));
            }
        }
    }

    template<class T>
    template< class Rep, class Period >
    AMFutureStatus AMFuture<T>::wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) const
    {
        if (validFlag || (holder && holder->SIsAvail(mem))) {
            return AMFutureStatus::ready;
        }
        return AMFutureStatus::timeout;
    }

    template< class Function, class... Args >
    AMFuture<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
    AMAsync( Function&& f, Args&&... args )
    {
        return AMAsync(AMLaunch::async, f, args...);
    }

    template< class Callback, class AvailCallback, class Function, class TCF, class... Args >
    AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void*>>
    AMAsync( AMLaunch policy, Callback&& callback, AvailCallback&& a, Function&& f, TCF&& tcf, Args&&... args )
    {
        (void)policy;
        auto h = new _AMLaunchFnHolder(tcf, std::move(a), std::move(callback));
        void* mem = std::invoke(f, tcf, args...);
        return AMFuture<std::invoke_result_t<std::decay_t<Callback>, std::decay_t<TCF>, void*>>(mem, h);
    }



    class _AMFutureZombieBase
    {
    public:
        static bool checkZombies() {return true;};
    };

    inline bool checkZombies()
    {
        return _AMFutureZombieBase::checkZombies();
    }

}
#else

#include <future>
#include <set>

namespace AMCore {

    /**
     * \brief Status of future is exacly the same as std::future_status
     */
    typedef std::future_status AMFutureStatus;

    /**
     * \brief Launch mode is exactly the same as std::launch
     */
    typedef std::launch AMLaunch;


    template<class T>
    class AMFuture;
    template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
    AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
    AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args);

    /**
     * \brief Result of type T of asynchronous call.
     */
    template<class T>
    class AMFuture : public std::future<T> {
    public:

        /**
         * \brief default contructor
         */
        AMFuture() noexcept;

        /**
         * \brief move constructor
         * @param other copy from another AMFuture
         */
        AMFuture(AMFuture<T> &&other) noexcept;

        /**
         * \brief copy constructor is deleted.
         * @param other copy from another AMFuture
         */
        AMFuture(const AMFuture &other) = delete;

        /**
         * \brief move assignment
         * @param other move from another AMFuture
         */
        AMFuture &operator=(AMFuture &&other) noexcept;

        /**
         * \brief copy assigment
         * @param other copy from another AMFuture
         */
        AMFuture &operator=(const AMFuture &other) = delete;

        /**
         * \brief destructor.
         *
         * If launched process is not complete, moves AMFuture to internal strunctures, ehre it can wait for finish.
         */
        ~AMFuture();

        template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
        friend
        AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
        AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args);
    protected:
        template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
        friend
        AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
        AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args);

        template<class Function, class Callback, class TCF, class... Args>
        static T perform(Function &&f, Callback &&c, TCF &&tcf, Args &&... args);

        AMFuture(std::future<T> &&other) noexcept;
    };

    template<class T>
    AMFuture<T>::AMFuture() noexcept
        :std::future<T>() {
    }

    template<class T>
    AMFuture<T>::AMFuture(AMFuture<T> &&other) noexcept
        :std::future<T>(std::move(other)) {
    }

    template<class T>
    AMFuture<T>::AMFuture(std::future<T> &&other) noexcept
        :std::future<T>(std::move(other)) {
    }

    template<class T>
    AMFuture<T> &AMFuture<T>::operator=(AMFuture &&other) noexcept {
        std::future<T>::operator=(std::move(other));
        return *this;
    }

    template<class T>
    template<class Function, class Callback, class TCF, class... Args>
    T AMFuture<T>::perform(Function &&f, Callback &&c, TCF &&tcf, Args &&... args) {
        void *mem = std::invoke(f, tcf, args...);
        return std::invoke(c, tcf, mem);
    }

    /**
     * \brief asynchronous call
     *
     * Launch, possible asynchronous a function.
     *
     * @tparam Callback Get data function. Return type is T (From AMFuture<T>) a parameter is your own tag of type void *. It must be member function of type TCF.
     * @tparam AvailCallback Check, if data is available. Tou need not implement this function.  It must be member function of type TCF.
     * @tparam Function Prepare data function. It should return unique tag tied with result. Return type is a tag of void *.  It must be member function of type TCF.
     * @tparam TCF Caller object type.
     * @tparam Args All parameters of Function. It's absolutly free.
     * @param policy valid is AMLaunch::async
     * @param callback Callback type function.
     * @param a AvailCallback type function.
     * @param f Function type function.
     * @param tcf Caller object.
     * @param args Free paramater that f have.
     * @return AMFuture<T>
     */
    template<class Callback, class AvailCallback, class Function, class TCF, class... Args>
    AMFuture<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>>
    AMAsync(AMLaunch policy, Callback &&callback, AvailCallback &&a, Function &&f, TCF &&tcf, Args &&... args) {
        (void) policy;
        auto newCallback = &AMFuture<std::invoke_result_t<std::decay_t<Callback>, std::decay_t<TCF>, void *>>::template perform<std::decay_t<Function>, std::decay_t<Callback>, std::decay_t<TCF>, std::decay_t<Args>...>;
        std::future<std::invoke_result_t<std::decay_t<Callback>, TCF, void *>> base =
            std::async(
                policy,
                newCallback,
                std::forward<Function>(f),
                std::forward<Callback>(callback),
                std::forward<TCF>(tcf),
                std::forward<Args>(args)...
                );
        return std::move(
            AMFuture<std::invoke_result_t<std::decay_t<Callback>, std::decay_t<TCF>, void *>>(std::move(base)));
    }

    class _AMFutureZombieBase {
    public:
        static void add(_AMFutureZombieBase *p);

        static bool checkZombies();

        virtual ~_AMFutureZombieBase();

    protected:
        virtual bool valid() = 0;

        virtual bool ready() = 0;

        virtual void get() = 0;

        static std::set<_AMFutureZombieBase *> m_zombies;
    };

    template<class T>
    class _AMFutureZombie : public _AMFutureZombieBase, public AMFuture<T> {
    public:
        _AMFutureZombie(AMFuture<T> &&other) noexcept;

        ~_AMFutureZombie() {/*printf("zombie destruct\n");*/}

    protected:
        bool valid() override;
        bool ready() override;
        void get() override;
    };

    template<class T>
    AMFuture<T>::~AMFuture() {
        if (AMFuture<T>::valid()) {
            _AMFutureZombieBase::add(new _AMFutureZombie<T>(std::move(*this)));
        }
    }

    template<class T>
    bool _AMFutureZombie<T>::valid() {
        return AMFuture<T>::valid();
    }

    template<class T>
    bool _AMFutureZombie<T>::ready() {
        return AMFuture<T>::wait_for(std::chrono::seconds(0)) == AMCore::AMFutureStatus::ready;
    }

    template<class T>
    void _AMFutureZombie<T>::get() {
        AMFuture<T>::get();
    }

    template<class T>
    _AMFutureZombie<T>::_AMFutureZombie(AMFuture<T> &&other) noexcept
        :AMFuture<T>(std::move(other)) {
    }


    /**
     * \brief Check for active \ref AMFuture
     * @return That destroy of application is safe
     */
    inline bool checkZombies()
    {
        return _AMFutureZombieBase::checkZombies();
    }
}


#endif
#endif //SAW_ALL_AMFUTURE_H
