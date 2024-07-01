#include "shared_mutex.h"
#include <errno.h> // errno, ENOENT
#include <fcntl.h> // O_RDWR, O_CREATE
#include <linux/limits.h> // NAME_MAX
#include <sys/mman.h> // shm_open, shm_unlink, mmap, munmap,
// PROT_READ, PROT_WRITE, MAP_SHARED, MAP_FAILED
#include <unistd.h> // ftruncate, close
#include <stdio.h> // perror
#include <stdlib.h> // malloc, free
#include <string.h> // strcpy

// Structure of a shared mutex.
typedef struct shared_mutex {
    pthread_mutex_t *ptr; // Pointer to the pthread mutex and
    // shared memory segment.
    int shm_fd;           // Descriptor of shared memory object.
    char *name;           // Name of the mutex and associated
    // shared memory object.
    int created;          // Equals 1 (true) if initialization
    // of this structure caused creation
    // of a new shared mutex.
    // Equals 0 (false) if this mutex was
    // just retrieved from shared memory.
} shared_mutex;

shared_mutex_t shared_mutex_init(const char *name) {
    shared_mutex_t mutex = malloc(sizeof(shared_mutex));
    memset(mutex, 0, sizeof(shared_mutex));
    errno = 0;

    // Open existing shared memory object, or create one.
    // Two separate calls are needed here, to mark fact of creation
    // for later initialization of pthread mutex->
    mutex->shm_fd = shm_open(name, O_RDWR, 0660);
    if (errno == ENOENT) {
        mutex->shm_fd = shm_open(name, O_RDWR|O_CREAT, 0660);
        mutex->created = 1;
    }
    if (mutex->shm_fd == -1) {
        perror("shm_open");
        return mutex;
    }

    // Truncate shared memory segment so it would contain
    // pthread_mutex_t.
    if (ftruncate(mutex->shm_fd, sizeof(pthread_mutex_t)) != 0) {
        perror("ftruncate");
        return mutex;
    }

    // Map pthread mutex into the shared memory.
    void *addr = mmap(
            NULL,
            sizeof(pthread_mutex_t),
            PROT_READ|PROT_WRITE,
            MAP_SHARED,
            mutex->shm_fd,
            0
    );
    if (addr == MAP_FAILED) {
        perror("mmap");
        return mutex;
    }
    pthread_mutex_t *mutex_ptr = addr;

    // If shared memory was just initialized -
    // initialize the mutex as well.
    if (mutex->created) {
        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr)) {
            perror("pthread_mutexattr_init");
            return mutex;
        }
        if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
            perror("pthread_mutexattr_setpshared");
            return mutex;
        }
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        if (pthread_mutex_init(mutex_ptr, &attr)) {
            perror("pthread_mutex_init");
            return mutex;
        }
    }
    mutex->ptr = mutex_ptr;
    mutex->name = (char *)malloc(NAME_MAX+1);
    strcpy(mutex->name, name);
    return mutex;
}

int shared_mutex_close(shared_mutex_t mutex) {
    if (mutex->ptr != NULL && munmap(mutex->ptr, sizeof(pthread_mutex_t))) {
        perror("munmap");
        return -1;
    }
    mutex->ptr = NULL;

    if (mutex->shm_fd > -1 && close(mutex->shm_fd)) {
        perror("close");
        return -1;
    }
    mutex->shm_fd = -1;

    free(mutex->name);
    free(mutex);
    return 0;
}

int shared_mutex_destroy(shared_mutex_t mutex) {
    if ((errno = pthread_mutex_destroy(mutex->ptr))) {
        perror("pthread_mutex_destroy");
        return -1;
    }

    if (mutex->ptr != NULL && munmap(mutex->ptr, sizeof(pthread_mutex_t))) {
        perror("munmap");
        return -1;
    }
    mutex->ptr = NULL;

    if (mutex->shm_fd > -1 && close(mutex->shm_fd)) {
        perror("close");
        return -1;
    }
    mutex->shm_fd = -1;

    if (mutex->name != NULL && shm_unlink(mutex->name)) {
        perror("shm_unlink");
        return -1;
    }
    free(mutex->name);

    free(mutex);
    return 0;
}

int shared_mutex_lock(shared_mutex_t mutex)
{
    int result = pthread_mutex_lock(mutex->ptr);
    if (result == EOWNERDEAD)
    {
        result = pthread_mutex_consistent(mutex->ptr);
        if (result != 0)
            perror("pthread_mutex_consistent");
    }

    return result;
}

int shared_mutex_unlock(shared_mutex_t mutex)
{
    return pthread_mutex_unlock(mutex->ptr);
}

bool shared_mutex_is_valid(shared_mutex_t mutex) {
    return mutex != NULL && mutex->name != NULL && mutex->shm_fd > -1 && mutex->ptr != NULL;
}
