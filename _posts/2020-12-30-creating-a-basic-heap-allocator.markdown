---
layout: post
title: "Creating a Basic Heap Allocator"
date: 2020-12-30
---

The heap is an important segment of memory that grows upwards, towards the stack. This section of memory is used by computers to dynamically allocate memory to store whatever data they need for their programs to work. Heap allocators are used by programs in order to allot and keep track of chunks of memory that are being used inside of the heap. Modern operating systems use efficient and effective heap allocators in their systems, but this blog post will just be going over how a very basic heap allocator can be constructed in C.

Four functions will be implemented in this basic heap allocator:
```c
void *heap_alloc(size_t size); // Allocates memory on the heap
heap_header_t *best_fit(size_t size); // Finds a free block (if available)
void heap_free(void *block); // Frees memory on the heap
void heap_defrag(); // Defrags the heap
```

Every chunk of memory on this heap allocator will have a header, which is shown below. The header will store information such as the size of the chunk and whether the chunk is being used or not. All free chunks in the heap allocator will be stored in a linked list, which is why two pointers for storing the next and previous value on the linked list of free chunks are included in the structure.
```c
struct heap_header {
    struct heap_header *next; // Next value in linked list.
    struct heap_header *prev; // Previous value in list.
    size_t size; // Amount of memory allocated for user.
    char free; // Set to 1 if memory isn't being used.
};
```

The header should always be aligned to a multiple of four bytes. In this implementation, the heap allocator will make all headers have a size of 32 bytes. This alignment property can be easily implemented by defining the structure inside of a union, as shown below.
```c
// Header for each chunk of allocated memory
typedef union heap_header {
    struct { // Actual header data that we care about
        union heap_header *next; // Next value in linked list.
        union heap_header *prev; // Previous value in list.
        size_t size; // Amount of memory allocated for user.
        char free; // Set to 1 if memory isn't being used.
    } data;
    char align[32]; // Only used to align header to 32 bytes
} heap_header_t;
```

A static global containing the head of the linked list is required for this to work. Note that this linked list will only be used to store free chunks, and it should not contain any chunks that are not free.
```c
// Head for the linked list of free chunks
static heap_header_t *head = NULL;
```

It is also necessary to keep track of where the heap begins and ends. This can be done using two global void pointers. Note that the end of the heap is also called the "program break."
```c
// Start and end of the heap
static void *heap_start = NULL;
static void *heap_end = NULL;
```

Throughout the code, the `sbrk(intptr_t increment)` function, which is located in `unistd.h`, will be used to increase or decrease the size of the heap. The `increment` value may be positive or negative, and if it is set to zero, then the program break will be returned.

When a program is first initialized, the start and end of the program's heap should be the same since no memory has been allocated for the heap yet. When the first call to `heap_alloc()` occurs, the values of the start and end of the heap must be recorded using the code shown below.
```c
// If the heap has not been initialized, then initialize it
if(heap_start == NULL && heap_end == NULL) {
    heap_start = sbrk(0);
    heap_end = heap_start;
}

```

When a user calls `heap_alloc()`, the heap allocator should first check whether a free chunk already exists in the linked list of free chunks. This heap allocator will search for a suitable free chunk by using the best fit algorithm, which returns the smallest chunk that satisfies the user's size requirement. If a suitable free chunk is found, then it must be unlinked from the linked list of free chunks and returned to the user.
```c
heap_header_t *ret = NULL;

// Check if a suitable chunk already exists in the linked list.
// Assume that size is given to us by the user
ret = best_fit(size);
if(ret) {
    // If we found a suitable free chunk in the linked list, then remove
    // that chunk from the linked list of free chunks.
    if(ret->data.prev) {
        ret->data.prev->data.next = ret->data.next;
    }
    if(ret->data.next) {
        ret->data.next->data.prev = ret->data.prev;
    }
    if(ret == head) {
        head = ret->data.next;
        if(head) {
            head->data.prev = NULL;
        }
    }
}
```

