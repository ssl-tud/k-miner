#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *global_p;

void bar() {
	int *local_p;
	int local_y;
	local_p = global_p;

	if(global_p) 
		global_p = &local_y;
}

void foo() {
	int local_x = 1;
	if(local_x) {
		global_p = &local_x;
	} else {
		bar();
	}
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
