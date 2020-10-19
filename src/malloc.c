#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allocated memory   */
   struct _block *next;  /* Pointer to the next _block of allocated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \877108
  Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0

// Find first block with enough size and which is free and assign to best
// Implement best fit by comparing every other block which is free and has enough size but
// size less than that of present best
// update best accordingly.

   struct _block *best_block = NULL;
    int p=1;
    label:
        while(curr)
            {
                if(curr->free && curr->size>=size)
                {
                    if(p==1)
                    {
                        best_block = curr;
                        *last = curr;
                        curr = curr->next;
                        p++;
                    goto label;
                    }
                    if(curr->size < best_block->size)
                        {
                            best_block = curr;
                        }
                }
            *last = curr;
            curr = curr->next;
            }

    curr = best_block;

#endif

#if defined WORST && WORST == 0

// Find first block with enough size and which is free and assign to worst
// Implement worst fit by comparing every other block which is free and has enough size but
// size more than that of present worst
// update worst accordingly.

   struct _block *worst_block = NULL;
    int p=1;
    label2:
        while(curr)
            {
                if(curr->free && curr->size>=size)
                {
                    if(p==1)
                    {
                        worst_block = curr;
                        *last = curr;
                        curr = curr->next;
                        p++;
                    goto label2;
                    }
                    if(curr->size > worst_block->size)
                        {
                            worst_block = curr;
                        }
                }
            *last = curr;
            curr = curr->next;
            }

    curr = worst_block;

#endif

#if defined NEXT && NEXT == 0

// Find the block which is last allocated by searching for block which is not free
// Implement next fit by allocating memory to the block just after the last non-free/last allocated block

   struct _block *next_block = NULL;
   while(curr)
   {
       if(curr->free)
        {
            if(curr->size >= size)
                {
                    next_block = curr;
                }
        }
       *last = curr;
       curr = curr->next;

   }
   curr = next_block;

#endif
    num_reuses++;
   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   num_blocks++;
   num_grows++;

   max_heap = max_heap + size;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
      num_reuses--;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   /* Mark _block as in use */
   next->free = false;
   num_mallocs++;
   num_requested = num_requested + size;
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}
// Calculate the total size and initialize the obtained total size
// memory contents to zero using memset.
void *calloc(size_t a, size_t size)
{
    size_t total_size = a * size;
    struct _block *res = malloc(total_size);
    if(!res)
        {
            return NULL;
        }
    else
        {
            memset(res, 0, total_size);
        }
    return res;
}

// Take the new pointer and new size as argument.
// Copy the new pointer to the old pointer with the new size.
void *realloc(void * ptr, size_t size)
{
    struct _block *res1 = malloc(size);
    if(ptr)
    {
        memcpy(res1,ptr,size);
    }
    else
    {
        return NULL;
    }
    return res1;
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   num_frees++;

   /* TODO: Coalesce free _blocks if needed */

   // Traverse through the memory to find any free block
   // Check if the block next to that block is also free
   // If both consecutive blocks are free, make a new size by adding the
   // size of both the blocks as well as struct _block
   // Initialize the current block with the new size
   // Repeat till the end of memory blocks

   struct _block *new_next = NULL;
   struct _block *curr5 = freeList;
   new_next = curr5->next;
   while(curr5)
   {
   if(new_next!= NULL)
   {
       if(curr5->free)
       {
            if(new_next->free)
       {
           size_t new_size = curr5->size + new_next->size + sizeof(struct _block);
           curr5->size = new_size;
           new_next = new_next->next;
           curr5->next = new_next;

           num_blocks--;
           num_coalesces++;
       }
       }
   }
   curr5=curr5->next;
   }
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
