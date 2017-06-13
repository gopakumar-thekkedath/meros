
/*
	File : mer_mm.c
*/
#include "mer_krnl.h"
#include "mer_mm.h"
/*
	declaration of mem_map 
*/
mem_map_t *mem_map;
meros_pg_data_t contig_page_data; /* NODE, representing the entire phy mem*/
static char *zone_names[MAX_NR_ZONES] = { "Dma", "Normal", "Highmem" };  

extern void inline __free_pages(mem_map_t*, unsigned int);

inline int put_page_testzero(mem_map_t *page)
{
	page->count--;
	if (!page->count)
		return 1;
	return 0; /* it is not zero */
}

inline unsigned int phys_to_virt(unsigned int phy_addr)
{
	return phy_addr + GB_3;
}
inline unsigned int virt_to_phys(unsigned int virt_addr)
{
	return virt_addr - GB_3;
}
inline unsigned int byte_to_page(unsigned int byte)
{
	return byte >> 12 ;
}
inline unsigned int page_to_byte(unsigned int page)
{
	return page << 12 ;
}

inline void set_bit(unsigned char *addr, unsigned int bit)
{
	unsigned int byte = bit/8;
	bit %= 8;
	*(addr + byte) |= (1 << bit);
}
/*
	28/12/04: Added the function virt_to_page and pfn_to _page
*/

inline mem_map_t* pfn_to_page(unsigned int pfn)
{
	return mem_map + pfn;
}
/* FIXME: Make below function more professional */
inline mem_map_t * virt_to_page(unsigned int vaddr)
{
	
	mem_map_t *page;
	unsigned int pfn;
	unsigned int phys = virt_to_phys(vaddr);
	pfn = byte_to_page(phys);
	return pfn_to_page(pfn);
}

/*
 Function: get_free_page:
	returns the first free page number available
*/
unsigned int get_free_page(void)
{
	unsigned char* bmp_start = b_mem_mgr.bitmap;
	unsigned int bmp_size = b_mem_mgr.bitmap_size;
	int i, j, byte_val, bit;
	for (i=0; i< bmp_size; i++) {
		byte_val = *(bmp_start + i);
		for (j=0; j<8; j++) {
			if( !((1<<j) &byte_val) ) {
				bit = j;
				set_bit(bmp_start, i*8 + bit);							return i*8 + bit;
			}
		}
		
	}
	return -1;
}

/*
 Function: get_free_pages:
	returns the virtual address of first free page number available
	in the group of contiguous pages.
*/
unsigned int get_free_pages(unsigned int num_of_pages)
{
	unsigned char* bmp_start = b_mem_mgr.bitmap;
	unsigned int bmp_size = b_mem_mgr.bitmap_size;
	unsigned long phy_addr;
	int i, j, k, byte_val, bit_start;
	int pg_cnt =0;
	for (i=0; i< bmp_size; i++) {
		byte_val = *(bmp_start + i); /* read each byte */
		for (j=0; j<8; j++) { /* for each bit in the byte */
			if( !((1<<j) &byte_val) ) { /* is the bit zero ? */
				pg_cnt++;
				if (pg_cnt == num_of_pages ) { /* got it ? */
					bit_start = (i*8)+j-(num_of_pages-1);
				/* beginning from bit_start set num_of_pages
					number of bits to '1' to indicate that
					they are reserved */
					for (k=0; k<num_of_pages; k++) 
						set_bit(bmp_start,bit_start+k);
					/*
						convert the PFN to physical
					address, and return the corresponding
					virutal address.
					*/
					phy_addr = page_to_byte(bit_start);
					return (unsigned int)
						phys_to_virt(phy_addr) ;
			
				}
			} else
				pg_cnt = 0;
		}
		
	}
	return -1;
}

