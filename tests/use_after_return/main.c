#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

/// Test1 -------------------
//int *global_p;
//void test() {
//	int local_x = 1;
//	global_p = &local_x;
//	global_p = NULL;
//}
// Result: No Bug
/// END Test1 -------------------

/// Test2 -------------------
//int *global_p;
//int *global_p2;
//
//void foo() {
//	global_p2 = global_p;
//	global_p = NULL;
//}
//
//void foo2() {
//	foo();
//}
//
//void test() {
//	int local_x = 1;
//	global_p = &local_x;
//	foo2();
//}
// Result: global_p2 -> local_x 
/// END Test2 -------------------

/// Test3 -------------------
//int *global_p;
//int **global_p2;
//void foo(int *param) {
//	global_p2 = &global_p;
//	*global_p2 = param;
//	global_p2 = NULL;
//}
//
//void test() {
//	int local_x = 1;
//	foo(&local_x);
//}
// Result: global_p -> local_x   
/// END Test3 -------------------

/// Test4 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D myvector_global;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//void test() {
//	int local_x = 1;
//	foo(&myvector_global, &local_x);
//}
// Result: myvector_global -> local_x
/// END Test4 -------------------

/// Test5 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D myvector_global;
//struct vec2D *myvector_global_p;
//
//void foo(int *param) {
//	myvector_global_p->x = param;
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = &myvector_global;
//	foo(&local_x);
//}
// Result: myvector_global -> local_x
/// END Test5 -------------------

/// Test6 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D *myvector_global_p;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//struct vec2D* init() {
//	return (struct vec2D*)malloc(sizeof(struct vec2D));
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = init();
//	foo(myvector_global_p, &local_x);
//	free(myvector_global_p);
//}
// Result: No Bug 
/// END Test6 -------------------

/// Test7 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D *myvector_global_p;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//struct vec2D* init() {
//	return (struct vec2D*)malloc(sizeof(struct vec2D));
//}
//
//void finish() {
//	if(myvector_global_p != NULL)
//		free(myvector_global_p);
//	myvector_global_p = NULL;
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = init();
//	foo(myvector_global_p, &local_x);
//	finish();
//}
// Result: No Bug
/// END Test7 -------------------

/// Test8 -------------------
//union unionType {
//	int x;
//	double y;
//};
//
//void foo(int **param) {
//	int local_x = 1;
//	*param = &local_x;
//}
//
//void test() {
//	int* local_p;
//	union unionType myunion;
//	union unionType myunion2;
//	myunion = myunion2;
//	foo(&local_p);	
//}
// Result: local_p -> local_x
/// END Test8 -------------------

/// Test9 -------------------
//void foo(int **param) {
//	int local_x = 1;
//	*param = &local_x;
//}
//
//int* init() {
//	int *p;
//	p = (int*)malloc(sizeof(int));
//	return p;
//}
//
//void test() {
//	int* local_p = init();
//	foo(&local_p);	
//}
// Result: local_p -> local_x
/// END Test9 -------------------

/// Test10 -------------------
//int *global_p;
//
//void foo(int *param) {
//	int local_x = 1;
//	param = &local_x;
//}
//
//void test() {
//	int* local_p;
//	foo(global_p);	
//	global_p = local_p;
//}
// Result: No Bug
/// END Test10 -------------------

/// Test11 -------------------
//int *global_p;
//
//void bar() {
//	int *local_p;
//	local_p = global_p;
//
//	if(global_p) 
//		global_p = NULL;
//}
//
//void foo() {
//	int local_x = 1;
//	global_p = &local_x;
//	bar();
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_x 
/// END Test11 -------------------

/// Test12 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D *myvector_global_p;
//int global_x= 1;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//struct vec2D* init() {
//	return (struct vec2D*)malloc(sizeof(struct vec2D));
//}
//
//void finish() {
//	myvector_global_p->x = &global_x;
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = init();
//	foo(myvector_global_p, &local_x);
//	finish();
//}
// Result: No Bug
/// END Test12 -------------------

/// Test13 -------------------
//int *global_p;
//int **global_p2;
//int ***global_p3;
//int ****global_p4;
//void foo(int *param) {
//	global_p2 = &global_p;
//	global_p3 = &global_p2;
//	global_p4 = &global_p3;
//	***global_p4 = param;
//	global_p2 = NULL;
//}
//
//void test() {
//	int local_x = 1;
//	foo(&local_x);
//}
// Result: global_p -> local_x   
/// END Test13 -------------------

