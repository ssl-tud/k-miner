K-Miner: Data-Flow Analysis for the Linux Kernel
================================================
                               _  ____  __ __
                              | |/ /  \/  (_)_ _  ___ _ _
                              | ' <| |\/| | | ' \/ -_) '_|
                              |_|\_\_| _|_|_|_||_\___|_|__
                              |___|___|___|___|___|___|___|

Requirements:
-------------
Install LLVM/Clang (Version 3.8.1):
* http://linuxdeveloper.blogspot.de/2012/12/building-llvm-32-from-source.html
* http://llvm.org/docs/CMake.html

Build KMiner:
-------------
```
cd KMiner
mkdir build
cd build
export LLVM_DIR=<path_to_LLVM_build>
cmake ..
make -j4
ln -s /path_to_kminer_build/bin/kminer ~/.local/bin/kminer
```

Test KMiner:
------------
```
cd tests
./run_tests.sh
```

Build the Kernel:
-----------------
In order to analyze the kernels sourcecode it is necessary to convert the code into LLVM-Bitcode.
This entire process can be skipped for kernel version (3.19, 4.2, 4.6, 4.10 and 4.12) as their LLVM-Bitcodes are [already compiled](http://david.g3ns.de/kminer/kminer.html).

Clone the Linux kernel and apply the patches that enable compilation with LLVM/Clang. Follow the README in clang-kernel-build. Afterwards, the file "<path_to_kernel>/scripts/Makefile.build" has to be modified.

To patch Kernel Makefiles, do `patch kernel/scripts/Makefile.build clang-kernel-build/Makefile.build.patch`.

Because the Makefiles might differ with different versions it might be necessary to change the rules manually (in scripts/Makefile.build):
1. Disable the "CONFIG_MODVERSION=n" in .config
2. add below "ifndef CONFIG_MODVERSIONS" (top .o.c rule, line 201):
    ```
    cmd_cc_o_c = \
            if [ $(@) != "scripts/mod/empty.o" ]; then \
                    $(CC) $(c_flags) -save-temps=obj -o $@ -c $<; \
            else \
                    $(CC) $(c_flags) -c -o $@ $<; \
            fi
    ```
3. Add LLVM-IR bitcode target rule from C-source files (line 280):
    ```
    ...
    $(obj)/%.bc: $(src)/%.c
            $(call if_changed_rule,cc_o_c)
    ```
4. Add LLVM-IR bitcode target rule from ASM source files (line 320):
    ```
    quiet_cmd_as_bc_S = $(CC) -emit-llvm $(quiet_modtag)  $@
    cmd_as_bc_S       = $(CC) $(a_flags) -emit-llvm -c -o $@ $<
    asm_ign := ""
    $(obj)/%.bc: $(src)/%.S FORCE
            $(eval asm_ign += $@)
            $(call if_changed_dep,as_bc_S)
    ```
5. If we have bitcode-files that can be linked together create a command, otherwise create an empty targetfile (line 358):
    ```
    tmp = $(patsubst %.o, %.bc, $(obj-y))
    bc-y = $(filter-out $(asm_ign), $(tmp))
    #$(foreach ign, $(asm_ign), $(eval link_bc-y = $(filter-out $ign, $(link_bc-y))))
    #quiet_cmd_link_o_target_bc = llvm-link $@
    ifneq ($(bc-y),"")
    cmd_link_bc_target = $(if $(strip $(bc-y)),\
                         llvm-link -o $@ $(filter $(bc-y), $^) \
                         $(cmd_secanalysis),\
                         echo "" > $@)
    #rm -f $@; $(AR) rcs$(KBUILD_ARFLAGS) $@)
    else
    cmd_link_bc_target = echo "" > $@
    endif
    ```
6.  Add rule to link bitcode-files (line 382):
    ```
    $(builtin-target_bc): $(bc-y) FORCE
            $(call if_changed,link_bc_target)
    ```
7. Link library bc objects (line 410):
    ```
    lib_bc-y := $(patsubst %.o, %.bc, $(lib-y))
    cmd_link_bc_l_target = llvm-link -o $@.bc $(lib_bc-y)

    $(lib-target): $(lib-y) FORCE
            $(call if_changed,link_l_target)
            $(call if_changed,link_bc_l_target)
    ```
8. Add rule to link multiple bitcode-files (line 429):
    ```
    quiet_cmd_link_bc_multi-y = llvm-link   $@
    cmd_link_bc_multi-y = llvm-link -o $(patsubst %.o, %.bc, $@) \
                     $(patsubst %.o, %.bc, $(link_multi_deps))
    ```
9. Add the previously created rule to the original dependencies (line 442):
    ```
    $(multi-used-y): FORCE
            $(call if_changed,link_multi-y)
            $(call if_changed,link_bc_multi-y)
    ```

Now compile the Kernel:
```
make defconfig
make HOSTCC=clang CC=clang SHELL=/bin/bash
```
And you should see many .bc files being created (in addition to the default built artifacts). Now you should be able to link the vmlinux.bc image. If the linking process fails for some reason, try another linker, or link the bitcode yourself (e.g., obtain the file list from the command used to link vmlinux during the build):
```
llvm-link -o vmlinux.bc <list of files to link>
```

Manual:
-------
kminer <bitcode> [options] [checker]
```
Options:
        -help                                           Display available options.
        -syscall=<system_call_name>                     Select a system call that should be analyzed.
        -driver=<driver_name>                           Select a driver that should be analyzed.
        -initcall-contexts=<context_file>               File used for import and export of the pre-analysis.
        -report=<report_file>                           Outputfile
        -cg_ignore=<ignore_file>                        Filter functions that should be ignored.
        -export-bugs=<bug_file>                         File to store the bugs that where found.
        -import-bugs=<bug_file>                         File of bugs that should be imported.
        -uarl-sinks=<sink_file>                         Contains the functions that will interpreted as
                                                        delete functions within the use-after-return checker.
        -uar-timeout=<duration>                         Timeout of single variable check within the
                                                        use-after-return checker.
        -num-threads=<number_of_threads>                Number of threads the analysis should use. (At the
                                                        moment mainly the use-after-return check uses multithreading)
        -rm-deref                                       Ignore derefences.
        -path-sens                                      Perform a path-sensitive analysis.
        -monitor-system                                 Capture the usage of system resources.
        -func-limit=<max_num_of_func>                   Max. number of functions that should be analyzed.

Checker:
        -use-after-return                               Check for use-after-returns.
        -use-after-return-lite                          Same as use-after-return but less accurate and faster.
        -use-after-free                                 Check for use-after-free.
        -double-free                                    Check for double-free.
        -double-lock                                    Check for double-locks.
        -leak                                           Check for memory leaks.
```

Examples:
---------
Use-after-return support (e.g., in futex system call):
```
kminer vmlinux_v3.19.bc -syscall=sys_futex -initcall-contexts=initcall_contexts.txt \
-cg-ignore=filter.txt -use-after-return-lite -uarl-sinks=sinks.txt
```

Early driver analysis support (e.g., double free in SCSI Generic driver interface):
```
kminer vmlinux_v3.19.bc -driver=sg -double-free -num-threads=8 -cg-ignore=filter.txt \
-initcall-contexts=initcall_contexts.txt -report=report.txt -rm-deref -path-sens
```
