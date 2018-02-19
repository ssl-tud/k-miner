#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *global_p;

void foo() {
	int local_x = 1;
	global_p = &local_x;
	global_p = NULL;
}

void do_test() {
	foo();
}

void sys_test() {
	do_test();
}

int main() {
	sys_test();
	return 0;
} 