/// Test14 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D *myvector_global_p;
//struct vec2D myvector;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//struct vec2D* init() {
//	return (struct vec2D*)malloc(sizeof(struct vec2D));
//}
//
//void finish() {
//	if(myvector_global_p != NULL)
//		free(myvector_global_p);
//	myvector_global_p = &myvector;
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = init();
//	foo(myvector_global_p, &local_x);
//	finish();
//}
// Result: No Bug
/// END Test14 -------------------

/// Test15 -------------------
//int *global_p;
//int *global_pp;
//int **global_p2;
//int ***global_p3;
//int ****global_p4;
//void foo(int *param) {
//	global_p2 = &global_p;
//	global_p3 = &global_p2;
//	global_p4 = &global_p3;
//	***global_p4 = param;
//	global_pp = ***global_p4;
//	global_p2 = NULL;
//}
//
//void test() {
//	int local_x = 1;
//	foo(&local_x);
//}
// Result: global_p -> local_x   
// 	 : global_pp -> local_x   
// Info  : Is node 27 guaranteed to be visited? (merging)
/// END Test15 -------------------

/// Test16 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//struct vec2D *myvector_global_p;
//struct vec2D myvector;
//struct vec2D myvector2;
//
//void foo(struct vec2D *myvector_param, int *param) {
//	myvector_param->x = param;
//}
//
//struct vec2D* init() {
//	return (struct vec2D*)malloc(sizeof(struct vec2D));
//}
//
//void finish() {
//	if(myvector_global_p != NULL)
//		free(myvector_global_p);
//	myvector = myvector2;
//}
//
//void test() {
//	int local_x = 1;
//	myvector_global_p = init();
//	foo(myvector_global_p, &local_x);
//	myvector2.x = myvector_global_p->x;
//	finish();
//}
// Result: myvector -> local_x 
//  	 : myvector2 -> local_x 
//  	 : call -> local_x 
/// END Test16 -------------------

/// Test17 -------------------
//int *global_p;
//int *global_p2;
//int *global_p3;
//
//void bar() {
//	global_p3 = global_p2;
//}
//
//void foo(int *param) {
//	global_p = param;
//	global_p2 = global_p;
//	bar();
//}
//
//void test() {
//	int local_x = 1;
//	foo(&local_x);
//}
// Result: global_p2 -> local->x
// 	 : global_p3 -> local->x
//  	 : global_p -> local->x
/// END Test17 -------------------

/// Test18 -------------------
//int **global_pp;
//
//void *__vmalloc_node() { 
//	return malloc(1);
//}
//
//void *__vmalloc(int size);
//
//void *__vmalloc(int size) {
//	return __vmalloc_node();
//}
//
//void foo(int *param) {
//	global_pp = (int**)__vmalloc(1);
//	global_pp[0] = param;
//}
//
//void test() {
//	int local_x = 1;
//	foo(&local_x);
//}
// Result: call -> local->x
/// END Test18 -------------------

/// Test18 -------------------
//int **global_pp;
//
//struct time {
//	int h;
//};
//
//struct timer {
//	struct time t;
//};
//
//void *__vmalloc_node() { 
//	return malloc(1);
//}
//
//void *__vmalloc(int size);
//
//void *__vmalloc(int size) {
//	return __vmalloc_node();
//}
//
//void bar(int *param, int **param2) {
//	*param2 = param;
//}
//
//void bar2(struct time *param_time) {
//	param_time->h = 1;	
//}
//
//void foo(int *param, int **param2, struct timer *param_timer) {
//	bar(param, param2);
//	bar2(&param_timer->t);
//}
//
//void test() {
//	global_pp = (int**)__vmalloc(1);
//	int local_x = 1;
//	int **local_pp = global_pp;
//	struct timer local_timer;
//	foo(&local_x, local_pp, &local_timer);
//}
// Result: call -> local_x 
/// END Test18 -------------------

