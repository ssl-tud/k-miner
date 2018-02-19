#include "list.h"

int list_empty(const struct list_head *head) {
	return head->next == head;
}

void INIT_LIST_HEAD(struct list_head *list) {
	list->next = list;
	list->prev = list;
}

void plist_head_init(struct plist_head *head) {
	INIT_LIST_HEAD(&head->node_list);
}

void plist_node_init(struct plist_node *node, int prio) {
	node->prio = prio;
	INIT_LIST_HEAD(&node->prio_list);
	INIT_LIST_HEAD(&node->node_list);
}

void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
	printf("__list_add\n");
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void __list_del(struct list_head *prev, struct list_head *next) {
	next->prev = prev;
	prev->next = next;
}

void list_del_init(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

void list_add_tail(struct list_head *new, struct list_head *head) {
	__list_add(new, head->prev, head);
}

struct plist_node *plist_first(const struct plist_head *head) {
	return list_entry(head->node_list.next, struct plist_node, node_list);
}

void plist_add(struct plist_node *node, struct plist_head *head) {
	printf("plist_add\n");
	struct plist_node *first, *iter, *prev = NULL;
	struct list_head *node_next = &head->node_list;
		
	if(list_empty(&head->node_list))
		goto ins_node;
	
	first = plist_first(head);

	do {
		if(node->prio < iter->prio) {
			node_next = &iter->node_list;
			break;
		}

		prev = iter;
		iter = list_entry(iter->prio_list.next, struct plist_node, prio_list);
	} while (iter != first);

	if(!prev || prev->prio != node->prio)
		list_add_tail(&node->prio_list, &iter->prio_list);

ins_node:
	list_add_tail(&node->node_list, node_next);
}

void plist_del(struct plist_node *node, struct plist_head *head) {
	if(!list_empty(&node->prio_list)) {
		if(node->node_list.next != &head->node_list) {
			struct plist_node *next;

			next = list_entry(node->node_list.next, 
					struct plist_node, node_list);
			
			if(list_empty(&next->prio_list))
				__list_add(&next->prio_list, &node->prio_list, 
					(&node->prio_list)->next);
		}
		list_del_init(&node->prio_list);
	}
	list_del_init(&node->node_list);
}
