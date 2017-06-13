/*
	mer_mm.h
*/
#define LONG_ALIGN(x)	(((x) + (3))& ~0x3)

#define MAX_NR_ZONES	3

#define PAGE_RESERVED	1 /* 0th bit set to '1' indicates page reserved */
#define MAX_DMA_ADDRESS 0xc0000000 + 0x1000000 

/* allocation flags */
#define GFP_KERNEL	0
#define GFP_BUFFER	1
#define GFP_ATOMIC	2
#define GFP_USER	3
#define GFP_HIGHUSER	4 /* allocates from high memory(ZONE_HIGHMEM) if any*/
#define GFP_DMA		5 /* allocates from ZONE_DMA*/
#define GFP_HIGHMEM	6 /* allocates from ZONE_HIGHMEM */

/* free_area, the array of structures used by buddy allocator */
#define MAX_ORDER	10
typedef struct free_area_struct {
	void		*next;
	void		*prev;
	unsigned long	*bitmap;	
	unsigned long 	bitmap_size; /* this is not there in Linux */
}free_area_t;

/*Page*/
typedef struct meros_page {
	struct meros_page *prev;
	struct meros_page *next;
	unsigned long flags;
	void *virtual;
	unsigned int count; /* page's reference count, if zero then page
				can be freed. */
	unsigned int my_zone;
}mem_map_t;

/* Zones */
typedef struct meros_zone_struct {
	unsigned long free_pages; /* total number of free pages in this zone*/
	unsigned long pages_min, pages_low, pages_high; /*Zone water marks */
	int need_balance;
	// free_area_t free_area[MAX_ORDER]; /* used by buddy allocator */
	//meors_pg_data_t *zone_pgdat; /* pointer to node that owns this zone*/	
	free_area_t	free_area[MAX_ORDER];
	struct page* zone_mem_map;
	unsigned long zone_start_paddr;
	unsigned long zone_start_mapnr; 
	char *name;
	unsigned long size; /* total number of pages in each zone */
	
}meros_zone_t;

/* Node */
typedef struct meros_pglist_data {
	meros_zone_t meros_zones[MAX_NR_ZONES]; /* static arrays rather
		than dynamic allocations as we initially have only page
		wise memory allocator */
	int nr_zones;
	struct meros_page* node_mem_map;
	unsigned long node_size; /* in pages */
}meros_pg_data_t;


#define ZONE_DMA	0
#define ZONE_NORMAL	1
#define ZONE_HIGHMEM	2
unsigned long zone_sizes[MAX_NR_ZONES];
unsigned long max_low_pfn; /* Last page frame of normal memory, if total
				physical memory is > 896MB then
			 	max_low_pfn= 896MB 
			   */	
unsigned long num_mapped_pages;
unsigned long num_physpages;
unsigned long max_mapnr;
unsigned long high_start_pfn;
unsigned long high_end_pfn;
unsigned long total_ram_pages;

#define __free_page(page) __free_pages((page), 0)
#define Get_Free_Page(flag) Get_Free_Pages(0, flag)
