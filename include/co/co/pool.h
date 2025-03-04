#pragma once

#include "../def.h"
#include <functional>

namespace co {

/**
 * a general pool for coroutine programming
 *   - It is designed to be coroutine-safe, users do not need to lock it.
 *   - It stores void* pointers internally and it does not care about the actual
 *     type of the pointer.
 *   - It is usually used as a connection pool in network programming.
 * 
 *   - NOTE: Each thread holds its own pool, users SHOULD call pop() and push() 
 *     in the same thread.
 */
class __coapi pool {
  public:
    // default constructor without ccb and dcb 
    pool();
    ~pool();

    /**
     * the constructor with parameters
     *
     * @param ccb  a create callback like:   []() { return (void*) new T; }
     *             it is used to create an element when pop from an empty pool.
     * @param dcb  a destroy callback like:  [](void* p) { delete (T*)p; }
     *             it is used to destroy an element.
     * @param cap  max capacity of the pool for each thread, -1 for unlimited.
     *             this argument is ignored if dcb is NULL.
     *             default: -1.
     */
    pool(std::function<void*()>&& ccb, std::function<void(void*)>&& dcb, size_t cap=(size_t)-1);

    pool(pool&& p) : _p(p._p) { p._p = 0; }

    pool(const pool& p);

    void operator=(const pool&) = delete;

    /**
     * pop an element from the pool of the current thread 
     *   - It MUST be called in coroutine.
     *   - If the pool is empty and ccb is set, ccb() will be called to create a new element.
     *
     * @return  a pointer to an element, or NULL if pool is empty and ccb is not set.
     */
    void* pop() const;

    /**
     * push an element to the pool of the current thread 
     *   - It MUST be called in coroutine.
     *   - Users SHOULD call push() and pop() in the same thread.
     *
     * @param e  a pointer to an element, nothing will be done if e is NULL.
     */
    void push(void* e) const;

    /**
     * return pool size of the current thread 
     *   - It MUST be called in coroutine.
     */
    size_t size() const;

    /**
     * clear pools of all threads 
     *   - It can be called from any where. 
     *   - If dcb is set, dcb() will be called to destroy elements in the pools. 
     */
    void clear() const;

  private:
    void* _p;
};

/**
 * guard to push an element back to co::pool
 *   - pool::pop() is called in the constructor.
 *   - pool::push() is called in the destructor.
 *
 *   - usage:
 *     struct T { void hello(); };
 *     co::pool pool(
 *         []() { return (void*) new T; }, // ccb
 *         [](void* p) { delete (T*)p; },  // dcb
 *         8192                            // max capacity
 *     );
 *
 *     co::pool_guard<T> g(pool);
 *     g->hello();
 */
template<typename T>
class pool_guard {
  public:
    explicit pool_guard(const pool& p) : _p(p) {
        _e = (T*)_p.pop();
    }

    explicit pool_guard(const pool* p) : pool_guard(*p) {}

    ~pool_guard() { _p.push(_e); }

    T* operator->() const { assert(_e); return _e; }
    T& operator*()  const { assert(_e); return *_e; }

    bool operator==(T* e) const noexcept { return _e == e; }
    bool operator!=(T* e) const noexcept { return _e != e; }
    bool operator!() const noexcept { return _e == NULL; }
    explicit operator bool() const noexcept { return _e != NULL; }

    // get the pointer owns by pool_guard
    T* get() const noexcept { return _e; }

  private:
    const pool& _p;
    T* _e;
    DISALLOW_COPY_AND_ASSIGN(pool_guard);
};

using Pool = pool;

template<typename T>
using PoolGuard = pool_guard<T>;
 
} // co
