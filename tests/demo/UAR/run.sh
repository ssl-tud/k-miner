#!/bin/bash

set -x
bitcode="$1"

kminer "$bitcode" 								\
		-syscall="sys_futex" 						\
		-use-after-return-lite						\
		-num-threads=8 							\
		-cg-ignore="blist.txt"  					\
		-initcall-contexts="initcall_contexts.txt" 			\
		-uarl-sinks="sinks.txt"						\
		2>> err.log