void reserve_pages(unsigned int start_addr, unsigned int end_addr)
{
	unsigned int start_pfn, end_pfn;
	int i;
	start_pfn = byte_to_page(start_addr);
	end_pfn = byte_to_page(end_addr);
	/* now find the corresponding bits in the bitmap */
#ifdef DEBUG
	mprintk("*Reserve page start:", 20, 'c');
	mprintk(&start_pfn, 4, 'd');
	mprintk("*Reserve page end:", 18, 'c');
	mprintk(&end_pfn, 4, 'd');
	/* set (end_pfn - start_pfn) + 1  bits to '1' */
	
	mprintk("*bitmap[0]:", 11, 'c');
	mprintk(b_mem_mgr.bitmap, 4, 'd');
	mprintk("*bitmap[1]:", 11, 'c');
	mprintk(b_mem_mgr.bitmap + 1, 4, 'd');
#endif
	for (i=start_pfn; i<=end_pfn; i++)
		set_bit(b_mem_mgr.bitmap , i);
#ifdef DEBUG
	mprintk("*bitmap[0]:", 11, 'c');
	mprintk(b_mem_mgr.bitmap, 4, 'd');
	mprintk("*bitmap[1]:", 11, 'c');
	mprintk(b_mem_mgr.bitmap + 1, 4, 'd');
#endif
}

/* map the virtual address from 0xc0000000 to physical addrss 0x0 on wards */
inline void setup_pagetables(unsigned int* pd_base, unsigned int mem_size)
{

	unsigned int mem_cur = (1 << 12) | PRESENT ;
	unsigned int pde_loc = 769;	
	unsigned int pte_loc = 0;
	unsigned int pd_content = *pd_base;
	unsigned int next_free_page;
	unsigned int phy_addr;
	unsigned int *pt_base;
	int first_time =1;
	unsigned char *test_ptr = (unsigned char*)(0xc0000000 + 0x4000000);
	unsigned char test_var;
	/* align memory size to page size */
	mem_size &= 0xFFFFF000;
#ifdef DEBUG
	mprintk("*Inside setup_pagetables", 24, 'c');
	mprintk("*PD Address :", 13, 'c');
	mprintk(&pd_base, 4, 'd');
	mprintk("*PD Content :", 13, 'c');
	mprintk(&pd_content, 4, 'd');

	mprintk("*Memory Size :", 14, 'c');
	mprintk(&mem_size, 4, 'd');
#endif
	/********************************************************/
	/* start setting up the PGD */
	while (pde_loc < PGD_SIZE && mem_cur < mem_size) {
		/* get a page to act as a page table */
		next_free_page = get_free_page();
		/* 
		FIXME: Need to do some thing about this, cannot 
		proceed if there are no pages to act as page tables !!
		 */
		if (next_free_page < 0)	
			break;
		phy_addr = page_to_byte(next_free_page);
		/* convert it to virtual addrss and save */
		pt_base = (unsigned int*)phys_to_virt(phy_addr) ;
		/* set the present bit */
		phy_addr|= PRESENT;
		/* set it as the current PGD entry */
		*(pd_base + pde_loc++ ) = phy_addr;
		/* now fill the page with PTEs */
		pte_loc = 0;
		while (pte_loc < PT_SIZE && mem_cur < mem_size) {
			 	*(pt_base + pte_loc++) = mem_cur ;		
			 	mem_cur += 0x1000;
		#ifndef GKT_DBG
				if (mem_cur >= 0xb8000 && mem_cur < 0x100000)
					mem_cur |= CACHE_DISABLE;
		#endif
		}
	}
	/* Kernel page table setup done! now flush TLB */
	flush_tlb();
#ifdef DEBUG
	mprintk("*PT_BASE :", 10, 'c');
	mprintk(&pt_base, 4, 'd');
	mprintk("*MEM_CUR :", 10, 'c');
	mprintk(&mem_cur, 4, 'd'); 
	mprintk("*Next Free Page :", 17, 'c');
	mprintk(&next_free_page, 4, 'd'); 
	mprintk("*PD Loc :", 9, 'c');
	mprintk(&pde_loc, 4, 'd');
	mprintk("*PT Base :", 10, 'c');
	mprintk(&pt_base, 4, 'd');
#endif
	*test_ptr = 33;
	test_var = *test_ptr;
	mclrscr();
}
/* Functions pertaining to zone based memory managment are here */


