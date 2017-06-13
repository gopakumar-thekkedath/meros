/*
	mer_slab.c
*/
#include "mer_slab.h"
#define L1_CACHE_SIZE	32
#define L1_CACHE_ALIGN(x)\
	(((x) + L1_CACHE_SIZE-1)&~(L1_CACHE_SIZE-1))
	
/* array used for kmalloc */
cache_sizes_t cache_sizes[SLAB_MAX];
void kmem_cache_init(void)
{
}

#define SET_PAGE_CACHE(page, cache)\
	(page)->next = (cache)
#define	SET_PAGE_SLAB(page, slab)\
	(page)->prev = (slab)

#define BUFCTL(slabp) \
	((char*)((slab_t*)slabp + 1))
/*
	We are going to statically define the cache structures, in Linux
this is done dynamically by calling kmem_cache_alloc
*/
void kmem_cache_sizes_init()
{
	int i;
	/* initialize the cache_sizes array */
	for (i=0; i<SLAB_MAX; i++) {
		cache_sizes[i].cache_normal.obj_size = SLAB_SIZE_MIN*(1 << i);
		cache_sizes[i].cache_normal.slab_full.next = NULL;
		cache_sizes[i].cache_normal.slab_full.prev = NULL;
		cache_sizes[i].cache_normal.slab_partial.next = NULL;
		cache_sizes[i].cache_normal.slab_partial.prev = NULL;
		cache_sizes[i].cache_normal.slab_empty.next = NULL;
		cache_sizes[i].cache_normal.slab_empty.prev = NULL;
		
		cache_sizes[i].cache_DMA.obj_size = SLAB_SIZE_MIN*(1 << i);
		cache_sizes[i].cache_DMA.slab_full.next = NULL;
		cache_sizes[i].cache_DMA.slab_full.prev = NULL;
		cache_sizes[i].cache_DMA.slab_partial.next = NULL;
		cache_sizes[i].cache_DMA.slab_partial.prev = NULL;
		cache_sizes[i].cache_DMA.slab_empty.next = NULL;
		cache_sizes[i].cache_DMA.slab_empty.prev = NULL;
#ifdef DEBUG
		mprintk("*Size:", 6, 'c');
		mprintk(&cache_sizes[i].cache_normal.obj_size, 4, 'd');
#endif
	}
}

/*
	Function: find_bufctl_size
*/

unsigned int calc_bufctl_size(unsigned int slab_size, unsigned int obj_size,
				int* wastage)
{
	/* num of obj that can be accomodated if there was no bufctl */
	unsigned int num_of_obj = slab_size / obj_size;
#ifdef DEBUG
	mprintk("*slab size:", 11, 'c');
	mprintk(&slab_size, 4, 'd');
	mprintk("*obj size:", 10, 'c');
	mprintk(&obj_size, 4, 'd');
	mprintk("*Init num of obj=", 17, 'c');
	mprintk(&num_of_obj, 4, 'd');
#endif
	*wastage = slab_size - num_of_obj*obj_size;
	/*
		FIXME: Maybe, to ensure that the object's beginning is aligned
	to 4 bytes, (num_of_obj + sizeof(slab_t)) must always be a multiple 
	of 4.
	*/
	while ( (*wastage = slab_size - (num_of_obj*obj_size + 
			L1_CACHE_ALIGN(num_of_obj + sizeof(slab_t)) )) <0) {
	#ifdef DEBUG
		{
			int temp = L1_CACHE_ALIGN(num_of_obj + sizeof(slab_t));
			int size = sizeof(slab_t);
			mprintk("*CACHE ALIGN:", 13, 'c');
			mprintk(&temp, 4, 'd');
			mprintk("*num_of_obj:", 12, 'c');
			mprintk(&num_of_obj, 4, 'd');
			mprintk("*size:", 6, 'c');
			mprintk(&size, 4, 'd');
		}
	#endif
			num_of_obj--;
	
	}
	
#ifdef DEBUG
	{
		
	unsigned int my_waste = *wastage; 
	mprintk("*wastage:", 9, 'c');
	mprintk(&my_waste, 4, 'c');
	mprintk("*Fina num of obj=", 17, 'c');
	mprintk(&num_of_obj, 4, 'd');
	}
#endif
	return num_of_obj;
}
/*
	Function: kmem_cache_alloc
	arguments
		1) cache from where to alloc

	Purpose: This function is called when the cache has no slabs to
	allocate objects from and hence a new slab needs to be allocted.		
	Assumptions: For us, the slab descriptor is always ON SLAB and
		the slab size is always one page.
*/