/// Test19 -------------------
//int *global_p;
//
//void bar() {
//	int *local_p;
//	int local_y;
//	local_p = global_p;
//
//	if(global_p) 
//		global_p = &local_y;
//}
//
//void foo() {
//	int local_x = 1;
//	if(local_x) {
//		global_p = &local_x;
//	} else {
//		bar();
//	}
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_y 
//       : global_p -> local_x 
/// END Test19 -------------------

/// Test20 -------------------
//int *global_p;
//
//void foo();
//void fooo();
//
//void bar(int *param_x) {
//	int *local_p;
//	int local_y;
//	local_p = global_p;
//
//	int i;
//	for(i=0; i < 10; i++) {
//		if(global_p) 
//			global_p = &local_y;
//		else
//			global_p = param_x;
//	}
//
//	fooo();
//}
//
//void fooo() {
//	foo();
//}
//
//void foo() {
//	int local_x = 1;
//	if(local_x) {
//		bar(&local_x);
//	}
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_y
// 	 : global_p -> local_x 
/// END Test20 -------------------

/// Test21 -------------------
//int *global_p;
//
//void foo();
//
//void bar() {
//	if(global_p)
//		foo();
//}
//
//void foo() {
//	int local_x = 1;
//	if(local_x) {
//		global_p = &local_x;
//		bar();
//	}
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_x 
/// END Test21 -------------------

/// Test22 -------------------
//int *global_p;
//int *global_p2;
//
//void bar(int *param) {
//	int i=0;
//	int local_y = 1;
//
//	for(; i < 10; ++i) {
//		if(global_p2)
//			global_p = global_p2;
//
//		if(param)
//			global_p = param;
//		else if(global_p)
//			global_p2 = &local_y;
//		
//	}
//}
//
//void foo() {
//	int local_x = 1;
//	int local_z = 1;
//	bar(&local_x);
//	bar(&local_z);
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_x 
// 	 : global_p -> local_y 
//  	 : global_p -> local_z 
//  	 : global_p2 -> local_y 
/// END Test22 -------------------

/// Test23 -------------------
//int *global_p;
//int *global_p2;
//
//void foo();
//
//void anotherFunc(int *param) {
//	if(global_p)
//		foo();
//}
//
//void bar(int *param) {
//	int i=0;
//	int local_y = 1;
//
//	for(;;){
//		if(global_p2) {
//			global_p = global_p2;
//			break;
//		}
//
//		if(param)
//			global_p = param;
//		else if(global_p)
//			global_p2 = &local_y;
//		
//		anotherFunc(param);		
//	}
//}
//
//void foo() {
//	int local_x = 1;
//	int local_z = 1;
//	bar(&local_x);
//	bar(&local_z);
//}
//
//void test() {
//	foo();	
//}
// Result: global_p -> local_y 
//  	 : global_p2 -> local_y 
/// END Test23 -------------------

/// Test24 -------------------
//struct vec {
//	int *x;
//	int *y;
//} *global_v;
//
//void test(struct vec *v) {
//	int local_x = 1;
//	v->x = &local_x;
//}
//
//void sys_futex(struct vec *v) {
//	global_v = v;
//	test(v);
//}
// Result: No Bug 
/// END Test24 -------------------

/// Test24 -------------------
//struct vec {
//	int *x;
//	int *y;
//} global_v;
//
//void tmp(int *x) {
//	printf("%p", x);
//}
//
//void test(struct vec *v) {
//	int local_x = 1;
//	v->x = &local_x;
//	tmp(v->x);
//}
//
//void sys_futex(struct vec *v) {
//	v = &global_v;
//	tmp(v->x);
//}
// Result: No Bug (both v-parameter should be replaced
// 	   by a single global pointer of the same 
// 	   structure.)
// Info	 : In ssca.cpp set syscallNameGroup={"sys_futex", "test")
/// END Test24 -------------------

/// Test25 -------------------
//struct vec {
//	int *x;
//	int *y;
//} global_v, *global_p;
//
//void tmp(int *x) {
//	printf("%p", x);
//}
//
//void test() {
//	int local_x = 1;
//	global_v.x = &local_x;
//	tmp(global_v.x);
//}
// Result: global_v -> local_x
/// END Test25 -------------------

