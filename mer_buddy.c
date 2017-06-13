/*
	mer_buddy.c
*/
#include "mer_krnl.h"
#include "mer_mm.h"
#include "mer_list.h"

extern inline int PageReserved(mem_map_t *);
extern inline int put_page_testzero(mem_map_t *);
extern meros_pg_data_t contig_page_data;

extern mem_map_t* mem_map;
#ifndef DEBUG
void analyse_free_area()
{
	meros_zone_t *zone = &contig_page_data.meros_zones[1];
	free_area_t *area ;
	int i;
	//mprintk("*ANALYSE", 8, 'c');
	for (i=0; i<=MAX_ORDER-1; i++) {
		if (i == 3 || i == 2 || i ==1 || i == 0) {
			//mprintk("*ORDER=", 7, 'c');
			//mprintk(&i, 4, 'd');
			area = zone->free_area + i;
			while(area->next != NULL) {
				//mprintk("*page:", 6, 'c');
				//mprintk(&area->next, 4, 'd');
				area = area->next;
			}
		}
	}
}
#endif
unsigned int test_and_change_bit(unsigned long index, unsigned char *ara)
{
	unsigned int byte = index >> 3;
	unsigned char bit = index % 8;
	unsigned char *ptr = ara + byte;
	unsigned char ret;
	
	if ( *ptr & (1 << bit)) {
		/* bit in that location is a '1' ie buddy of this page is
			free */
		*ptr &=  ~(1 << bit) ; /* make it '0' as now both buddys are
					free */
		ret = 1;
	//	mprintk("*BUDDY IS FREE", 14, 'c');
	}
	else {
		/* bit in that location is a '0' ie buddy is also busy*/
		*ptr |= 1 << bit; /* make it '1' as one of the buddy is free
					now */
		ret = 0;
	//	mprintk("*BUDDY IS NOT FREE", 18, 'c');
	}
	return ret;
}
meros_zone_t*  get_zone(mem_map_t *page)
{
	return &(contig_page_data.meros_zones[page->my_zone]);
	
}
/*
	This is this function which is responsible for adding the 
page to the free list. First we need to check if the buddy of this
page is also free, this is done by monitoring the state of the bit
that corresponds to this page and its buddy in the bitmap. If that
bit is '1' it means that either this page or its buddy was already 
free, now as this page is not yet free, (ie it is on its way to be
free) this implies that its buddy is free and the two can be merged.
*/
#ifdef BUDDY_DEBUG
#define JUNK_LIMIT	4
#endif

/* alloc pages routines */
struct meros_page*  Expand(meros_zone_t *zone, struct meros_page *page,
				unsigned int req_order, 
				unsigned int cur_order,
				unsigned long index)
{
	/*
		zone	 : zone from which the block is going to be freed
		page	 : first page in the block
		req_order: desired order
		cur_order: current order where free pages are found, could
				be >= req_order
		index	 : offset of this page in zone->mem_map
	*/
	free_area_t *area;
	/* start splitting (if necessary) */
#ifdef DEBUG
	mprintk("*cur_order:", 11, 'c');
	mprintk(&cur_order, 4, 'd');
	mprintk("*req_order:", 11, 'c');
	mprintk(&req_order, 4, 'd');
#endif
	while (cur_order > req_order) {
		/* if we are here, it implies we need to split */
		cur_order--; /* go to the previous order */

		/* first add this page to this order */
		area = zone->free_area + cur_order;
		list_add(page, area);
		/*
			01/12/04: I all of a sudden had a doubt, whether
		I had implemented the below statement. Thank God, I had...
		*/
		test_and_change_bit((index >> (cur_order+1)),
				 area->bitmap);
		index += (1 << cur_order);
		/* increment page field */
		page += (1 << cur_order);
	}
	return page;
}		
/*
	Function: rmqueue
		
	 CORE STUFF TO BE DONE 
	 1) get a pointer to free_area_struct array in this zone 
	 2) go to the appropriate index in the array
	 3) find if the next pointer is NULL or not
		3.1)  if not NULL then there is a block of
			 required size available, return a struct page and
			 clear the approprite bit in bit map
		3.2)  if NULL then there is no block availabe, start
			going upwards till we get a non NULL  value or
			MAX_ORDER is reached
		3.3) if still NULL then return 0
		3.4) else we need to split the block, do it
		3.5) return a struct page
*/
struct meros_page* rmqueue(meros_zone_t *zone, unsigned int order)
{
	struct meros_page *page = NULL;
	free_area_t *area;
	int i;
	unsigned long index;
	mem_map_t *base = zone->zone_mem_map; /* index in to the mem_map array for this page */
	
