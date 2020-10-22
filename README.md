# BenHeap
A proof of concept simple memory management system. This is not fully functional, and is likely quite buggy, but is just a starting point for future reference. This was written with working at a kernel level in mind, so some variables (eg. alloc, data_end, etc.) remain unused, and functions like malloc() and free() are used for simplicity.

## How It Works
The memory management system is made up of structures called Heaps (struct heap_t). Each heap object has a struct, header, and data section. The struct section contains information about the heap, including pointers to the header and data sections. The header section consists of a series of bytes. Each bit within each byte corresponds to a block of memory (in this case, aligned to 16-bit shorts), with 0 meaning unallocated, and 1 meaning allocated. Allocating memory is an O(n_1) process (where n_1 is the size of the header section) that consists of finding the string of 0's (unallocated memory) nearest to min_free (the lowest point in the header section that unallocated data is available) that has the proper size to fit the specified amount of bytes. Freeing memory is a O(n_2) (where n_2 is the number of bits in the header that the data takes up, keep in mind that n_2 << n_1 in most practical cases) process that simply sets the bits of allocated memory to 0.

## Example
If I create a heap of size 16 (meaning 16 bytes), the corresponding heap object will look like the following (assuming that the heap is aligned to 16-bit shorts):<br/>

| Info vars     | Header              | Data                           |
| ------------- | ------------------- | ------------------------------ |
| ...           | [ 0 0 0 0 0 0 0 0 ] | ... (16 bytes of randomness)   |

If we call heap_malloc(..., 6), then the struct will look like this:<br/>

| Info vars     | Header              | Data                           |
| ------------- | ------------------- | ------------------------------ |
| ...           | [ 1 1 1 0 0 0 0 0 ] | ... (16 bytes of randomness)   |