/// Test26 -------------------
//struct vec {
//	int *x;
//	int *y;
//};
//
//struct vec *global_p; 
//struct vec *global_p2;
//
//void *vmalloc() { 
//	return malloc(1);
//}
//
//void swapVec(struct vec *v1, struct vec *v2) {
//	int *tmp;
//	tmp = v1->x;
//	v1->x = v2->x;
//	v2->x = tmp;
//	tmp = v1->y;
//	v1->y = v2->y;
//	v2->y = tmp;
//}
//
//struct vec* initVec(int *x, int *y) {
//	struct vec* tmpVec = (struct vec*)malloc(sizeof(struct vec));// (struct vec*)vmalloc();
//	tmpVec->x = x;
//	tmpVec->y = y;
//	return tmpVec;
//}
//
//void deleteVec(struct vec* v) {
//	v->x = NULL;	
//	v->y = NULL;	
//}
//
//void test() {
//	int local_x1 = 1;
//	int local_y1 = 2;
//	int local_x2 = 3;
//	int local_y2 = 4;
//
//	global_p = initVec(&local_x1, &local_y1);
////	printf("global_p x=%d y=%d\n", *global_p->x, *global_p->y);
//	global_p2 = global_p;
//	global_p = initVec(&local_x2, &local_y2);
////	printf("global_p x2=%d y2=%d\n", *global_p->x, *global_p->y);
//	deleteVec(global_p);
////	printf("global_p2 x=%d y=%d\n", *global_p2->x, *global_p2->y);
//}
// Result: global_v -> local_x
/// END Test26 -------------------

/// Test26 -------------------
//struct vec {
//	int *x;
//	int *y;
//};
//
//struct vec global_v3; 
//struct vec *global_p2;
//struct vec global_v; 
//struct vec global_v2; 
//
//void *vmalloc() { 
//	return malloc(1);
//}
//
//void initVec(struct vec *v, int *x, int *y) {
//	v->x = x;
//	v->y = y;
//}
//
//void deleteVec(struct vec* v) {
//	v->x = NULL;	
//	v->y = NULL;	
//}
//
//void test() {
//	int local_x1 = 1;
//	int local_y1 = 2;
//	int local_x2 = 3;
//	int local_y2 = 4;
//
//	initVec(&global_v, &local_x1, &local_y1);
//	initVec(&global_v2, &local_x2, &local_y2);
////	global_v3.x = global_v2.x;
//	deleteVec(&global_v2);
//}
// Result: global_v -> local_x
/// END Test26 -------------------

/// Test27 -------------------
//struct vec {
//	int *x;
//	int *y;
//};
//
//struct vec global_v; 
//struct vec global_v2; 
//
//void *vmalloc() { 
//	return malloc(1);
//}
//
//void initVec(struct vec *v, int *x, int *y) {
//	v->x = x;
//	v->y = y;
//}
//
//void deleteVec(struct vec* v) {
//	v->x = NULL;	
//	v->y = NULL;	
//}
//
//void tmp(struct vec *param1, int *tmp_x) {
//	tmp_x= param1->x;
//}
//
//void foo(struct vec *param1, struct vec *param2) {
//	int local_x1 = 1;
//	int local_y1 = 2;
//	int local_x2 = 3;
//	int local_y2 = 4;
//	initVec(param1, &local_x1, &local_y1);
//	initVec(param2, &local_x2, &local_y2);
//	deleteVec(param2);
//}
//
//void test() {
//	struct vec local_v; 
//	struct vec local_v2; 
//	foo(&global_v, &global_v2);
////	foo(&local_v, &local_v2);
//}
// Result:
/// END Test27 -------------------

/// Test28 -------------------
//int *global_p1;
//int *global_p2;
//
//void init(int **pp, int *x) {
//	*pp = x;
//}
//
//void delete(int **pp) {
//	*pp = NULL;
//}
//
//void test() {
//	int a;
//	int local_x = 1;
//	int local_y = 2;
//	int *local_p1;
//	int *local_p2;
//
////	init(&global_p1, &local_x);
////	init(&global_p2, &local_y);
////	delete(&global_p2);
//	init(&local_p1, &local_x);
//	init(&local_p2, &local_y);
//	delete(&local_p2);
//}
// Result:
/// END Test28 -------------------