	/*
		starting from the desired over, check if we have a
	(block of) page in that order, is it is there then remove
	it from that order. If not go to the next order, repeat this
	till we have reached MAX_ORDER.

	If there was no free page(s) in the desired order, but there are
	free page(s) in the higher order then higher order pages needs to
	be split and added to lower orders.
	*/
	for (i=order; i< MAX_ORDER; i++) {
		area = zone->free_area + i;
		if (area->next != NULL) {/*there are free pages in this order*/
			/* remove that block from this order */
			page = area->next;
			list_del(page);
			//mprintk("*Initial Page:", 14, 'c');
			//mprintk(&page, 4, 'd');
			/* determine the location of this page in zone */
			index = page - base;// zone->zone_mem_map; 
			/*
				invoke test_and_change_bit, we expect the
			particular bit to be '1' and convert it to '0'. do
			this only if current index is not 'MAX_ORDER-1' as
			there is no bitmap for MAX_ORDER-1
			*/
			if (i != MAX_ORDER-1)
				test_and_change_bit((index >> (i+1)),
				 area->bitmap);
			/* we may have to break the block, this function does
				it */
			 page =Expand(zone, page, order, i, index);
			// mprintk("*Final Page:", 12, 'c');
			 //mprintk(&page, 4, 'd');
			/*
				set the page usage count to '1' as it
			is no longer free.
			*/
			page->count = 1;
			 return page;
		}
	}
	return page;
}

/*
	Function: get_free_pages
*/
#ifndef DEBUG
extern mem_map_t* mem_map;
unsigned long page_to_virt(struct meros_page *page) 
{
	unsigned int page_offset = page - mem_map;
	unsigned int phy_addr = page_offset * 4096;
	unsigned int virt_addr = phy_addr + 0xc0000000;
	return virt_addr;
}

#endif
unsigned long Get_Free_Pages(unsigned int gfp_mask, unsigned int order)
{
	struct meros_page *page;
	unsigned long virtual = NULL;
	page = alloc_pages(gfp_mask, order);
#ifdef DEBUG
	mprintk("*Get_Free_Page:", 15, 'c');
	mprintk(&page, 4, 'd'); 
	{
		unsigned long temp_virt = page_to_virt(page);
		mprintk("*temp_virt:", 11, 'c');
		mprintk(&temp_virt, 4, 'd'); 
	}
#endif
	if (page)
		virtual = page->virtual;
	#ifdef DEBUG
		mprintk("*virtual:", 9, 'c');
		mprintk(&virtual, 4, 'd');
	#endif
		return virtual;
}
/*
	Function: alloc_pages
*/
struct meros_page* alloc_pages(unsigned int gfp_mask, unsigned int order)
{
	meros_zone_t *zone = &contig_page_data.meros_zones[ZONE_NORMAL];
	if (order > MAX_ORDER)
		return NULL;
	/* currently we are bothered about only these two flags */
	if (gfp_mask == GFP_DMA)
		zone = &contig_page_data.meros_zones[ZONE_DMA];
	else if (gfp_mask == GFP_HIGHMEM)
		zone = &contig_page_data.meros_zones[ZONE_HIGHMEM];

	/* 
		ensure we have enough free pages left in the zone 
		NOTE: We are ready for allocation if the zone has
		just enough pages, this is different from Linux where
		they do lot more computations..
	*/	
	if (zone->free_pages < (1 << order)) {
		mprintk("*Not enough pages left in this zone", 35, 'c');
		return NULL;
	}
	
	return rmqueue(zone, order);
}