/*
*	Function: zone_sizes_init
*/

void zone_sizes_init()
{
	/* zone_sizes initialization */
	unsigned long total_mem = b_mem_mgr.mem_size;
	unsigned long zone_dma_size;
	unsigned long zone_normal_size;
	unsigned long zone_highmem_size;
	zone_sizes[0]= 0x0;
	zone_sizes[1]= 0x0;
	zone_sizes[2]= 0x0;

	/*
		ZONE_DMA should be minimum 4MB and max 16 MB, ideally
		20% of available physical memory.
	*/
	 #define MIN_ZONE_DMA_SIZE	4*1024*1024
	#define MAX_ZONE_DMA_SIZE	16*1024*1024
	#define MAX_ZONE_NORMAL_SIZE	896*1024*1024

	/* 
		if 20% of available memory is greater than 16MB then
		zone_dma_size = 16 MB
	*/
	if ( ((total_mem*20)/100) > MAX_ZONE_DMA_SIZE ) {
		zone_dma_size = MAX_ZONE_DMA_SIZE;
	}
	/* 
		if 20% of available memory is less than 4MB then
		zone_dma_size = 4 MB
	*/
	else if( ((total_mem*20)/100) < MIN_ZONE_DMA_SIZE){
		zone_dma_size = MIN_ZONE_DMA_SIZE;
	}
	else
		zone_dma_size = (total_mem/20);

	zone_sizes[ZONE_DMA] = zone_dma_size;
	if (total_mem > zone_dma_size) {
		zone_sizes[ZONE_NORMAL] = max_low_pfn*4096 - zone_dma_size;
		if (high_start_pfn) 
			zone_sizes[ZONE_HIGHMEM] =(high_end_pfn
				 - high_start_pfn)*4096;
			
	}

	/*
		convert the zone_sizes in to pages
	*/		
	zone_sizes[ZONE_DMA] >>= 12;
	zone_sizes[ZONE_NORMAL] >>= 12;
	zone_sizes[ZONE_HIGHMEM] >>= 12;
#ifdef DEBUG
	mprintk("*zone dma in bytes:", 19, 'c');
	mprintk(&zone_dma_size, 4, 'd');
	mprintk("*zone_sizes[0]:",15, 'c');
	mprintk(&zone_sizes[ZONE_DMA], 4, 'd'); 
	mprintk("*zone_sizes[1]:",15, 'c');
	mprintk(&zone_sizes[ZONE_NORMAL], 4, 'd'); 
	mprintk("*zone_sizes[2]:",15, 'c');
	mprintk(&zone_sizes[ZONE_HIGHMEM], 4, 'd'); 
#endif
}
/*
*/
int check_if_free_and_mark_reserved(unsigned long preferred, 
					unsigned long num_of_pages) {
	unsigned char* bmp_start = b_mem_mgr.bitmap;
	unsigned int bmp_size = b_mem_mgr.bitmap_size;
	unsigned int num_of_bytes = (num_of_pages/8) + 1;
	unsigned int preferred_byte = preferred/8;
	int i, j, byte_val, bit;
#ifdef DEGUG
	unsigned long last_pf;
#endif
	
#ifdef DEBUG
	mprintk("*Num of pages for allocation:", 29, 'c');
	mprintk(&num_of_pages, 4, 'd');
	mprintk("*Preferred PF beginning", 23, 'c');
	mprintk(&preferred, 4, 'd');   
	mprintk("*Last PF for mem_map", 23, 'c');
#endif
#ifdef DEBUG
	last_pf = preferred + num_of_pages -1;
	mprintk(&last_pf, 4, 'd');   
#endif
	if (num_of_bytes > bmp_size) 
		return -1;	

	for (i=preferred_byte; i< (preferred_byte + num_of_bytes); i++) {
		byte_val = *(bmp_start + i);
		for (j=0; j<8; j++) {
			if( ((1<<j) &byte_val) ) {
			#ifdef DEBUG
				mprintk("*Page Already Reserved", 22, 'c'); 
			#endif
				return -1;
			}
			
		}
		
	}
#ifdef DEBUG
	mprintk("*Pages Not yet reserved", 23, 'c');
#endif
	/* if we are here, the pages were not reserved yet, do it now */
	for (i=preferred_byte; i< (preferred_byte + num_of_bytes); i++) 
		*(bmp_start+i) = 0xff;
	return 0; /* SUCCESS */

}	
/* 
 * Function: alloc_bootmem_node()
 * Parameters: 
 *	map_size : total size in bytes required by mem_map
 * 	goal	 : starting address of mem_map, here 16MB	
 */