/// Test30 -------------------
//typedef struct list {
//	struct list *Next;
//	int Data;	
//} list;
//
//int Global = 10;
//
//void do_all(list *L, void (*FP)(int*)) {
//	do {
//		FP(&L->Data);
//		L = L->Next;
//	} while(L);
//}
//
//void addG(int *X) { 
//	(*X) += Global; 
//}
//
//void addGToList(list *L) {
//	do_all(L, addG);
//}
//
//list *makeList(int Num) {
//	list *New = malloc(sizeof(list));
//	New->Next = Num ? makeList(Num-1) : 0;
//	New->Data = Num;
//	return New;
//}
//
//void test() {
//	list *X = makeList(10);
//	list *Y = makeList(100);
//	addGToList(X);
//	Global = 20;
//	addGToList(Y);
//}
//Result: No Bug
/// END Test30 -------------------

/// Test31 -------------------
//typedef struct list {
//	int *Data;	
//} list;
//
//list global_lst1;
//list global_lst2; 
//
//void init(list *lst_param, int *x) {
//	lst_param->Data = x;
//}
//
//void delete(list *lst_param) {
//	lst_param->Data = NULL;
//}
//
//void test() {
//	int a;
//	int local_x = 1;
//	int local_y = 2;
//	list local_lst1;
//	list local_lst2;
//
//	init(&global_lst1, &local_x);
//	init(&global_lst2, &local_y);
//	delete(&global_lst2);
////	init(&local_lst1, &local_x);
////	init(&local_lst2, &local_y);
////	delete(&local_lst2);
//}
// Result:
/// END Test31 -------------------

/// Test32 -------------------
//void init(int **pp, int *p) {
//	*pp = p;
//}
//
//void delete(int **pp) {
//	*pp = NULL;
//}
//
//void test(int **param_x, int **param_y) {
//	int local_x = 1;
//	int local_y = 2;
//
//	init(param_x, &local_x);
//	init(param_y, &local_y);
//	delete(param_y);
//		
//}
//
//void sys_futex() {
//	int *x;
//	int *y;
//	test(&x, &y);
//}
// Result: x -> local_x
/// END Test32 -------------------

/// Test33 -------------------
//int* bar(int *y);
//
//int* foo(int *x) {
//	return bar(x);
//}
//
//int* bar(int *y) {
//	if(y)
//		foo(y);
//	return y;
//}
//
//void test() {
//	int local_x = 1;
//	int local_y = 1;
//	int* res1 = foo(&local_x);	
//	int* res2 = foo(&local_y);	
//}
// Result: No Bug
/// END Test33 -------------------

/// Test34 -------------------
//int* bar(int *y);
//int* foo2(int *x);
//int* bar2(int *y);
//int* global_p;
//
//int* foo(int *x) {
//	return bar(x);
//}
//
//int* bar(int *y) {
//	if(y)
//		return foo2(y);
//	return y;
//}
//
//int* foo2(int *x) {
//	return bar2(x);
//}
//
//int* bar2(int *y) {
//	if(y)
//		return foo(y);
//	else
//		global_p = y;
//	return y;
//}
//
//void test() {
//	int local_x = 1;
//	int local_y = 1;
//	int* res1 = foo(&local_x);	
//	int* res2 = foo(&local_y);	
//}
// Result: global_p -> local_x (wont be found because a node wont be forwarded two times within the same context.
// 	 : global_p -> local_y
/// END Test34 -------------------

/// Test35 -------------------
//int* bar(int *a) {
//	int *b = a;
//	return b;
//}
//
//int* foo() {
//	int a = 1;
//	return bar(&a);
//}
//
//void test() {
//	int* res1 = foo();	
//}
// Result: res1 -> a
/// END Test35 -------------------

/// Test36 -------------------
//int* global_p;
//
//void foo(int *x) {
//	int a;
//	x = &a;
//}
//
//void test() {
//	int local_x = 1;
//	foo(global_p);	
//}
// Result: No Bug
/// END Test36 -------------------

/// Test37 -------------------
//void bar(int *y);
//int* global_p1;
//int* global_p2;
//
//void foo(int *x) {
//	global_p1 = x;
//	bar(x);
//}
//
//void bar(int *y) {
//	if(y)
//		foo(y);
//	global_p2 = y;
//}
//
//void test() {
//	int local_x = 1;
//	int local_y = 1;
//	foo(&local_x);	
//	bar(&local_y);
//}
// Result: global_p1 -> local_x
// 	   global_p2 -> local_x
// 	   global_p1 -> local_y
// 	   global_p2 -> local_y
/// END Test37 -------------------

