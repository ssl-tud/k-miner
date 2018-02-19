#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vec2D {
	int *x;
	int *y;
};

struct vec2D *myvector_global_p;
int global_x= 1;

void foo(struct vec2D *myvector_param, int *param) {
	myvector_param->x = param;
}

struct vec2D* init() {
	return (struct vec2D*)malloc(sizeof(struct vec2D));
}

void finish() {
	myvector_global_p->x = &global_x;
}

void do_test() {
	int local_x = 1;
	myvector_global_p = init();
	foo(myvector_global_p, &local_x);
	finish();
}

void sys_test() {
	do_test();
}

int main() {
	sys_test();
	return 0;
} 
