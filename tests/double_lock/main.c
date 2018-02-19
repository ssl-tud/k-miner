#include<stdio.h>

typedef struct raw_spinlock {
	int lock;
} raw_spinlock_t;

typedef struct spinlock {
	struct raw_spinlock rlock;
} spinlock_t;

struct inode {
	int tmp_id;
	spinlock_t i_lock;
	spinlock_t i_lock2;
};

void* vmalloc(size_t size) {
	printf("\n");
}

static inline void spin_lock(spinlock_t *lock) {
	printf("lock\n");
}

static inline void spin_unlock(spinlock_t *lock) {
	printf("unlock\n");
}

static inline void raw_spin_lock(spinlock_t *lock) {
	printf("lock\n");
}

static inline void raw_spin_unlock(spinlock_t *lock) {
	printf("unlock\n");
}

static inline void raw_spin_lock_irqrestore(spinlock_t *lock) {
	printf("lock\n");
}

static inline void raw_spin_unlock_irqrestore(spinlock_t *lock) {
	printf("unlock\n");
}

static inline void raw_spin_lock_irqsave(spinlock_t *lock) {
	printf("lock\n");
}

static inline void raw_spin_unlock_irqsave(spinlock_t *lock) {
	printf("unlock\n");
}

struct container_node {
	struct inode my_inode1; // 61, 59, 60, 62, 63, 64, 59, 60, 65, 66
	spinlock_t i_lock; // 58, 67, 68
};

//void test1() {
//	struct inode *inode_ptr;
//	struct inode my_inode;
//	inode_ptr = &my_inode;
//
//	add_dquot_ref(inode_ptr);
//}

//void inode_get_rsv_space(struct inode *inode){
//	if(inode)
//		return;
//	spin_lock(&inode->i_lock);
//	spin_unlock(&inode->i_lock);
//}
//
//void add_dquot_ref(struct inode *inode) {
//	spin_lock(&inode->i_lock);
//
//	if(inode) {
//		spin_unlock(&inode->i_lock);
//		return;
//	}
//
//	inode_get_rsv_space(inode);
//	spin_unlock(&inode->i_lock);
//}
//
//void add_dquot_ref2(struct inode *inode) {
//	spin_lock(&inode->i_lock);
//
//	if(inode) {
//		spin_unlock(&inode->i_lock);
//		return;
//	}
//
//	inode_get_rsv_space(inode);
//	spin_unlock(&inode->i_lock);
//}
//
//
//void test2() {
//	struct container_node node;
//
//	add_dquot_ref(&node.my_inode1);
//	spin_lock(&node.i_lock);
//	spin_lock(&node.i_lock);
//	spin_unlock(&node.i_lock);
//	add_dquot_ref2(&node.my_inode1);
//}

//void setup_hb(struct inode **p) {
//	*p = (struct inode*)vmalloc(sizeof(struct inode));
//}
//
//void test3() {
//	struct inode *hb;
//	setup_hb(&hb);
//	spin_lock(&hb->i_lock);
////	spin_unlock(&hb->i_lock);
//	spin_lock(&hb->i_lock);
//}

//void foo(spinlock_t *lock) {
//	spin_unlock(lock);
//}
//
//void test4() {
//	spinlock_t *lock_ptr;
//
//	spin_lock(lock_ptr);
//	spin_lock(lock_ptr);
//	foo(lock_ptr);
//}

//void unqueue_me(struct container_node *c) {
//	spinlock_t *lock_ptr;
//retry:
//	lock_ptr = &c->i_lock;
//	spin_lock(lock_ptr);
//	if(lock_ptr) {
//		spin_unlock(lock_ptr);
//		goto retry;
//	}
//	spin_lock(lock_ptr);
//}
//
//void test5() {
//	struct container_node c;
//	unqueue_me(&c);
//}

//void bar(struct container_node *c) {
//	printf("%p\n", c);
//}
//
//void test6() {
//	int *cond;
//	struct container_node c;
//
//	if(cond) {
//		spin_lock(&c.i_lock);
//		bar(&c);
//		spin_unlock(&c.i_lock);
//	} else {
//		spin_lock(&c.i_lock);
//	}
//
//}

//struct top_container {
//	struct container_node *c;
//};
//
//void isp1362_hc_stop(struct top_container *tc) {
//	spin_lock(&tc->c->i_lock);
//}
//
//void test7() {
//	struct top_container *tc = vmalloc(sizeof(struct top_container));
//	tc->c = vmalloc(sizeof(struct container_node));
//	isp1362_hc_stop(tc);
//	spin_lock(&tc->c->i_lock);
//	spin_unlock(&tc->c->i_lock);
//}
spinlock_t logbuf_lock;

void console_unlock() {
	int *c;
again:
	for(;;) {
		raw_spin_lock_irqsave(&logbuf_lock);
		if(c)
			break;
		raw_spin_unlock(&logbuf_lock);
	}

	raw_spin_unlock(&logbuf_lock);
	raw_spin_lock(&logbuf_lock);

	if(c)
		goto again;

	raw_spin_unlock_irqrestore(&logbuf_lock);
}

void test8() {
	console_unlock();
}

void sys_futex() {
	test8();
}

int main() {
	sys_futex();
	return 0;
}
