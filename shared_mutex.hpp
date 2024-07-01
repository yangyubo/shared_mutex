#pragma once

#include <string>
#include "shared_mutex.h"

class SharedMutex {
public:
    // Initialize a new shared mutex with given `name`. If a mutex
    // with such name exists in the system, it will be loaded.
    // Otherwise a new mutex will by created.
    //
    // In case of any error, it will be printed into the standard output
    // and the returned structure will have `ptr` equal `NULL`.
    // `errno` wil not be reset in such case, so you may used it.
    //
    // **NOTE:** In case when the mutex appears to be uncreated,
    // this function becomes *non-thread-safe*. If multiple threads
    // call it at one moment, there occur several race conditions,
    // in which one call might recreate another's shared memory
    // object or rewrite another's pthread mutex in the shared memory.
    // There is no workaround currently, except to run first
    // initialization only before multi-threaded or multi-process
    // functionality.
    explicit SharedMutex(const std::string& name) {
        _rawMutex = shared_mutex_init(name.c_str());
    }

    SharedMutex(const SharedMutex&) = delete;
    void operator=(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&&) = delete;

    int lock() {
        return shared_mutex_lock(_rawMutex);
    }
    int unlock() {
        return shared_mutex_unlock(_rawMutex);
    }
    bool isValid() const {
        return shared_mutex_is_valid(_rawMutex);
    }

    // Close access to the shared mutex and free all the resources,
    // used by the structure.
    //
    // Returns 0 in case of success. If any error occurs, it will be
    // printed into the standard output and the function will return -1.
    // `errno` wil not be reset in such case, so you may used it.
    //
    // **NOTE:** It will not destroy the mutex. The mutex would not
    // only be available to other processes using it right now,
    // but also to any process which might want to use it later on.
    // For complete desctruction use `destroy()` instead.
    //
    // **NOTE:** It will not unlock locked mutex.
    int close() {
        if (_rawMutex) {
            auto const ret = shared_mutex_close(_rawMutex);
            _rawMutex = nullptr;
            return ret;
        }
        return 0;
    }


    // Close and destroy shared mutex.
    // Any open pointers to it will be invalidated.
    //
    // Returns 0 in case of success. If any error occurs, it will be
    // printed into the standard output and the function will return -1.
    // `errno` wil not be reset in such case, so you may used it.
    //
    // **NOTE:** It will not unlock locked mutex.
    int destory() {
        return shared_mutex_destroy(_rawMutex);
    }

    virtual ~SharedMutex() {
        shared_mutex_close(_rawMutex);
    }

private:
    shared_mutex_t _rawMutex = nullptr;
};