/// Test39 -------------------
//union futex_key {
//	struct {
//		int *x;
//	} shared;
//
//	struct {
//		int *x;
//	} private;
//};
//
//struct futex_q {
//	union futex_key key;
//};
//
//#define FUTEX_KEY_INIT (union futex_key) {.private = { .x = NULL } }
//
//void foo(struct futex_q *q, union futex_key *key) {
//	q->key = *key;	
//}
//
//struct futex_q q;
//void test() {
//	union futex_key key = FUTEX_KEY_INIT;
//	foo(&q, &key);
//}
// Result: No Bug
/// END Test39 -------------------

/// Test40 -------------------
//struct futex_q {
//	int key;
//};
//
//struct list {
//	struct futex_q q;
//};
//
//#define FUTEX_KEY_INIT (union futex_key) {.private = { .x = NULL } }
//
//void bar(struct list *l, struct futex_q *q2) {
//	l->q = *q2;
//}
//
//struct list l;
//void test() {
//	struct futex_q q2;
//	bar(&l, &q2);
//}
// Result: No Bug
/// END Test40 -------------------
	
/// Test41 -------------------
//struct futex_q {
//	int key;
//};
//
//struct list {
//	struct futex_q *q;
//};
//
//#define FUTEX_KEY_INIT (union futex_key) {.private = { .x = NULL } }
//
//struct futex_q* bar(struct list *l, struct futex_q *q2) {
//	l->q = q2;
//	return q2;
//}
//
//void foo(struct list *l, struct futex_q *q2) {
//	l->q = q2;
//}
//
//struct list l;
//void test() {
//	struct futex_q q2;
//	struct futex_q q3;
//	foo(&l, bar(&l, &q2));
//}
// Result: l -> q2
/// END Test41 -------------------

/// Test42 -------------------
//int *global_p;
//int global_x;
//
//void foo3() {
//	int local_z;
//	global_p = &local_z;
//}
//
//void foo2() {
//	int local_y;
//	global_p = &local_y;
//	foo3();
//}
//
//void foo1(int *p) {
//	global_p = p;
//	foo2();
//}
//
//void test() {
//	int local_x;
//	foo1(&local_x);
//}
// Result: global_p -> local_z 
/// END Test42 -------------------

/// Test43 -------------------
//int *global_p;
//int *global_p2;
//
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//void init(struct vec2D *v, int *x, int *y) {
//	v->x = x;
//	v->y = y;
//}
//
//void del_x(struct vec2D *v) {
//	if(v->y == NULL)
//		printf("\n");		
//	v->x = NULL;
//}
//
//void foo(struct vec2D *v) {
//	int *x = v->x;
//	int *y = v->y;
//}
//
//void test() {
//	struct vec2D local_v;
//	int x = 1;
//	int y = 3;
//	init(&local_v, &x, &y);
//	del_x(&local_v);
//	foo(&local_v);
//}
// Result: No Bug
/// END Test43 -------------------

/// Test44 -------------------
//int *global_p;
//int *global_p2;
//
//void foo() {
//	global_p2 = global_p;
//	global_p = NULL;
//}
//
//void foo2() {
//	foo();
//}
//
//void test() {
//	int local_x = 1;
//	int *local_p = &local_x;
//	memcpy(global_p, local_p, sizeof(int));
//	foo2();
//}
// Result: No Bug
/// END Test44 -------------------

/// Test45 -------------------
//int *global_p;
//int *global_p2;
//
//int* foo2();
//
//int* foo() {
//	return foo2();
//}
//
//int* foo2() {
//	int local_y = 1;
//	global_p = &local_y;
//	return global_p;
//}
//
//void test() {
//	int local_x = 1;
//	int *local_p = foo();
//}
// Result: global_p -> local_y
// 	   local_p -> local_y	
/// END Test45 -------------------

/// Test46 -------------------
//struct vec2D {
//	int *x;
//	int *y;
//};
//
//int *global_p;
//
//void foo(struct vec2D *param_v) {
//	int local_x;
//	param_v->x = &local_x;
////	global_p = *pp;	
//	global_p = param_v->x;
//}
//
//void test() {
//	struct vec2D local_v;
//	foo(&local_v);
//}
// Result: global_p -> local_x
// 	   local_v -> local_x
/// END Test46 -------------------

