# OS_Assigment3
This memory allocation project implements a memory allocator for the heap of a user-level process that behaves with the behaviors described below:

 • void *initMem (int sizeOfRegion) - This function should call mmap to request sizeOf- Region bytes of memory to manage. initMem should return the address of the region returned by mmap. If mmap fails, return NULL.

• void *allocMem (int size) - This function should behave the same as malloc. It takes as input the size in bytes of the object to be allocated and returns a pointer to the start of that object. It should return NULL if there is not enough contiguous free space to satisfy the request.

• int freeMem (void *ptr) - This function should behave the same as free. It frees the memory object that ptr points to. Just like with the standard free(), if ptr is NULL, then no operation is performed and 0 is returned. The function returns 0 on success, and -1 otherwise.

• void dumpMem() - Use this for debugging. Prints the headers for all the blocks in the free list.

