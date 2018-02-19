#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *global_p;
int global_x;

void foo3() {
	int local_z;
	global_p = &local_z;
}

void foo2() {
	int local_y;
	global_p = &local_y;
	foo3();
}

void foo1(int *p) {
	global_p = p;
	foo2();
}

void sys_test() {
	int local_x;
	foo1(&local_x);
}

int main() {
	sys_test();
	return 0;
} 
