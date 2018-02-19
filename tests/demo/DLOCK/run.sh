#!/bin/bash

set -x
bitcode="$1"

kminer "$bitcode" -driver="drm/drm_prime.c" -cg-ignore=blist.txt -initcall-contexts=initcall_contexts.txt -report=report.txt -double-lock 2>>err.log
