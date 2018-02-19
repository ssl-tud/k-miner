#include<stdio.h>
#include<stdlib.h>

typedef struct raw_spinlock {
	int lock;
} raw_spinlock_t;

typedef struct spinlock {
	struct raw_spinlock rlock;
} spinlock_t;

struct inode {
	spinlock_t i_lock;
};

void* vmalloc(size_t size) {
	printf("\n");
	return malloc(42);
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

void sys_test() {
	struct inode *inode_ptr;
	struct inode my_inode;
	inode_ptr = &my_inode;

	spin_lock(&inode_ptr->i_lock);
	spin_lock(&inode_ptr->i_lock);
}

int main() {
	sys_test();
	return 0;
}
