#ifndef MMAPHELP_H_
#define MMAPHELP_H_

template <typename T_>
class mmap_helper
{
   public:
    mmap_helper();

    mmap_helper(size_t N)
    {
        length = sizeof(T_) * N;
        start_ptr = (T_ *)mmap(NULL,
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
        length = sizeof(T_) * N;
        start_ptr =
            (T_ *)mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (start_ptr == MAP_FAILED)
        {
            perror("mmap failed");
            exit(1);
        }
    }

    T_ *get_iterator() { return start_ptr; }

    void unmap()
    {
        if (munmap(start_ptr, length))
        {
            perror("unmap failed");
            exit(1);
        }
        start_ptr = nullptr;
    }

    T_ *remap(size_t new_length)
    {
        start_ptr = (T_ *)mremap(start_ptr, length, new_length, 0);
        if (start_ptr == MAP_FAILED)
        {
            perror("mremap failed");
            exit(1);
        }
        return start_ptr;
    }

   private:
    T_ *start_ptr;
    size_t length;
};

#endif