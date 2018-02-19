#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct list {
	struct list *Next;
	int Data;	
} list;

int Global = 10;

void do_all(list *L, void (*FP)(int*)) {
	do {
		FP(&L->Data);
		L = L->Next;
	} while(L);
}

void addG(int *X) { 
	(*X) += Global; 
}

void addGToList(list *L) {
	do_all(L, addG);
}

list *makeList(int Num) {
	list *New = malloc(sizeof(list));
	New->Next = Num ? makeList(Num-1) : 0;
	New->Data = Num;
	return New;
}

void do_test() {
	list *X = makeList(10);
	list *Y = makeList(100);
	addGToList(X);
	Global = 20;
	addGToList(Y);
}

void sys_test() {
	do_test();
}

int main() {
	sys_test();
	return 0;
} 
