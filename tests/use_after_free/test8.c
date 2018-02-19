#include <stdio.h>
#include <stdlib.h>

struct request {
	char *cmd;
};

typedef struct sg_request {
	struct request *rq;
} Sg_request;

Sg_request global_rq;


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
	global_rq.rq = rq;
}

void free_rq() {
	kfree(global_rq.rq);
	global_rq.rq = NULL;
}

void use_rq() {
	struct request tmp_rq;
	*global_rq.rq = tmp_rq;
}

void sys_test() {
	struct request *rq = (struct request*)vmalloc();

	add_rq(rq);

	if(global_rq.rq)
		free_rq();

	use_rq();
}

int main() {
	sys_test();
	return 0;
}