/* free pages routines */
void inline __free_pages_ok(mem_map_t *page, unsigned int order)
{
	meros_zone_t *zone; /* each zone has its own free_area array */
	unsigned long page_idx; /* the index number of this page from
				the mem_map array start */
	unsigned long index; /* index of this page in the bitmap */
	unsigned long mask;
	mem_map_t *base; /* index in to the mem_map array for this page */
	free_area_t *area; /* index in to the free_area array for this order */
#ifdef BUDDY_DEBUG
	static int junk_val =0;
#endif
	int i;
	/* FIXME: need to do lots of sanity checking */

	/* find the zone to which this page belongs to */
	zone = get_zone(page);
	mask = (~0UL)<<order;
	
	/* each zone has a pointer to the mem_map array, get the
		pointer to the entry in mem_map array corresponding
		to this zone
	 */
	base = zone->zone_mem_map;
	page_idx = page-base; /* index of this page wrto mem_map*/
	index = page_idx >> (1 + order);  /* index of this page in bitmap */
	
	/* check the state of its buddy by checking the state of the bit
	  corresponding to this page and its buddy in the bitmap */
	area = zone->free_area + order;

	/*
		if the buddy of this page is free then the two
	can be merged and the status of the joined page's buddy
	needs to be checked, this may continue up to the 8th
	entry in free_area. (9th entry need not be checked as
	we do not merge buddys there 
	*/
	for (i=order; i< MAX_ORDER-1; i++) {
		/*
		1)	check the state of the buddy
		2)	if buddy is also free merge them and
			proceed to the next order
		3)	if buddy not free then no more merging,
			come out of the loop  
		*/

		mem_map_t *buddy1, *buddy2; 
		if (!test_and_change_bit(index, area->bitmap)){
			/* the buddy is not free, get out */		
			break;
		}

		/* the buddy is free, merge the two */

		/*
			Time to find the buddy of this page, it will be
		known as buddy1, and this page as buddy2, 
		so buddy2 = base + page_idx, 
		as for buddy1,   
		*/
	#ifdef MY_FORMULA
		buddy1 = (unsigned long)(base + page_idx) ^ (1 << i)*
					sizeof(mem_map_t);
	#else
		buddy1 = base + (page_idx ^ -mask);
	#endif
		buddy2 = base + page_idx;
		/* buddy1 is the index (in mem_map) of the buddy of this
			page, as it is already in the free list,
			delete it from the list 
		*/
			list_del(buddy1);
	#ifdef DEBUG
		if (junk_val <= JUNK_LIMIT) {
			mprintk("*node:", 6, 'c');
			mprintk(&buddy1, 4, 'd');
			mprintk("*buddy2", 7, 'c');
			mprintk(&buddy2, 4, 'd');
			mprintk("*list_del ", 10, 'c'); 
			mprintk("buddy1:", 7, 'c');
			mprintk(&buddy1, 4, 'd');
			mprintk("*buddy2:", 8, 'c');
			mprintk(&buddy2, 4, 'd');
		}
	#endif
		/* now, that the buddy1 is removed from the list, we can
		treat buddy1 and buddy2 as one single block, the question
		is which should be the starting of the block, buddy1 or 
		buddy2 ie, consider a case where order =2 and page_idx=12

		when order = 2, buddys are
		0-3	8-11	16-19	24-27
		4-7	12-15	20-23	28-31
		
			when page_idx = 12,
				buddy1 = 12
				buddy2 = 8  
			where as if page_idx =8
				buddy1 = 8	
				buddy2 = 12
		In both cases when we go to the next order ie order 3,
		page_idx should be 8, this can be achieved by 
		page_idx = page_idx & (mask << 1) , by left shifting
		mask by 1, we get a value which is a multiple of next
		order (here order 3), so that by 'anding' it with
		page_idx, we make it a multiple of order (if it was not
		earlier)

			In our case, if page_idx was 12 when order was 2
				after anding, page_idx = 8
					if page_idx was 8 when order was 2
				after anding, page_idx = 8 (remains same here)
					
		*/
		mask <<= 1;
		index >>=1; /* I didn't do it earlier and had to think */
		page_idx &= mask;
		area++; /* keep track of the index in free_area array */	
	}
	/* if we are here, no more merging is possible, so add to the 
		free_area array */
		list_add(base + page_idx, area);
#ifdef BUDDY_DEBUG
	if (junk_val<= JUNK_LIMIT)  {
		unsigned int node;
		node = (unsigned int)(base + page_idx);
		unsigned int node;
		mprintk("*node:", 6 ,'c');
		node = (unsigned int)(base + page_idx);
		mprintk(&node, 4, 'd');
		mprintk("*head:", 6, 'c');
		mprintk(&area, 4, 'd');
		mprintk("*node->next:", 12, 'c');
		mprintk(&((list_head*)node)->next , 4, 'd');
		mprintk("*node->prev:", 12, 'c');
		mprintk(&((list_head*)node)->prev , 4, 'd');
		mprintk("*head->next:", 12, 'c');
		mprintk( &((list_head*)area)->next, 4, 'd'); 
		mprintk("*head->prev:", 12, 'c');
		mprintk( &((list_head*)area)->prev, 4, 'd'); 
		mprintk("*list_add ", 10, 'c'); 
		mprintk(&node, 4, 'd');
	}
	junk_val++;
#endif
#if 0
	if (junk_val++ <= 10) {
	mprintk("*prev:", 6, 'c');
	mprintk(&area->prev, 4, 'd');
	mprintk("*next:", 6, 'c');
	mprintk(&area->next, 4, 'd');
	}
#endif
}
void inline __free_pages(mem_map_t *page, unsigned int order)
{
	/* we are about to assign the given page to the buddy allocator,
before that, make sure that the page is really free by checking its 
RESERVED flag and also by ensuring that its usage count is '1'  (by subtracting
the usage count and making sure not it is zero )
	*/
//	mprintk("*__free_pages", 13, 'c'); 
	if (!PageReserved(page) && put_page_testzero(page))
		__free_pages_ok(page, order);	
	else
		mprintk("*ERROR", 6, 'c');
}

/* 
	Function: Free_Pages
	Arguments
			virtual : the virtual address to be freed
			order   : order
*/

void inline Free_Pages(unsigned long virtual, unsigned int order)
{
	unsigned long phy_addr = virt_to_phys(virtual);
	unsigned int pfn = byte_to_page(phy_addr);
	struct meros_page *page = mem_map + pfn;
#ifdef BUDDY_DEBUG
	mprintk("*Free_Pages:", 12, 'c');
	mprintk(&page, 4, 'd');
#endif
	__free_pages(page, order); 
}
