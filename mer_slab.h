/*
	mer_slab.h
*/
/*
	NOTE: The object sizes currently allowed are
	32, 64, 128, 256, 512, 1024, 2048. 
*/
#include "mer_mm.h"
//#include "mer_list.h"
#define SLAB_SIZE_MIN	32
#define SLAB_MAX	7
#define NULL	0x0

/*FIXME: This is redundant */
typedef struct list_head_t {
	void *next;
	void *prev;
}list_head;
/* slab */
typedef struct slab_s {
	/* pointers to other slabs */
	void *next;
	void *prev;
	unsigned long mem; /* starting address of the first object slab*/
	unsigned int next_free_obj;
	unsigned int in_use; /* number of objects in use */
}slab_t;

/* The cache descriptor */
typedef struct kmem_cache_s{
	/* three pointers to slab full, partial and empty */
	list_head slab_full;
	list_head slab_partial;
	list_head slab_empty;
#if 0
	slab_t *slab_full;
	slab_t *slab_partial;
	slab_t *slab_empty;
#endif
	unsigned int obj_size; /* size of each object this cache hold*/
	unsigned int num; /* number of objects that each slab can hold */
}kmem_cache_t;

/* structure used for kmalloc */

typedef struct cache_sizes_s{
	kmem_cache_t cache_normal;
	kmem_cache_t cache_DMA;
}cache_sizes_t;
