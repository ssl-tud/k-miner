#include <stdio.h>
#include <stdlib.h>

struct request {
	char *cmd1;
	char *cmd2;
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

void remove_cmds(struct request *rq) {
	kfree(rq->cmd1);
	kfree(rq->cmd2);
	rq->cmd1 = NULL;
	rq->cmd2 = NULL;
}

void blk_end_request_all(struct request *rq) {
	// Skip blk_end_bidi_request
	// SKip blk_finish_request(rq);
	// Skip __blk_put_request 
	int cond;
	
	if(cond) 
		remove_cmds(rq);
}

void sg_finish_rem_req(Sg_request *srp) {
	int cond;

	if(cond) {
		remove_cmds(srp->rq);
	}

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

Sg_request* add_request() {
	Sg_request *srp = (Sg_request*) vmalloc();
	srp->rq = (struct request*) vmalloc();
	srp->rq->cmd1 = (char*) vmalloc();
	srp->rq->cmd2 = (char*) vmalloc();
	return srp;
}

int sg_new_write(Sg_request **o_srp) {
	Sg_request *srp;
	srp = add_request();

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