The best fit algorithm is implemented as follows:
```c
// Loops through the linked list and finds a chunk that has a size big enough to store
// the user's data. Uses the best fit algorithm. May return NULL if there is no free
// chunk in the linked list, in which case the size of the heap must be increased.
heap_header_t *best_fit(size_t size) {
    heap_header_t *ret = NULL;
    heap_header_t *curr = head;
    while(curr) {
        // Check if this current chunk meets the size requirements.
        if(curr->data.size >= size) {
            if(ret == NULL) {
                // If we haven't found a chunk yet, then store this one
                ret = curr;
            } else if(ret->data.size <= curr->data.size) {
                // If we have already found a chunk, then only store this
                // one if it has a smaller size than the stored chunk (or,
                // in other words, if it has the better fit).
                ret = curr;
            }
        }

        // Go to the next chunk in the linked list
        curr = curr->data.next;
    }

    return ret;
}
```

If this algorithm returns NULL, then the heap allocator must increase the size of the heap so that a new chunk can be created and returned to the user. This can easily be done using the `sbrk()` function described earlier.
```c
// If there are no free chunks in the linked list with a big enough
// size for the requested data, then increase the size of the heap.
ret = sbrk(size + sizeof(heap_header_t));
if(ret == (void *) -1) {
    exit(1);
}
ret->data.size = size;
heap_end += size + sizeof(heap_header_t);
```

The `heap_alloc()` function should now look something like this:
```c
// Allocates memory on the heap
void *heap_alloc(size_t size) {
    heap_header_t *ret = NULL;

    // Make sure some idiot didn't try to allocate zero bytes of memory.
    if(size != 0) {
        // If the heap has not been initialized, then initialize it
        if(heap_start == NULL && heap_end == NULL) {
            heap_start = sbrk(0);
            heap_end = heap_start;
        }

        // Check if a suitable chunk already exists in the linked list.
        ret = best_fit(size);
        if(ret) {
            // If we found a suitable free chunk in the linked list, then remove
            // that chunk from the linked list of free chunks.
            if(ret->data.prev) {
                ret->data.prev->data.next = ret->data.next;
            }
            if(ret->data.next) {
                ret->data.next->data.prev = ret->data.prev;
            }
            if(ret == head) {
                head = ret->data.next;
                if(head) {
                    head->data.prev = NULL;
                }
            }
        } else {
            // If there are no free chunks in the linked list with a big enough
            // size for the requested data, then increase the size of the heap.
            ret = sbrk(size + sizeof(heap_header_t));
            if(ret == (void *) -1) {
                exit(1);
            }
            ret->data.size = size;
            heap_end += size + sizeof(heap_header_t);
        }

        // Mark the chunk as not free and return
        ret->data.free = 0;
        ret = (void *)ret + sizeof(heap_header_t);
    }

    return ret;
}
```

Next, the heap allocator needs to deal with freeing up used memory. This is done by marking the chunk as free and defragging the entire heap. Note that this is not the most efficient way to go about doing this.
```c
// Frees the given block of memory.
void heap_free(void *block) {
    if(block != NULL) {
        // Mark the chunk of memory as free
        heap_header_t *to_free = (heap_header_t *) (block - sizeof(heap_header_t));
        to_free->data.free = 1;

        // Defrag the heap (will automatically modify linked list)
        heap_defrag();
    }
}
```

Defragmentation does three things in this implementation:
1. If two or more free blocks are sitting right next to each other, they will be combined to form one giant free block.
2. If a free block is sitting at the end of the heap, then the program break will be reduced so that the free blocks are deleted.
3. The entire linked list of free blocks will be reconstructed.

To complete requirements 1 and 3, the defragmentation algorithm must start at the beginning of the heap and loop through every chunk. It must also store the last free chunk it saw. For every free chunk, the defragmentation algorithm will do the following:
1. Check if the current chunk is adjacent to the last free chunk it saw. If so, increment the size of the last free chunk so that it "gobbles up" the current chunk.
2. Else, add the current chunk to the linked list.

If there are no free chunks in the linked list, then the algorithm will just make the current chunk the head of the linked list. The first part of the defragmentation algorithm is shown below. Note that the implementation "loops through" the chunks by adding the size of the current chunk to a pointer to the current chunk.

