#include <stdio.h>
#include <stdlib.h>
#include <string.h>	
#include <sys/mman.h>
 #include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mem.h"
#define MAGIC_NUMBER 1234567

typedef struct allocated_memory_block_header {
	int size;
	int magicNumber;	
} allocated_mem_blockhd;

typedef struct free_memory_block_header {
	int size;
	struct free_memory_block_header *next;
} free_mem_blockhd;


/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
free_mem_blockhd* free_list_head = NULL;

//keeps track of the size of allocated memory and keeps track of the free list
/*
* request sizeOf- Region bytes of memory to manage
*/
void *initMem (int sizeOfRegion){
	/* Get the pagesize */
    int pagesize = getpagesize();
    printf("Size of init requested region = %d\n", sizeOfRegion);
	printf("Page size = %d\n", pagesize);
	// open the /dev/zero device
    int fd = open("/dev/zero", O_RDWR);
	//rounds up size 
	int padding = sizeOfRegion % pagesize;
    padding = (pagesize - padding) % pagesize;

    int requestSize = sizeOfRegion + padding;
    printf("Size of region = %d\n", requestSize);

   // requestSize (in bytes) needs to be evenly divisible by the page size
   void *mmappedData =
      mmap(NULL, requestSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
   if (mmappedData == MAP_FAILED) { perror("mmap"); exit(1); }
   // close the device (don't worry, mapping should be unaffected)
   close(fd);

 /* initialize one big, free block */
    free_list_head = (free_mem_blockhd *)mmappedData;
    free_list_head->size = requestSize - (int)sizeof(free_mem_blockhd);
    free_list_head->next = NULL;
printf("memory chunk address: %016p\n size of head= %d\n", (void *)free_list_head, free_list_head->size);
return mmappedData;

}


/*
*It takes as input the size in bytes of the object to be allocated 
and returns a pointer to the start of that object. It should return 
NULL if there is not enough contiguous free space to satisfy the 
request.
*/
void *allocMem (int size){
    /**
    *  If the size is less than 0 then return NULL
    */
	if (size <= 0) { return NULL; }
	//Going to need space to store the header for the memory block, so add that size to current size
	int alloc_size = size + sizeof(allocated_mem_blockhd);
	printf("allocated size: %d\n", alloc_size);

   //the current node starts at the head of the free list
   free_mem_blockhd *previous = NULL;
   free_mem_blockhd* currentNode = free_list_head;
   free_mem_blockhd* selectedBlock = NULL;

   // go through the free list to find the position that we want to allocate
   //we are using a first fit approach
   while(currentNode != NULL){
   	printf("current->size = %d\n", currentNode->size);
    printf("size i'm looking for = %d\n", size);
	if(currentNode->size >= size){
		selectedBlock = currentNode;		
	    break;
	}
	//if it doesnt satisfy size requirement move to next free block
	else{
		previous = currentNode;
		currentNode = currentNode->next;
	}
   }
  //now we have the selected block, calculate size of next free block
int unused_space = ((int)(selectedBlock->size)) -((int) size);
printf("unused space = %d\n", unused_space);

//if the size fits the block and the allocated size fits unused space
if(size < selectedBlock->size && unused_space >= alloc_size){
	//get the starting point of where the free and allocated blocks split
	int address = sizeof(free_mem_blockhd) + size;
	//points to split btw free and to be allocated
	free_mem_blockhd *updated_free =(free_mem_blockhd*) (((void *) selectedBlock) + address);
	
	//calculates the size of split node,which is unused space 
	updated_free->size = unused_space - sizeof(allocated_mem_blockhd);
	printf("new free node size = %d\n", updated_free->size);
	printf("good here\n");
	updated_free->next = selectedBlock->next;


			if(previous == NULL){
				
				free_list_head = updated_free;
			}
			else{
				previous->next = updated_free;
			}
}

else if((selectedBlock->size > size && unused_space < alloc_size)){
	printf("Unused space is too small to be a new chunk\n");
	if(previous == NULL){
				free_list_head = selectedBlock->next;
			}
			else{
				previous->next = selectedBlock->next;
			}
	return NULL;
}

//change it to an allocated block

allocated_mem_blockhd *allocation = (allocated_mem_blockhd *) selectedBlock;
allocation->size = size; 
allocation->magicNumber = MAGIC_NUMBER;
printf("allocated address = %016p\n", allocation);
printf("allocated ending address = %016x\n ", (void *)allocation + (int)allocation->size);
return (void *) (allocation + 1);
}

/*
* It frees the memory object that ptr points to
*/
//int freeMem (void *ptr)

/*
*prints the headers for all the blocks in the free list
*/
void dumpMem() {
	printf("------------------------------\nFREE LIST:\n");
	free_mem_blockhd* current = free_list_head;
	while(current != NULL){
		printf("address: %016p, total size: %d \n", current, current->size);
		current = current->next;
	}
	printf("------------------------------\n");
}

int main(int argc, char const *argv[])
{
	initMem(4000);
	allocMem(100);
	dumpMem();
	return 0;
}
