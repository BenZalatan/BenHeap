#pragma once

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/* https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed */
#define PACK(x) __pragma(pack(push, 1)) x __pragma(pack(pop))

/* align to 16 bit short */
#define HEAP_ALIGN 0x2
#define HEAP_TYPE short

/* things like self, alloc, data_end would only be useful in the context of a kernel implementation */
PACK(struct heap_t
{
	/* a failsafe for checking if a heap is valid */
	struct heap_t *self;
	/* a boolean indicating if the heap is free or not */
	char alloc;
	/* start of alloc header */
	char *header_start;
	/* start of data portion */
	HEAP_TYPE *data_start;
	/* end of data portion */
	HEAP_TYPE *data_end;

	/***************/

	/* the first instance of a byte containing free memory */
	char *min_free;

});

/* print a char in binary */
void printbin(char num)
{
	char i;

	/* a lot of things in this file aren't optimized properly... */

	for (i = 0; i < 8; i++)
	{
		if (num & (1 << (7 - i))) printf("1");
		else printf("0");
	}
}

/* print debug info about a heap object */
void heap_debug(struct heap_t *heap)
{
	long i;

	printf("#############################################################################\n");
	printf("--------HEAP STRUCTURE INFO-----------------\n");
	printf("SELF:         %p\n", heap->self);
	printf("ALLOC:        %i\n", heap->alloc);
	printf("HEADER_START: %p\n", heap->header_start);
	printf("	^ Off by:      %i\n", (heap->header_start - (char*)heap) - sizeof(struct heap_t));
	printf("	^ Header size: %i\n", (char *)heap->data_start - heap->header_start);
	printf("DATA_START:   %p\n", heap->data_start);
	printf("DATA_END:     %p\n", heap->data_end);
	printf("	^ Data size:   %i\n", (char *)heap->data_end - (char *)heap->data_start);
	printf("MIN_FREE:     %p\n", heap->min_free);

	printf("--------HEAP HEADER INFO--------------------\n");

	for (i = 0; i < ((char *)heap->data_start - heap->header_start); i++)
	{
		printf("HEADER [%i]: ", i);
		printbin(*(heap->header_start + i));
		printf("\n");
	}

	printf("#############################################################################\n");
}

/* values of "alloc" */
#define HEAP_UNALLOC 0x0
#define HEAP_ALLOC 0x1

/* get size of header given size of allocation */
static inline long heap_get_header_size(long size)
{
	return ceil(size / (double)(HEAP_ALIGN * 8));
}

/* align size with HEAP_ALIGN */
static inline long heap_align_size(long size)
{
	return (
		size % HEAP_ALIGN == 0 ?
		(size) :
		(size - (size % HEAP_ALIGN) + HEAP_ALIGN));
}

/* make a new heap of specified size */
struct heap_t *heap_make(long size)
{
	struct heap_t *output;
	long header_size, i;

	size = heap_align_size(size);
	header_size = heap_get_header_size(size);

	output = (struct heap_t *)malloc(
		sizeof(struct heap_t) +
		header_size +
		size);

	if (!output) return NULL;

	output->self = output;
	output->alloc = HEAP_ALLOC;
	output->header_start = (char *)((char *)output + sizeof(struct heap_t));
	output->data_start = (HEAP_TYPE *)(output->header_start + header_size);
	output->data_end = (HEAP_TYPE*)((char*)output->data_start + size);

	output->min_free = output->header_start;

	for (i = 0; i < header_size; i++) *(output->header_start + i) = 0;

	return output;
}

/* free a heap object */
void heap_delete(struct heap_t *heap)
{
	/* if this was in the context of a kernel, I would use "alloc = HEAP_UNALLOC", but since we have malloc() and free() available... */

	if (!heap || heap->self != heap) return;

	free(heap);
}

/* convert a header location to a data location */
static inline HEAP_TYPE *heap_header_get_real(struct heap_t *heap, char *pos, char bit_offset)
{
	return (HEAP_TYPE *)
		(
			(char *)heap->data_start +
			((pos - heap->header_start) * 8 * HEAP_ALIGN) +
			bit_offset
		);
}
/* convert a data location to a header location */
static inline char *heap_real_get_header(struct heap_t *heap, HEAP_TYPE *pos, char* OUT_bit_offset)
{
	*OUT_bit_offset = (((char *)pos - (char*)heap->data_start) / HEAP_ALIGN) % 8;
	return heap->header_start + (((char *)pos - (char *)heap->data_start) / HEAP_ALIGN) / 8;
}

/* allocate memory in a heap */
void *heap_malloc(struct heap_t *heap, long size)
{
	char *pos;
	long amt;
	char i;

	char *spos;
	char sbit, j;
	long si;

	spos = sbit = 0;

	size = heap_align_size(size) / HEAP_ALIGN;
	amt = 0;
	pos = heap->min_free;

	while (pos < (char *)heap->data_start)
	{
		/**/
		for (i = 0; i < 8; i++)
		{
			if (!(
				*pos & (1 << (7 - i))
				))
			{
				if (amt == 0)
				{
					spos = pos;
					sbit = i;
				}
				++amt;
			}
			else amt = 0;

			if (amt >= size)
			{
				if (!spos) return NULL; /* how? */

				for (si = 0; si <= pos - spos; si++)
				{
					for (j = (si == 0 ? sbit : 0); j <= (spos + si == pos ? i : 7); j++)
					{
						*(spos + si) |= 1 << (7 - j);
					}
				}

				if (spos == heap->min_free && *spos == 0xFF) heap->min_free = pos;

				return heap_header_get_real(heap, spos, sbit);
			}
		}
		pos++;
	}
	return NULL;
}

/* free memory in a heap */
void heap_mfree(struct heap_t *heap, void *ptr, long size)
{
	/* yes, I know it sucks to keep track of the size of allocations, whoops... I can improve the system a bit... */

	char out, *head, i;

	head = heap_real_get_header(heap, (HEAP_TYPE *)ptr, &out);

	if (head < heap->min_free) heap->min_free = (char*)head;

	while (head < heap->data_start)
	{
		for (i = (head == (char*)ptr ? out : 0); i < 8; i++)
		{
			*(head) ^= 1 << (7 - i);
			size -= HEAP_ALIGN;
			if (size <= 0) return;
		}
		head++;
	}
}