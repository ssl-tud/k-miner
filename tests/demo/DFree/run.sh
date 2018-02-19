#!/bin/bash

set -x
bitcode="$1"

kminer "$bitcode" 								\
		-driver="sg" 							\
		-double-free							\
		-num-threads=8 							\
		-cg-ignore="blist.txt"						\
		-initcall-contexts="initcall_contexts.txt" 			\
		-report="report.txt"						\
		-rm-deref							\
		-path-sens							\
		2>> err.log