int kmem_cache_grow(kmem_cache_t *cache, int flags)
{
	/* request a page from buddy allocator, order = 0 ie 1 page */
	void *objp = (void*)Get_Free_Pages(flags, 0);	
	unsigned int num_of_obj, bufctl_size;
	int wastage, i;
	slab_t* slabp;
	mem_map_t *page;
	if (!objp)
		return 0;
	slabp = objp; /* As the slab descriptor is at the start of the slab*/
	
	/* If we are here, we have got a page to form our slab, and we
		have placed the slab descriptor at the beginning. As
		this slab is empty, we need to add it to cachep->slab_empty
	*/
//	cache->slab_empty = slabp;
//	mprintk("*cache grow Added a slab in cache->empty", 39, 'c');
	list_add(slabp, &cache->slab_empty);
	/* initialize the slab descriptor */
	/* commented below as it will corrupt the link list */
//	slabp->prev = slabp->next = NULL;
	slabp->next_free_obj = 0;
	slabp->in_use =0; /* initially no objects are in use */
	/* calculate the bufctl size and total wastage in bytes */
	cache->num = num_of_obj = bufctl_size = calc_bufctl_size( (1<<12), 
							cache->obj_size,
							&wastage);
	/* FIXME: Make slab->mem 4 byte aligned, do this carefully!! */
	slabp->mem = (unsigned int)(slabp) + L1_CACHE_ALIGN(sizeof(slab_t) + num_of_obj);
	/* initialize bufctl array */
	for (i=0; i<num_of_obj-1; i++) {
		BUFCTL(slabp)[i] = i+1;
#ifdef DEBUG
	{
		unsigned char* temp_bufctl = &BUFCTL(slabp)[i];
		if (i < 3) {
			mprintk("*bufctl val:", 12, 'c');
			mprintk(&temp_bufctl, 4, 'd');
		}
	}
#endif
	}
	BUFCTL(slabp)[i] = -1;

	/* in this page's next and prev pointers, store the cache and
		slab descriptor address */
	/* FIXME: Verify that this function returns the right page */
	page = virt_to_page((unsigned int)objp);
	SET_PAGE_CACHE(page, cache);
	SET_PAGE_SLAB(page, slabp);
#ifdef DEBUG
	mprintk("*PAGE:", 6, 'c');
	mprintk(&page, 4, 'd');
#endif

#ifdef DEBUG 
	{
	unsigned int slab_end = (unsigned int) slabp + 4096;
	unsigned int bufctl_start = (unsigned int) slabp + sizeof(slab_t);
	mprintk("*slab start:" ,12, 'c');
	mprintk(&slabp, 4, 'd');
	mprintk("*slab end:", 10, 'c');
	mprintk(&slab_end, 4, 'd');
	mprintk("*bufctl start:", 14, 'c');
	mprintk(&bufctl_start, 4, 'd');
	mprintk("*object start:" ,14, 'c');
	mprintk(&slabp->mem, 4, 'd');
	mprintk("*num_of_obj:", 12, 'c');
	mprintk(&num_of_obj, 4, 'd');
	mprintk("*wastage:", 9, 'c');
	mprintk(&wastage, 4, 'd');
	mprintk("*obj size:", 10, 'c');
	mprintk(&cache->obj_size, 4, 'd');
	}
#endif
	return 1; /* SUCCESS */
}
/*
	FIXME: change this into a macro so that if no partial or
	free slabs then we call call kmem_cache_grow.
*/
void *kmem_cache_alloc_one(kmem_cache_t* cache)
{
	/* 
		When we are here, we are sure that either
	cache->slab_partial or cache->slab_empty has free
	objects in them.
	*/
	slab_t *slabp;
	void *obj = NULL;
	
	
	if (!(slabp =cache->slab_partial.next)) {
	#ifdef DEBUG
		mprintk("*No slab in partial",  20 ,'c');
	#endif
		if (!(slabp = cache->slab_empty.next)) {
		#ifdef DEBUG
			mprintk("*No slab in free",  17 ,'c');
		#endif
			goto end;
		}
		/* we are going to allocate from slab_free, so move
			that slab to cache->slab_partial list 
		*/
	#ifdef DEBUG 
		mprintk("*Remove from empty and add to partial", 37, 'c');
	#endif
		list_del(slabp);
		list_add(slabp, &cache->slab_partial);
	}
	if (slabp->next_free_obj == -1)
		goto end;
	/* 
		got the slab, now get the free object 
		free obj = start_of_obj_in_slab + next_free_obj*size_of_obj;  
	*/
	obj = (void*)(slabp->mem + (slabp->next_free_obj*cache->obj_size));
	slabp->next_free_obj = BUFCTL(slabp)[slabp->next_free_obj];
	/* increment the in_use counter */
	slabp->in_use++;

	if (slabp->next_free_obj == -1) {
		/* The slab is full, add it to cache->slab_full */
	#ifdef DEBUG 
		mprintk("*Remove from empty/partial, add to full", 39, 'c');
	#endif
		list_del(slabp);
		list_add(slabp, &cache->slab_full);
	
	}
end:
	return obj;
}