```c
heap_header_t *chunk = NULL, *last_free_chunk = NULL;
void *curr = NULL;
int size = 0;

// Loop through every chunk
curr = heap_start;
while(curr < heap_end) {
    chunk = (heap_header_t *) curr;
    if(chunk->data.free) { // Check if this is a free chunk
        if(!head) { // If there is no linked list, then make this the head
            head = chunk;
            head->data.next = NULL;
            head->data.prev = NULL;
            last_free_chunk = chunk; // Save the last free chunk
        } else { // Else, add the chunk to the linked list
            // Check if the current chunk is adjacent to the last free
            // chunk that we stored.
            if((void *)last_free_chunk + last_free_chunk->data.size
                    + sizeof(heap_header_t) == curr) {
                // Instead of adding this chunk to the linked list, just
                // increase the size of the last free chunk.
                last_free_chunk->data.size += sizeof(heap_header_t) + chunk->data.size;
            } else {
                // If chunks aren't adjacent, then just add the current
                // chunk to the linked list.
                chunk->data.prev = last_free_chunk;
                if(last_free_chunk) {
                    last_free_chunk->data.next = chunk;
                }
                chunk->data.next = NULL;
                last_free_chunk = chunk; // Save the last free chunk
            }
        }
    }
    curr += chunk->data.size + sizeof(heap_header_t);
}
```

The second part of the defragmentation algorithm will reduce the size of the heap if the last free chunk is at the end of the heap.

```c
// Check if the last free chunk is at the end of the heap
size = sizeof(heap_header_t) + last_free_chunk->data.size;
if((void *)last_free_chunk + size == heap_end) {
    // If so, reduce the size of the heap with sbrk() and check for errors
    if(sbrk(-1 * size) == (void *) -1) {
        exit(1);
    }

    // If the head of the linked list was the last freed chunk,
    // then we should set it to the previous chunk.
    if((void *)head + size == heap_end &&
            head != heap_start && head) {

        head = head->data.prev; // Set head to the previous value
        if(head) { // Delete the next value in the linked list
            head->data.next = NULL;
        }
    }

    // Reduce the size of the heap
    heap_end -= size;
}
```

This is what the entire algorithm looks like:
```c
// Defragmentation does the following:
// 1.) Combines free blocks sitting next to each other.
// 2.) Reduces the size of the heap if there are free block(s) at the end.
// 3.) Reconstructs the entire linked list of free blocks.
void heap_defrag() {
    heap_header_t *chunk = NULL, *last_free_chunk = NULL;
    void *curr = NULL;
    int size = 0;

    // Delete the linked list
    head = NULL;

    // Loop through every chunk
    curr = heap_start;
    while(curr < heap_end) {
        chunk = (heap_header_t *) curr;
        if(chunk->data.free) { // Check if this is a free chunk
            if(!head) { // If there is no linked list, then make this the head
                head = chunk;
                head->data.next = NULL;
                head->data.prev = NULL;
                last_free_chunk = chunk; // Save the last free chunk
            } else { // Else, add the chunk to the linked list
                // Check if the current chunk is adjacent to the last free
                // chunk that we stored.
                if((void *)last_free_chunk + last_free_chunk->data.size
                        + sizeof(heap_header_t) == curr) {
                    // Instead of adding this chunk to the linked list, just
                    // increase the size of the last free chunk.
                    last_free_chunk->data.size += sizeof(heap_header_t) + chunk->data.size;
                } else {
                    // If chunks aren't adjacent, then just add the current
                    // chunk to the linked list.
                    chunk->data.prev = last_free_chunk;
                    if(last_free_chunk) {
                        last_free_chunk->data.next = chunk;
                    }
                    chunk->data.next = NULL;
                    last_free_chunk = chunk; // Save the last free chunk
                }
            }
        }
        curr += chunk->data.size + sizeof(heap_header_t);
    }

    // Check if the last free chunk is at the end of the heap
    size = sizeof(heap_header_t) + last_free_chunk->data.size;
    if((void *)last_free_chunk + size == heap_end) {
        // If so, reduce the size of the heap with sbrk() and check for errors
        if(sbrk(-1 * size) == (void *) -1) {
            exit(1);
        }

        // If the head of the linked list was the last freed chunk,
        // then we should set it to the previous chunk.
        if((void *)head + size == heap_end &&
                head != heap_start && head) {

            head = head->data.prev; // Set head to the previous value
            if(head) { // Delete the next value in the linked list
                head->data.next = NULL;
            }
        }

        // Reduce the size of the heap
        heap_end -= size;
    }
}
```

