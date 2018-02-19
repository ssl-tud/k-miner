#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({ 				\
		const typeof( ((type *)0)->member ) *_mptr = (ptr); 	\
		(type *)( (char *)_mptr - offsetof(type, member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

// ptr = *list_head (futex_hash_bucket->chain->node_list)
// type = struct futex_q
// member = list_head (futex_q.list.node_list)
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

// pos = *futex_q (this)
// member = list_head (futex_q.list.node_list)
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)


// pos = *futex_q (this)
// n = *futex_q (next)
// head = *list_head (futex_hash_bucket->chain->node_list)
// member = list_head (futex_q.list.node_list)
#define list_for_each_entry_safe(pos, n, head, member) \
	for(pos = list_first_entry(head, typeof(*pos), member), \
	    n = list_next_entry(pos,member); \
	    &pos->member != (head); \
	    pos = n, n = list_next_entry(n, member))

// pos = *futex_q (this)
// n = *futex_q (next)
// head = *plist_head (futex_hash_bucket->chain)
// m = plist_node (futex_q.list)
#define plist_for_each_entry_safe(pos, n, head, m) \
	list_for_each_entry_safe(pos, n, &(head)->node_list, m.node_list)

//struct list{
//	struct list *next, *prev;
//};

struct list_head {
	struct list_head *next, *prev;
};

struct plist_head {
	struct list_head node_list;
};

struct plist_node {
	int prio;
	struct list_head prio_list;
	struct list_head node_list;
};


int list_empty(const struct list_head *head);

void INIT_LIST_HEAD(struct list_head *list);

void plist_head_init(struct plist_head *head);

void plist_node_init(struct plist_node *node, int prio);

struct plist_node *plist_first(const struct plist_head *head);

void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next);

void __list_del(struct list_head *prev, struct list_head *next);

void list_del_init(struct list_head *entry);

void list_add_tail(struct list_head *new, struct list_head *head);

void plist_add(struct plist_node *node, struct plist_head *head);

void plist_del(struct plist_node *node, struct plist_head *head);

#endif // LIST_H_