/// Test47 -------------------
//struct vec2D {
//	int x;
//	int y;
//};
//
//int *global_p;
//
//void foo(struct vec2D *param_v) {
//	int *local_x;
//	local_x = &param_v->x;
//	global_p = local_x;
//}
//
//void test() {
//	struct vec2D local_v;
//	foo(&local_v);
//}
// Result: global_p -> local_v
/// END Test47 -------------------

/// Test48 -------------------
//struct vec2D {
//	int x;
//	int *y;
//};
//
//int *global_p;
//
//void foo(struct vec2D *param_v) {
//	int *local_x;
//	struct vec2D local_v2;
//	local_v2.y = &param_v->x;
//	global_p = local_v2.y;
//}
//
//void test() {
//	struct vec2D local_v;
//	foo(&local_v);
//}
// Result: global_p -> local_v
/// END Test48 -------------------

/// Test49 -------------------
//char* kstrdup(const char *s) {
//	char *buf;
//	size_t len = 10;
//
//	buf = (char*)malloc(len);
//
//	if(buf)
//		memcpy(buf, s, len);
//
//	return buf;
//}
//
//void bar(char *module_name) {
//	module_name = kstrdup(module_name);
//}
//
//void foo() {
//	char module_name[10];
//	bar(module_name);
//}
//
//void test() {
//	foo();
//}
// Result: No Bug
/// END Test49 -------------------

/// Test50 -------------------
//struct v_area {
//	struct v_area *v_prev;
//	struct v_area *v_next;
//} global_v;
//
//void mlock_fixup(struct v_area *va, struct v_area **prev) {
//	va = *prev;
//	*prev = va;
//}
//
//void do_mlock() {
//	struct v_area *prev;
//	struct v_area *va = &global_v;
//	prev = va->v_prev;
//	mlock_fixup(va, &prev);
//}
//
//void test() {
//	do_mlock();
//}
// Result: No Bug
/// END Test50 -------------------

/// Test51 -------------------
//struct net_device {
//	struct list_head close_list;
//};
//
//void foo(struct net_device *dev) {
//	LIST_HEAD(single);
//	list_add(&dev->close_list, &single);
//	list_del(&single);
//}
//
//void test() {
//	struct net_device dev;
//	foo(&dev);
//}
// Result: No Bug
/// END Test51 -------------------

/// Test52 -------------------
//struct vec2D {
//	int *x;
//};
//
//struct elem {
//	struct vec2D v;
//};
//
//void foo(struct vec2D *param_v) {
//	int local_x = 0;
//	param_v->x = &local_x;
//}
//
//void test() {
//	struct elem e;
//	foo(&e.v);
//}
// Result: e -> local_x
/// END Test52 -------------------

/// Test53 -------------------
//void kfree(void *p) {
//	p = NULL;
//	// do something
//}
//
//void* vmalloc(size_t size) {
//}
//
//void foo() {
//	int val = 32;
//	int *p= (int*)vmalloc(sizeof(int));
//	if(val == 0) {
//		kfree(p);
//	}
//}
//
//void test() {
//	foo();
//}
// Result: partial use-after-free
/// END Test53 -------------------

///// Test54 -------------------
//void* vmalloc(size_t size) {
//}
//
//void kfree(void *p) {
//	p = NULL;
//	// do something
//}
//
//void foo() {
//	int val = 32;
//	int *p= (int*)vmalloc(sizeof(int));
//	kfree(p);
//	free(p);
//}
//
//void test() {
//	foo();
//}
// Result: double free
/// END Test54 -------------------

void init(int **pp, int *x) {
	*pp = x;
}

void delete(int **pp) {
	*pp = NULL;
}


void foo() {
	int local_x = 1; 
	int local_y = 2;
	int *local_p1 = (int*)malloc(1); 
	int *local_p2 = (int*)malloc(1);

	init(&local_p1, &local_x);
	init(&local_p2, &local_y);
	delete(&local_p2);
}

void test() {
	foo();
}

void sys_futex() {
	test();
}

int main() {
	test();
	return 0;
} 
