#include <stdio.h>
#include <stdlib.h>

// should be replaced
void* vmalloc() {
	printf("do something\n");
	return malloc(42);
}

// should be replaced
void kfree(void *cmd) {
	printf("do something %p\n", cmd);
}

void sys_test() {
	int *p = (int*)vmalloc();

	kfree(p);
	kfree(p);
}

int main() {
	sys_test();
	return 0;
}
