#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *global_p;

void foo(int *param) {
	int local_x = 1;
	param = &local_x;
}

void do_test() {
	int* local_p;
	foo(global_p);	
	global_p = local_p;
}

void sys_test() {
	do_test();
}

int main() {
	sys_test();
	return 0;
} 