struct meros_page* alloc_bootmem_node(unsigned long map_size, 
						unsigned long goal)
{
	unsigned long num_of_pages; /* number of pages required for this 
					allocation */
	unsigned long preferred =0;	/* preferred page frame to begin
					allocation */
	preferred = (preferred + goal)/PAGE_SIZE;
	num_of_pages = (map_size + PAGE_SIZE -1 )/PAGE_SIZE;
	/*
		Now we have the desired beginning PFN and number of pages
		required, next step is to find out whether that many pages
		starting from the required PFN are free or not.
	*/
	if (check_if_free_and_mark_reserved(preferred, num_of_pages)) {
		mprintk("*Pages NOT Available", 20, 'c');
		return 0;
	} 
#ifdef DEBUG
	mprintk("*Pages Available", 16, 'c');
#endif
	return phys_to_virt(goal);
}
/*
*	Function: build_zonelists
*	Builds allocation fall back zone lists
*/
void build_zonelists(meros_pg_data_t *pgdat)
{
	return;
}
/*
*	Function: free_area_init_core
	Arguments:
		1) node which represent the physical memory
		2) pointer to mem_map
		3) zone_sizes array which holds the page count for each zone.
	
	Purpose: initialize the various fields in struct node and each of 
	struct zone. Allocate memory in first available space in ZONE_NORMAL
	for mem_map. After mem_map, allocate memory in ZONE_NORMAL for the 
	bitmaps used by the buddy allocator.
	Finally do the basic setup for buddy allocator.
	
*/

