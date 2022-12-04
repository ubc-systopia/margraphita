#ifndef MMAPHELP_H_
#define MMAPHELP_H_

#include <sys/mman.h>

template <typename T>
class mmap_helper
{
   public:
    mmap_helper();

    mmap_helper(size_t N)
    {
        length = sizeof(T) * N;
        start_ptr = (T *)mmap(NULL,
                              length,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS,
                              -1,
                              0);
        if (start_ptr == MAP_FAILED)
        {
            perror("mmap failed in cons");
            exit(1);
        }
    }

    mmap_helper(size_t N, int fd)
    {
        length = sizeof(T) * N;
        start_ptr =
            (T *)mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (start_ptr == MAP_FAILED)
        {
            perror("mmap failed");
            exit(1);
        }
    }

    T *get_iterator() { return start_ptr; }

    void unmap()
    {
        if (munmap(start_ptr, length))
        {
            perror("unmap failed");
            exit(1);
        }
        start_ptr = nullptr;
    }

    T *remap(size_t new_length)
    {
        start_ptr = (T *)mremap(start_ptr, length, new_length, 0);
        if (start_ptr == MAP_FAILED)
        {
            perror("mremap failed");
            exit(1);
        }
        return start_ptr;
    }

   private:
    T *start_ptr;
    size_t length;
};

#endif