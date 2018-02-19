#include <stdio.h>
#include <stdlib.h>

struct request {
	char *cmd;
};

typedef struct sg_request {
	struct request *rq;
} Sg_request;

Sg_request srp;

// should be replaced
void* vmalloc() {
	printf("do something\n");
	return malloc(42);
}

// should be replaced
void kfree(void *cmd) {
	printf("do something %p\n", cmd);
}

void add_rq(struct request *rq) {
	srp.rq = rq;
}

void use_rq() {
	struct request tmp_rq;
	*srp.rq = tmp_rq;
}

void sys_test() {
	struct request *rq = (struct request*)vmalloc();

	add_rq(rq);

	if(rq)
		kfree(rq);

	use_rq();
}

int main() {
	sys_test();
	return 0;
}