The basic heap allocator implementation is complete. Now a `main()` function can be created to test out whether the heap allocator works.

```c
// Main function
void main(int argc, char **argv) {
    char *buf0, *buf1, *buf2;

    buf0 = (char *) heap_alloc(60);
    strcpy(buf0, "Buffer 0 is working.\n");
    buf1 = (char *) heap_alloc(60);
    strcpy(buf1, "Buffer 1 is working.\n");
    printf("%s%s", buf0, buf1);

    printf("Freeing buffer 0.\n");
    heap_free(buf0);

    buf2 = heap_alloc(50);
    strcpy(buf2, "Buffer 2 has been allocated.\n");
    printf("%s", buf2);

    if(buf0 == buf2) {
        printf("Buffer 2 has been allocated using buffer 0's chunk.\n");
    }

    printf("Freeing buffer 1.\n");
    heap_free(buf1);
    printf("Freeing buffer 2.\n");
    heap_free(buf2);
}
```

This is what heap.h looks like:
```c
#ifndef HEAP_H
#define HEAP_H

#include <stdio.h>

// Header for each chunk of allocated memory
typedef union heap_header {
    struct { // Actual header data that we care about
        union heap_header *next; // Next value in linked list.
        union heap_header *prev; // Previous value in list.
        size_t size; // Amount of memory allocated for user.
        char free; // Set to 1 if memory isn't being used.
    } data;
    char align[32]; // Only used to align header to 32 bytes
} heap_header_t;

// Functions
void *heap_alloc(size_t size); // Allocates memory on the heap
heap_header_t *best_fit(size_t size); // Finds a free block (if available)
void heap_free(void *block); // Frees memory on the heap
void heap_defrag(); // Defrags the heap

#endif
```

This is what heap.c looks like:
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "heap.h"

// Start and end of the heap
static void *heap_start = NULL;
static void *heap_end = NULL;

// Head for the linked list of free chunks
static heap_header_t *head = NULL;

// Allocates memory on the heap
void *heap_alloc(size_t size) {
    heap_header_t *ret = NULL;

    // Make sure some idiot didn't try to allocate zero bytes of memory.
    if(size != 0) {
        // If the heap has not been initialized, then initialize it
        if(heap_start == NULL && heap_end == NULL) {
            heap_start = sbrk(0);
            heap_end = heap_start;
        }

        // Check if a suitable chunk already exists in the linked list.
        ret = best_fit(size);
        if(ret) {
            // If we found a suitable free chunk in the linked list, then remove
            // that chunk from the linked list of free chunks.
            if(ret->data.prev) {
                ret->data.prev->data.next = ret->data.next;
            }
            if(ret->data.next) {
                ret->data.next->data.prev = ret->data.prev;
            }
            if(ret == head) {
                head = ret->data.next;
                if(head) {
                    head->data.prev = NULL;
                }
            }
        } else {
            // If there are no free chunks in the linked list with a big enough
            // size for the requested data, then increase the size of the heap.
            ret = sbrk(size + sizeof(heap_header_t));
            if(ret == (void *) -1) {
                exit(1);
            }
            ret->data.size = size;
            heap_end += size + sizeof(heap_header_t);
        }

        // Mark the chunk as not free and return
        ret->data.free = 0;
        ret = (void *)ret + sizeof(heap_header_t);
    }

    return ret;
}

// Loops through the linked list and finds a chunk that has a size big enough to store
// the user's data. Uses the best fit algorithm. May return NULL if there is no free
// chunk in the linked list, in which case the size of the heap must be increased.
heap_header_t *best_fit(size_t size) {
    heap_header_t *ret = NULL;
    heap_header_t *curr = head;
    while(curr) {
        // Check if this current chunk meets the size requirements.
        if(curr->data.size >= size) {
            if(ret == NULL) {
                // If we haven't found a chunk yet, then store this one
                ret = curr;
            } else if(ret->data.size <= curr->data.size) {
                // If we have already found a chunk, then only store this
                // one if it has a smaller size than the stored chunk (or,
                // in other words, if it has the better fit).
                ret = curr;
            }
        }

        // Go to the next chunk in the linked list
        curr = curr->data.next;
    }

    return ret;
}

