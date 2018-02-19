#include "list.h"
#include <stdio.h>
#include <stdlib.h>

struct rt_mutex_waiter {
	int prio;
};

union futex_key {
	int key;
};

struct futex_q {
	struct plist_node list;
	struct rt_mutex_waiter *rt_waiter;
	union futex_key *requeue_pi_key;
	union futex_key key;
};

struct futex_hash_bucket {
	struct plist_head chain;
};

struct hrtimer {
	long state;
};

struct hrtimer_sleeper {
	struct hrtimer timer;
};

struct futex_hash_bucket *futex_queues;
struct rt_mutex_waiter *global_waiter;

void *__vmalloc_node() { 
	return malloc(1);
}

void *__vmalloc(int size);

void *__vmalloc(int size) {
	return __vmalloc_node();
}

void kfree(void *p) {
	free(p);	
}

void print_list(struct plist_head *head) {
	struct futex_q *this, *next;

	printf("List: ");
	plist_for_each_entry_safe(this, next, head, list) {
		printf("%d, ", this->rt_waiter->prio);	
	}
	printf("\n");
}

void buggy_func(struct plist_head *head) {
	printf("buggy function\n");
	struct futex_q *this, *next;
	
	plist_for_each_entry_safe(this, next, head, list) {
		global_waiter = this->rt_waiter;
	}
}

void __schedule() {
	// kernel schedule	
	printf("__schedule\n");
	return;
}

void freezable_schedule() {
	printf("freezable_schedule\n");
	__schedule();
}

void queue_me(struct futex_q *q, struct futex_hash_bucket *hb) {
	printf("queue_me\n");
	int prio = 1;
	plist_node_init(&q->list, prio);
	plist_add(&q->list, &hb->chain);		
}

void unqueue_me(struct futex_q *q, struct futex_hash_bucket *hb) {
	plist_del(&q->list, &hb->chain);	
}

void unqueue_me_pi(struct futex_q *q) {
	return;
}

int hrtimer_start_range_ns(struct hrtimer *timer) {
	return 0;	
}

static inline int hrtimer_start_expires(struct hrtimer *timer) {
	return hrtimer_start_range_ns(timer);
}

void futex_wait_queue_me(struct futex_hash_bucket *hb, struct futex_q *q, struct hrtimer_sleeper *timeout) {
	printf("futex_wait_queue_me\n");
	queue_me(q, hb);
	hrtimer_start_expires(&timeout->timer);
	freezable_schedule();
}

//void tmp() {
//	struct futex_hash_bucket *hb2 = &futex_queues;
//	struct futex_q q2;
//	futex_wait_queue_me(hb2, &q2);
//}

struct futex_hash_bucket* hash_futex() {
	return &futex_queues[1];
}

struct futex_hash_bucket* queue_lock() {
	struct futex_hash_bucket *hb;

	hb = hash_futex();

	return hb;
}

void futex_wait_setup(struct futex_hash_bucket **hb) {
	*hb = queue_lock();
}

static const struct futex_q futex_q_init = {
	.key = (union futex_key) { .key = 100 }	
};

void futex_wait_requeue_pi() {
	printf("futex_wait_requeue_pi\n");
	struct rt_mutex_waiter rt_waiter;
//	struct futex_hash_bucket *hb = &futex_queues;
	struct futex_hash_bucket *hb;
	union futex_key key2 = (union futex_key) { .key = 100 };
//	struct futex_hash_bucket *hb2 = hb;
	struct futex_q q = futex_q_init;
	struct hrtimer_sleeper timeout, *to = NULL;

	to = &timeout;

//	struct futex_hash_bucket **hb2 = &hb;
	futex_wait_setup(&hb);

	plist_head_init(&hb->chain);

	rt_waiter.prio = 19;
	q.requeue_pi_key = &key2;
//	q.rt_waiter = &rt_waiter;

//	print_list(&hb->chain);

	futex_wait_queue_me(hb, &q, to);

//	buggy_func(&hb->chain);
//	print_list(&hb->chain);
//	
	if(q.rt_waiter) {
		global_waiter = NULL;
		unqueue_me_pi(&q);
	}

//	print_list(&hb->chain);
}

int init_hb(void) {
	int *i;
//	futex_queues = (struct futex_hash_bucket*)malloc(10*sizeof(struct futex_hash_bucket));	
	futex_queues = (struct futex_hash_bucket*)__vmalloc(10*sizeof(struct futex_hash_bucket));
	for(int i=0; i < 10; i++)
		plist_head_init(&futex_queues[i].chain);
	return 0;
}

//void tmp() {
//	init_hb();
//}

void sys_futex() {
	printf("----------------- futex system call -----------------\n");
	init_hb();
	futex_wait_requeue_pi();
	kfree(futex_queues);

}

int main() {
	sys_futex();
	return 0;
}
