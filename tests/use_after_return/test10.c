#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *global_p;
int *global_p2;

struct vec2D {
	int *x;
	int *y;
};

void init(struct vec2D *v, int *x, int *y) {
	v->x = x;
	v->y = y;
}

void del_x(struct vec2D *v) {
	if(v->y == NULL)
		printf("\n");		
	v->x = NULL;
}

void foo(struct vec2D *v) {
	int *x = v->x;
	int *y = v->y;
}

void do_test() {
	struct vec2D local_v;
	int x = 1;
	int y = 3;
	init(&local_v, &x, &y);
	del_x(&local_v);
	foo(&local_v);
}

void sys_test() {
	do_test();
}

int main() {
	sys_test();
	return 0;
} 
