/*
	mer_list.h
*/

typedef struct list_head_t {
	void *next;
	void *prev;
}list_head;
inline void list_add(list_head *node,  list_head *head)
{
	node->next = head->next;
	node->prev = head;
	if (head->next != NULL)
		((list_head*)head->next)->prev = node;
	head->next = node;
	
}
inline void list_del(list_head *node)
{
	if (node->next)
		((list_head*)node->next)->prev = node->prev;
	if (node->prev)
		((list_head*)node->prev)->next = node->next;

	node->next = NULL;
	node->prev = NULL;
}