void free_area_init_core(meros_pg_data_t *pgdat, struct meros_page **gmap ,
				 unsigned long *zone_sizes)
{
	unsigned long total_pages=0; /* total number of pages in zones */
	unsigned long map_size; /* total size in bytes required by mem_map*/
	unsigned long offset =0;
	unsigned long zone_start_paddr = 0x0;
	int i, j;
	/*
		we need to allocate memory for 'bitmap' field in free_area
	 array, and there are 10 elements in one free_area array and we have	
	3 such arrays. But we currently have only page wise allocation in place
	and it will be a great waste if we allocate one or more pages for each
	bitmap. So, what we will do is, for each zone_size, we calculate the
	total size required by the 10 bitmaps and allocate the nearest number
	of pages. Next we will make each bitmap point appropriately.

	*/
	unsigned long bitmap_size =0; /* bitmap size in bytes */
	unsigned long bitmap_pages; /* bitmap size in pages */
	unsigned long bitmap_begin;
	for (i=0; i<MAX_NR_ZONES; i++) 
		total_pages += zone_sizes[i];
#ifdef DEBUG
	mprintk("*Total pages:", 13, 'c');
	mprintk(&total_pages, 4, 'd');
#endif
	map_size = (total_pages + 1)*sizeof(struct meros_page);
	/* Allocate memory for mem_map array in ZONE_NORMAL */
	*gmap = pgdat->node_mem_map = alloc_bootmem_node(map_size, 
					virt_to_phys(MAX_DMA_ADDRESS));

	if (!pgdat->node_mem_map) {
		mprintk("*SCREWED!!!", 8, 'c');
		while(1);
	}
#ifdef DEBUG
	mprintk("*mem_map=", 9, 'c');
	mprintk(&pgdat->node_mem_map, 4, 'd'); 
#endif
	pgdat->node_size = total_pages;
	pgdat->nr_zones = 0; /* initially */

	/* so, lets start setting up the zones */
	for (i=0; i<MAX_NR_ZONES; i++) {
		unsigned long size;
		meros_zone_t *zone = pgdat->meros_zones + i;
		size = zone_sizes[i];
		zone->size = size; /* zone size in pages */
		zone->name = zone_names[i];
		zone->free_pages = 0; /*initially */
	#ifdef DEBUG
		mprintk("*ZONE :", 7, 'c');
		mprintk("*zone->size:", 12, 'c');  
		mprintk(&zone->size, 4, 'd');
	#endif
		/* if no pages in this zone, do not proceed */
		if (!size)
			continue;
	
		pgdat->nr_zones = i+1;
		/*
			FIXME: Here, do the stuff related to zone balancing
			calculate the values for zone->pages_min, 
			zone->pages_low, zone->pages_high.
				---
		*/
		zone->zone_mem_map = mem_map + offset;
	#ifdef DEBUG
		mprintk("*zone->mem_map:", 15, 'c');
		mprintk(&zone->zone_mem_map, 4, 'd');
	#endif
		zone->zone_start_mapnr = offset;
		zone->zone_start_paddr = zone_start_paddr;
	#ifdef DEBUG
		mprintk("*zone->mapnr", 12, 'c');
		mprintk(&zone->zone_start_mapnr, 4, 'd');
	#endif

		/*
		 * Time for page setup for each zone
		 * Initially, all pages are reserved- free ones are free
		 * by free_all_bootmem() once early boot process is over.
		 */
	       
		for (j=0; j<size; j++) {
			struct meros_page *page = mem_map + offset + j;
			page->count = 0; /* page is free */
			page->flags |=  PAGE_RESERVED; /* page reserved*/
			/* virtual field not set if ZONE_HIGHMEM as highmem
			pages are not directly mapped */
			if (j != ZONE_HIGHMEM)
				page->virtual = phys_to_virt(zone_start_paddr);
		
			page->next = page;
			page->prev = page;
			/* zone to which this page belongs to */
			page->my_zone = i;
			zone_start_paddr += PAGE_SIZE;
		} 
		offset+= size;
		/* 
		 *  Buddy related stuff
		 * here, we are now going to perform the initialization of
		 * free_area array which is present in each of the zone
		 * structures.
		*/
	#ifdef DEBUG	
		/* debugging stuff, remove later */
		if (i==1) {
			mprintk("*ZONE->size:", 12, 'c');  
			mprintk(&size, 4, 'd');
		}
	#endif
		bitmap_size = 0;
		for(j=0; j<MAX_ORDER -1; j++) {
			/* divide the number of pages in current zone by 16,
			NOTE that we divide it by 16 and not 8, as each bit
			will represent the state of two pages (buddies).
			*/	
			zone->free_area[j].bitmap_size = (size-1) >> (4 + j);
			zone->free_area[j].bitmap_size =
				LONG_ALIGN(zone->free_area[j].bitmap_size+1);
			bitmap_size += zone->free_area[j].bitmap_size;
			/* added on 13/11/04 as a probable fix to list_del
			crashing , still crashes thou!!
			16/11/04, the orignial reason for the crash was the
			improper computation of buddy1.
			*/
			//zone->free_area[j].next = 0x0;
			//zone->free_area[j].prev = 0x0;
		#ifdef DEBUG
			/* debugging stuff */
			if (i==1) {
				mprintk("*index:", 7, 'c');
				mprintk(&j, 4, 'd');
				mprintk("*bmp size:", 10, 'c');
			        mprintk(&zone->free_area[j].bitmap_size, 4,'d');
			}
		#endif
			/* 1/11/04, make the linked list elements NULL */
			zone->free_area[j].next = NULL;	
			zone->free_area[j].prev = NULL;	
		}	
		/* 1/11/04, make the linked list elements NULL */
		zone->free_area[j].next = NULL;	
		zone->free_area[j].prev = NULL;	
		/*
		 Now we will have the total number of bytes required by
		  all the bitmap fields in current zone. Next, we will cal-			  culate the nearest total number of pages that is required
		  for holding that many bytes and allocate that number of 
		  pages.
		*/
	#ifdef DEBUG 
		mprintk("*Zone:", 7, 'c');
		mprintk(&i, 4, 'd');
		mprintk("*Num of Bytes:", 14, 'c');
		mprintk(&bitmap_size, 4, 'd');
	#endif
		bitmap_pages = ((bitmap_size + 4095) & ~0xfff) >> 12;
		/* bitmap_pages may be more than 1 and in that case we
		 need to make sure that we have contiguous pages available
		*/
		bitmap_begin = get_free_pages(bitmap_pages);
	#ifdef DEBUG 
		mprintk("*Num of Pages:", 14, 'c');
		mprintk(&bitmap_pages, 4, 'd');
		mprintk("*Pages Begins At:", 17, 'c');
		mprintk(&bitmap_begin, 4, 'd');
	#endif
		
		/*
			Once we get the required number of free contiguous
		pages then start assigning bitmap to appropriate address
		and memset it to zero.
		*/
		for(j=0; j<MAX_ORDER -1; j++) {
			if (j ==0)
				zone->free_area[j].bitmap = bitmap_begin + 0;
			else
				zone->free_area[j].bitmap = 
					(unsigned long)zone->free_area[j-1].
							bitmap +
					 zone->free_area[j-1].bitmap_size; 
			/* debug stuff */
		#ifdef DEBUG
			if (i==1) {	
				mprintk("*Index:", 7, 'c');
				mprintk(&j, 4, 'd');
				mprintk("*bitmap", 7, 'c');
				mprintk(&zone->free_area[j].bitmap, 4, 'd');
			}
		#endif	
		}
		zero_fill(bitmap_begin, bitmap_pages*PAGE_SIZE );	
	}
	/* builds allocation fallback zone lists, not implemented now */
	build_zonelists(pgdat);
}
/*
 *	 Function: start_zone_mgmt
 *	 This is invoked from mer_krnl.c and is the first function
 *	 that deals with the zone based memory manger.
*/
void start_zone_mgmt()
{
	max_low_pfn = 0x0;
	high_start_pfn = 0x0;
	high_end_pfn = 0x0;
	/* calculate max_low_pfn */
	if (b_mem_mgr.mem_size > 896*1024*1024) {
	#ifdef DEBUG
		mprintk("*High memory avlb", 17, 'c');
		mprintk("*Memory Size:", 13, 'c');
		mprintk(&b_mem_mgr.mem_size, 4, 'd');
	#endif
		max_low_pfn = (896*1024*1024 >> 12) -1;
		high_start_pfn = (896*1024*1024 >> 12) ;
		high_end_pfn = (b_mem_mgr.mem_size) >> 12;
		 
	} else {
	#ifdef DEBUG
		mprintk("*High memory NOT avlb", 22, 'c');
	#endif
		/*
			No high memory available, so max_low_pfn is
		the last PFN available. high_start_pfn and high_end_pfn
		is zero as there is no high memory.
		*/
		max_low_pfn = (b_mem_mgr.mem_size >> 12);
		high_start_pfn = 0x0;
		high_end_pfn = 0x0;
	}
#ifdef DEBUG
	mprintk("*max_low_pfn:",13, 'c');
	mprintk(&max_low_pfn, 4, 'd');
	mprintk("*high_start_pfn:",16, 'c');
	mprintk(&high_start_pfn, 4, 'd');
	mprintk("*high_end_pfn:",14, 'c');
	mprintk(&high_end_pfn, 4, 'd');
#endif
	/* set up the zone_size array with the size of various zones */
	zone_sizes_init();
	/* initialize and setup node, zone fields */
	free_area_init_core(&contig_page_data, &mem_map, zone_sizes);
}