// Frees the given block of memory.
void heap_free(void *block) {
    if(block != NULL) {
        // Mark the chunk of memory as free
        heap_header_t *to_free = (heap_header_t *) (block - sizeof(heap_header_t));
        to_free->data.free = 1;

        // Defrag the heap (will automatically modify linked list)
        heap_defrag();
    }
}

// Defragmentation does the following:
// 1.) Combines free blocks sitting next to each other.
// 2.) Reduces the size of the heap if there are free block(s) at the end.
// 3.) Reconstructs the entire linked list of free blocks.
void heap_defrag() {
    heap_header_t *chunk = NULL, *last_free_chunk = NULL;
    void *curr = NULL;
    int size = 0;

    // Delete the linked list
    head = NULL;

    // Loop through every chunk
    curr = heap_start;
    while(curr < heap_end) {
        chunk = (heap_header_t *) curr;
        if(chunk->data.free) { // Check if this is a free chunk
            if(!head) { // If there is no linked list, then make this the head
                head = chunk;
                head->data.next = NULL;
                head->data.prev = NULL;
                last_free_chunk = chunk; // Save the last free chunk
            } else { // Else, add the chunk to the linked list
                // Check if the current chunk is adjacent to the last free
                // chunk that we stored.
                if((void *)last_free_chunk + last_free_chunk->data.size
                        + sizeof(heap_header_t) == curr) {
                    // Instead of adding this chunk to the linked list, just
                    // increase the size of the last free chunk.
                    last_free_chunk->data.size += sizeof(heap_header_t) + chunk->data.size;
                } else {
                    // If chunks aren't adjacent, then just add the current
                    // chunk to the linked list.
                    chunk->data.prev = last_free_chunk;
                    if(last_free_chunk) {
                        last_free_chunk->data.next = chunk;
                    }
                    chunk->data.next = NULL;
                    last_free_chunk = chunk; // Save the last free chunk
                }
            }
        }
        curr += chunk->data.size + sizeof(heap_header_t);
    }

    // Check if the last free chunk is at the end of the heap
    size = sizeof(heap_header_t) + last_free_chunk->data.size;
    if((void *)last_free_chunk + size == heap_end) {
        // If so, reduce the size of the heap with sbrk() and check for errors
        if(sbrk(-1 * size) == (void *) -1) {
            exit(1);
        }

        // If the head of the linked list was the last freed chunk,
        // then we should set it to the previous chunk.
        if((void *)head + size == heap_end &&
                head != heap_start && head) {

            head = head->data.prev; // Set head to the previous value
            if(head) { // Delete the next value in the linked list
                head->data.next = NULL;
            }
        }

        // Reduce the size of the heap
        heap_end -= size;
    }
}

// Main function
void main(int argc, char **argv) {
    char *buf0, *buf1, *buf2;

    buf0 = (char *) heap_alloc(60);
    strcpy(buf0, "Buffer 0 is working.\n");
    buf1 = (char *) heap_alloc(60);
    strcpy(buf1, "Buffer 1 is working.\n");
    printf("%s%s", buf0, buf1);

    printf("Freeing buffer 0.\n");
    heap_free(buf0);

    buf2 = heap_alloc(50);
    strcpy(buf2, "Buffer 2 has been allocated.\n");
    printf("%s", buf2);

    if(buf0 == buf2) {
        printf("Buffer 2 has been allocated using buffer 0's chunk.\n");
    }

    printf("Freeing buffer 1.\n");
    heap_free(buf1);
    printf("Freeing buffer 2.\n");
    heap_free(buf2);
}
```

Compiling and running the program produces the following output.
```
$ gcc -o heap heap.h heap.c
$ ./heap
Buffer 0 is working.
Buffer 1 is working.
Freeing buffer 0.
Buffer 2 has been allocated.
Buffer 2 has been allocated using buffer 0's chunk.
Freeing buffer 1.
Freeing buffer 2.
```

{% include related_posts.html %}
