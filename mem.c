#include <stdio.h>
#include <stdlib.h>
#include <string.h>	
#include <sys/mman.h>
 #include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mem.h"
#define MAGIC_NUMBER 1234567
 

/* 
Operating Systems Programming Assignment 3 : Writing a memory manager
This program keeps track of the size of allocated memory and keeps track of the free list
Group members: Johanna Brown, Pooja Kundaje
*/

//header structure for a allocated memory block, (has size 16)
typedef struct allocated_memory_block_header {
	int magicNumber;
	int size;
	double padding;	
} allocated_mem_blockhd;

//the header structure for a free memory block, (has size 16)
typedef struct free_memory_block_header {
	int size;
	struct free_memory_block_header *next;
} free_mem_blockhd;

/* Global variable that points to the first free block */
free_mem_blockhd* free_list_head = NULL;

/*
* Initializes the memory to manage
* sizeOfRegion- requested bytes of memory to manage
*/
void *initMem (int sizeOfRegion){
	// Get the pagesize 
	int pagesize = getpagesize();
	printf("initial size of requested region = %d\n", sizeOfRegion);
	//printf("Page size = %d\n", pagesize);
	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);
	//rounds up size 
	int padding = sizeOfRegion % pagesize;
	padding = (pagesize - padding) % pagesize;

	int regionSize = sizeOfRegion + padding;
	printf("final size of region = %d\n", regionSize);

   // requestSize (in bytes) needs to be evenly divisible by the page size
	void *mmappedData =
	mmap(NULL, regionSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mmappedData == MAP_FAILED) { perror("mmap"); exit(1); }
	close(fd);

    //initialize one big, free block 
	free_list_head = (free_mem_blockhd *)mmappedData;
	free_list_head->size = regionSize - (int)sizeof(free_mem_blockhd);
	free_list_head->next = NULL;
	printf("memory chunk address: %p\n size of free head= %d\n", (void *)free_list_head, free_list_head->size);
	printf("next of free node is: %p\n ", (void *) free_list_head->next);

	return mmappedData;
}


/*
*It takes as input the size in bytes of the object to be allocated 
and returns a pointer to the start of that object. It should return 
NULL if there is not enough contiguous free space to satisfy the 
request.
* It is allocating memory to the first free block that fits first
* size is the requested size to be allocated
*/
void *allocMem (int size){
printf("------------------------------\nADDING AN ALLOCATION:\n");
   //printf("Size of free_header = %d\n", sizeof(free_mem_blockhd));
   //printf("Size of alloc_header = %d\n", sizeof(allocated_mem_blockhd));
	if (size <= 0) { return NULL; }
    //size is the requested size
   //Going to need space to store the header for the memory block, so add that size to current size
	int alloc_size = size + sizeof(allocated_mem_blockhd);
	printf("TOTAL SIZE OF ALLOCATION = %d\n",alloc_size);
    //finds the available block that satisfied size condition
	free_mem_blockhd *previous = NULL;
	free_mem_blockhd* currentNode = free_list_head;
	free_mem_blockhd* selectedBlock = NULL;
	void * ptr = NULL;
	while(currentNode != NULL){
		printf("current->size = %d\n", currentNode->size);
		printf("size i'm looking for = %d\n", size);
		if(currentNode->size >= size && currentNode != NULL){
			selectedBlock = currentNode;		
			break;
		}
	//if it doesnt satisfy size requirement move to next free block
		else{
			previous = currentNode;
			currentNode = currentNode->next;
		}
	}
		//if the block I am looking for (with size requirements) does not exist,
		//print error message and exit the method
        if(selectedBlock == NULL){
				printf("ERROR: available block size of %d does not exist\n", size);
				exit(1);
			}

	printf("found a block that satisy size req=  %p\n", selectedBlock);
	int unused_space = ((int)(selectedBlock->size)) -((int) size);
	printf("unused space (not including header)= %d\n", unused_space);
	ptr =  (void *) (free_mem_blockhd*)selectedBlock + sizeof(allocated_mem_blockhd);

	if(size < selectedBlock->size && unused_space >= alloc_size){
   	  //Once I have the selected free node that I will take memory from, I caculate the new size it will have
      //ptr will start at the head of the selected node + the size of the header

       //update free block that memory is being taken from
		free_mem_blockhd *updated_free =(free_mem_blockhd*) (((void *) selectedBlock) + sizeof(allocated_mem_blockhd) + size);
        //updates the free block selected size to be what was initially requested and the size it's header takes
		updated_free->size = unused_space - sizeof(free_mem_blockhd);
		printf("updated size of free block that was used = %d\n", (int)updated_free->size);
		if(previous == NULL){
   			//update the head of the free list to be pointing to the updated free node	
			free_list_head = updated_free;
		}
		else{
			//set the next of the previous node of the selected node to the updated free node
			previous->next = updated_free;
		}
	}

   //if size of the block is less, but not enough to change into a new block, then just update the 
   //free list to remove the whole selected block since it will be casted into an allocated block
	else if((size < selectedBlock->size&& unused_space < alloc_size)){
		printf("Unused space is too small to be a new chunk with header\n");
		if(previous == NULL){
		//Since the selected node will be allocated and takes up all the unused space,
		//then we need to update the list to start at the next free node after it 
			free_list_head = selectedBlock->next;
		}
		else{
			//set the next the previous to the next of the selected block since all of its space
			//gets used up
			previous->next = selectedBlock->next;
		}
	}
   //casts the selected block chunk into a allocated block
	allocated_mem_blockhd *allocation = (allocated_mem_blockhd *) selectedBlock;
	allocation->size = size; 
	allocation->magicNumber = MAGIC_NUMBER;
    return ptr;
} 

/*
* It frees the memory object that ptr points to
*/
int freeMem (void *ptr){
	//error handling:
	printf("______ERROR HANDLING______\n");
	//if the ptr is null, then no operation is performed and 0 is returned
	if (ptr == NULL) {
		return 0;
	}
    //checks if the blobk being pointer at has the magic number at the memory location 
	int ptrCheck =  * (int *) ((void *) ptr - sizeof(allocated_mem_blockhd));
    printf("\nthis is the header address value: = %d\n", ptrCheck);
    printf("\nthis is the magic num = %d\n", MAGIC_NUMBER);
	if(ptrCheck != MAGIC_NUMBER){
		printf("\n the magic number does not exist there\n");
		return -1;
	}

    //if what is at the pointer is not an allocated block, return -1:
	printf("------------------------------\nFREEING AN ALLOCATION NOW:\n");
	//calculate the ptr to the header of the allocated block 
	allocated_mem_blockhd *allocated_hd =  (allocated_mem_blockhd *) ((void *) ptr - sizeof(allocated_mem_blockhd));
	//calulates the size to be freed excluding header
	int size_freed = allocated_hd->size;
	//turn allocated into free block
	free_mem_blockhd *freed = (free_mem_blockhd *) allocated_hd;
	freed->size = size_freed;
	//stores the head node and sets it as the next of new free node
	free_mem_blockhd* headNode = free_list_head;
	freed->next = headNode;
	free_list_head = freed;
	return 0; 
}
/*
*prints the headers for all the blocks in the free list
*/
void dumpMem() {
	printf("------------------------------\nFREE LIST:\n");
	free_mem_blockhd* current = free_list_head;
	while(current != NULL){
		printf("address: %p, size of block: %d \n", current, current->size);
		current = current->next;
	}
	printf("------------------------------\n");
}