/*
*	Function: bootmem_bitset
*/

unsigned long bootmem_bitset(unsigned char* bitmap, int i)
{
	unsigned char byte = *(bitmap + i/8);
	unsigned char bit = i % 8;
	if (byte & (1 << bit))
		return 1;
	return 0;
}

inline void set_page_count(mem_map_t *page, unsigned int count)
{
	page->count = count;
}

inline int PageReserved(mem_map_t *page)
{
	if (page->flags & PAGE_RESERVED)
		return 1;
	return 0; /* page reserved flag not set */
}
inline void ClearPageReserved(mem_map_t *page)
{
	if (!(page->flags & PAGE_RESERVED)) {
		mprintk("*Bug, page not reserved earlier:", 31, 'c');
		while(1); 
	}
	/* clear the reserved bit */
	page->flags &=(~PAGE_RESERVED);
}

/*
*	Function : free_all_bootmem_core
*	Here, we traverse through the boot bitmap and based on
*	the bits set, mark the mem_map page entries as reserved
*	or free. Finally return the total number of free pages.
*/
unsigned long free_all_bootmem_core(meros_pg_data_t *pgdat)
{
	int i;
//	int debug=0;
	unsigned long total_free_page = 0x0;
	unsigned long total_reserved_page = 0x0;
	mem_map_t *page = mem_map;
	if (!b_mem_mgr.bitmap) {
		mprintk("*PANIC: No boot bitmap!", 23, 'c');
		while(1); 
	}
	/* traverse the bitmap, bitmap_size is in bytes, we will convert
	it to bits as each page represent a page and will check its state */
	for (i=0; i<b_mem_mgr.bitmap_size*8; i++, page++) {
		/* if the bit is set, the page is reserved */
		if (bootmem_bitset(b_mem_mgr.bitmap, i)) {
			total_reserved_page++;
		} else {
			//debug++;
			total_free_page++;
			/* clear the reserved flag for this page */
			ClearPageReserved(page);
			set_page_count(page, 1);
			/*FIXME: only for debugging, remove it */
//			if (debug <=10)
			__free_page(page);
			/* increment the free page count of appropriate
				zone */
			contig_page_data.meros_zones[page->my_zone].
							free_pages++;
		}
	}
#ifdef DEBUG
	mprintk("*Total Reserved Pages:", 22, 'c');
	mprintk(&total_reserved_page, 4, 'd'); 	
	mprintk("*Total Free  Pages:", 19, 'c');
	mprintk(&total_free_page, 4, 'd'); 	
	mprintk("*Free pages in ZONE_DMA:", 24, 'c');
	mprintk(&contig_page_data.meros_zones[ZONE_DMA].free_pages, 4, 'd'); 
	mprintk("*Free pages in ZONE_NORMAL:", 27, 'c'); 
	mprintk(&contig_page_data.meros_zones[ZONE_NORMAL].free_pages, 4, 'd'); 
	mprintk("*Free pages in ZONE_HIGHMEM:", 28, 'c'); 
	mprintk(&contig_page_data.meros_zones[ZONE_HIGHMEM].free_pages, 4, 'd'); 
#endif
#ifdef DEBUG
	{
	struct meros_page *pg;
	mprintk("*BEFORE", 7, 'c');
	analyse_free_area();
	pg = alloc_pages(GFP_KERNEL, 0);
	mprintk("*allocated pg:", 14, 'c');
	mprintk(&pg, 4, 'd'); 
	mprintk("*AFTER", 6, 'c');
	__free_pages(pg, 0);
	analyse_free_area();
	}
#endif
#ifdef DEBUG
	{
		unsigned long virtual;
		mprintk("*BEFORE", 7, 'c');
		analyse_free_area();
	
		virtual = Get_Free_Page(GFP_KERNEL);
		mprintk("*virtual:", 9, 'c');
		mprintk(&virtual, 4, 'd');
		Free_Pages(virtual, 0);
		mprintk("*AFTER", 6, 'c');
		analyse_free_area();
	}
#endif
	/* FIXME: Free the bootmem bitmap as it is no longer needed */
	return total_free_page;

}
/*
*	Function : free_pages_init
*		returns the total number of reserved pages.
*/

unsigned long free_pages_init(void)
{
	/* this will put all low memory in to free lists */
	total_ram_pages = free_all_bootmem_core(&contig_page_data);
}

/*
*	Function: mem_init
*	The buddy allocator builds its free list using the unallocated
*	pages.
*/

void mem_init(void)
{
	int reserved_pages = free_pages_init();
	
}
