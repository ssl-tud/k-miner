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

void sys_test() {
	spinlock_t logbuf_lock;

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

int main() {
	sys_test();
	return 0;
}
