#include <stdio.h>
#include <stdlib.h>

struct request {
	unsigned char *cmd;
};

typedef struct sg_request {
	struct request *rq;
} Sg_request;

// should be replaced
void* vmalloc() {
	printf("do something\n");
	return malloc(42);
}

// should be replaced
void kfree(void *cmd) {
	printf("do something %p\n", cmd);
}

// should be replaced
void blk_mq_free_request(struct request *rq) {
	printf("do something %p\n", rq);
}

void blk_end_request_all(struct request *rq) {
	int cond;
	
	if(cond)
		blk_mq_free_request(rq);	
}

void sg_finish_rem_req(Sg_request *srp) {
	int cond;

	if(cond)
		kfree(srp->rq->cmd);

	blk_mq_free_request(srp->rq);	
}

int sg_common_write(Sg_request *srp) {
	int cond, cond2;
	
	if(cond) {
		if(cond2)
			blk_end_request_all(srp->rq);

		sg_finish_rem_req(srp);
		return 1;
	}

	return 0;
}

Sg_request* sg_add_request() {
	return (Sg_request*) vmalloc();
}

int sg_new_write(Sg_request **o_srp) {
	Sg_request *srp;
	srp = sg_add_request();

	srp->rq = vmalloc();
	srp->rq->cmd = vmalloc();

	int k = sg_common_write(srp);	

	return k;
}

void sg_ioctl() {
	Sg_request *srp;

	int result = sg_new_write(&srp);
}

void sys_test() {
	sg_ioctl();
}

int main() {
	sys_test();
	return 0;
}