/*
	Function: kmem_cache_alloc
	arguments
		1) cache from where to alloc
		
*/

void* kmem_cache_alloc(kmem_cache_t* cache, int flags)
{
	/*
		The cache has pointers to slab_partial and slab_empty,
	check this out to see if there are any slabs ready for use.
	*/
	void* obj = NULL;
try_again:
	if (!cache->slab_partial.next) {
		//mprintk("*No slab free in partial list", 29, 'c');
		if (!cache->slab_empty.next) {
			//mprintk("*No slab free in Free list", 26, 'c');
			if (kmem_cache_grow(cache, flags)) {
				//mprintk("*try_again", 10, 'c'); 
		#ifdef DEBUG
				goto try_again;
		#endif
			} else {
				mprintk("*kmem_cache_grow failed", 23, 'c'); 
				return obj;
			}
		} 
	}
	//mprintk("*Got an object:", 15, 'c');
	obj = kmem_cache_alloc_one(cache);
	return obj;
}

/*
	Function: kmalloc
		arguments: size = object size required
		flags	: allocation flags
*/

void* kmalloc(int size, int flags)
{
	int i;
	/* FIXME: As we have only two cache's ie for DMA and normal memory
		what to do if a request comes for HIGH_MEM?? 
	As per kmalloc in Linux the flags can only be

		GFP_KERNEL, GFP_USER, GFP_ATOMIC or GFP_DMA
	*/
	kmem_cache_t* cachep = NULL;  
	if(!size)
		return NULL;
	
	if (flags == GFP_DMA)
		cachep = &cache_sizes[0].cache_DMA;
	else
		cachep = &cache_sizes[0].cache_normal;
		
	for (i=0; i<SLAB_MAX; i++) {
		if (cachep->obj_size >= size) {
	#ifdef DEBUG
			mprintk("*Suitable Size:", 15, 'c'); 
			mprintk(&cachep->obj_size, 4, 'd');
	#endif
			/* we found the rigth sizes cache, allocate
			an object from it, the flags needs to be 
			passed to determine the zone to allocate 
			the memory for slab */
			return kmem_cache_alloc(cachep, flags);
		}
	#ifdef DEBUG
		mprintk("*Analysed size:", 15, 'c');
		mprintk(&cachep->obj_size, 4, 'd');
	#endif
		/*
			As the sequence of cachep is 
			cachep.normal[0].objsize 
			cachep.DMA[0].objsize 
			cachep.normal[1].objsize 
			cachep.DMA[1].objsize 
				.
				.
			cachep.normal[6].objsize 
			cachep.DMA[6].objsize 
		we need to increment cachep twice.
			
		*/
		cachep++;
		cachep++;
	}
#ifndef DEBUG
	mprintk("*Size out of range:", 19, 'c'); 
	mprintk(&size, 4, 'd');
#endif
	return NULL;
}

/*
	Function : __kfree
*/
void kmem_cache_free(kmem_cache_t *cache, unsigned int virt_addr)
{
	/*	
		The struct page's next and prev pointer holds the cache
		and slab descriptor address respectively.
	*/
        mem_map_t *page = virt_to_page(virt_addr); 
	slab_t *slabp = page->prev;
	/* find the obj num of this obj in the slab */
	unsigned int obj_num = (slabp->mem - virt_addr)/cache->obj_size;
	BUFCTL(slabp)[obj_num] = slabp->next_free_obj;
	slabp->next_free_obj = obj_num;
	/* If all the objects from this slab is freed then put it
	in the free list */
	if (--slabp->in_use == 0) {
#ifdef DEBUG
		mprintk("*All objects freed, add slab to empty", 37, 'c');
#endif
		list_del(slabp);
		list_add(slabp, &cache->slab_empty);
	} else if ((slabp->in_use + 1) == cache->num) {
	/*
	 This slab was full earlier,so freeing an object should
	 result in it getting added to partial list
	*/
#ifdef DEBUG
		mprintk("*Add this slab to partial from full", 34, 'c');
#endif
		list_del(slabp);
		list_add(slabp, &cache->slab_partial);
		
	}
}

/*
	Function : kfree
*/

void kfree(unsigned int virt_addr)
{
	if (!virt_addr)
		return;
	
	/* find the struct page corresponding to the PF this slab belongs to*/
	  mem_map_t *page = virt_to_page(virt_addr); 
	  kmem_cache_t *cache_desc = page->next;
	  kmem_cache_free(cache_desc, virt_addr);
}
