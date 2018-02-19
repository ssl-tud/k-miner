import re
from Report import (KernelReport, Report, APIType, UseAfterReturnReport,
                    UseAfterFreeReport, DoubleFreeReport, MemoryLeakReport,
                    DoubleLockReport, UseAfterFree, UseAfterReturn, DoubleFree,
                    MemoryLeak, DoubleLock, Var, Context, SystemInfo, PartitionerStat)


def get_last_word(line: str):
    return line.split()[-1].strip()


def parse_partitioner_stat(raw_partitioner_stat: str, type: APIType):
    partitioner_stat = PartitionerStat(type)
    i = 0

    # Initcall Stat
    for i, line in enumerate(raw_partitioner_stat):
            if "Initcalls" in line:
                    partitioner_stat.numIR = int(line.split("|")[2].strip())
                    partitioner_stat.numIO = int(line.split("|")[3].strip())
            elif "API" in line:
                    break
            elif "Functions" in line:
                    partitioner_stat.numIRFuncs = int(line.split("|")[2].strip())
                    partitioner_stat.numIOFuncs = int(line.split("|")[3].strip())
            elif "Global Variables" in line:
                    partitioner_stat.numIRGVs = int(line.split("|")[2].strip())
                    partitioner_stat.numIOGVs = int(line.split("|")[3].strip())
            elif "Non Defined Variables" in line:
                    partitioner_stat.numIRNDVs = int(line.split("|")[2].strip())
                    partitioner_stat.numIONDVs = int(line.split("|")[3].strip())
            elif "Call Graph Depth" in line:
                    partitioner_stat.iRCGDepth = int(line.split("|")[2].strip())
                    partitioner_stat.iOCGDepth = int(line.split("|")[3].strip())

    raw_partitioner_stat = raw_partitioner_stat[i:]


    # API (Syscall/Driver) Stat
    for i, line in enumerate(raw_partitioner_stat):
            if "Kernel" in line:
                    break
            elif "Functions" in line:
                    partitioner_stat.numARFuncs = int(line.split("|")[2].strip())
                    partitioner_stat.numAOFuncs = int(line.split("|")[3].strip())
            elif "Global Variables" in line:
                    partitioner_stat.numARGVs = int(line.split("|")[2].strip())
                    partitioner_stat.numAOGVs = int(line.split("|")[3].strip())
            elif "Call Graph Depth" in line:
                    partitioner_stat.aRCGDepth = int(line.split("|")[2].strip())
                    partitioner_stat.aOCGDepth = int(line.split("|")[3].strip())

    raw_partitioner_stat = raw_partitioner_stat[i:]

    # Kernel Stat
    for i, line in enumerate(raw_partitioner_stat):
            if "Functions" in line:
                    partitioner_stat.numKRFuncs = int(line.split("|")[2].strip())
                    partitioner_stat.numKOFuncs = int(line.split("|")[3].strip())
            elif "Global Variables" in line:
                    partitioner_stat.numKRGVs = int(line.split("|")[2].strip())
                    partitioner_stat.numKOGVs = int(line.split("|")[3].strip())
            elif "Initcall Function Limit" in line:
                    partitioner_stat.initcall_func_limit = int(get_last_word(line))
            elif "API Function Limit" in line:
                    partitioner_stat.api_func_limit = int(get_last_word(line))

    return partitioner_stat


def parse_system_info(raw_system_info: str):
    system_info = SystemInfo()

    for i, line in enumerate(raw_system_info):
            if "Kernel Name" in line:
                    system_info.kernel_name = get_last_word(line)
            elif "Kernel Release" in line:
                    system_info.kernel_release = get_last_word(line)
            elif "Kernel Version" in line:
                    system_info.kernel_version = raw_system_info[i + 1].strip()
            elif "Processor" in line:
                    system_info.processor = get_last_word(line)
            elif "Num Cores" in line:
                    system_info.num_cores = get_last_word(line)
            elif "Total-Mem" in line:
                    system_info.mem_usage = get_last_word(line).split("/")[0]
                    system_info.mem_max_usage = get_last_word(line).split("/")[1]
                    system_info.mem_total = get_last_word(line).split("/")[2]
                    break
            # for old versions
            elif "MemTotal" in line:
                    system_info.mem_total = get_last_word(line)
                    break
    return system_info


def parse_func_path(raw_func_path: str, terminator: str, res: list()):
    i = 0

    for i, line in enumerate(raw_func_path):
            if terminator not in line:
                    if line.count("+") == 1 or not line.strip() or line.count("________") > 0:
                            continue

                    line = re.sub("\.\d*$", "", line)
                    line = line.replace(".", ".&emsp;")
                    line = line.replace("--->", "&#8674;")
                    line = line.replace("<---", "&#8672;")
                    line = line.replace("===>", "&#8658;")
                    line = line.replace("<===", "&#8656;")
                    line = line.replace("++>", "&#10565;")
                    line = line.replace("<++", "&#10566;")

                    res.append(line)
            else:
                    break
    return i


def parse_func_loc_map(raw_func_loc_map: str, terminator: str, res: dict()):
    i = 0

    for i, line in enumerate(raw_func_loc_map):
            if terminator not in line:
                    if line != "" and line.split()[0] == "-":
                            loc = Context()
                            loc.function_name = line.split()[1]
                            loc.file_name = line.split()[2]
                            loc.line = line.split()[4][:-1]
                            res[loc.function_name] = loc
            else:
                    break

    return i


def parse_use_after_free(raw_uaf: str):
    use_after_free = UseAfterFree()
    dangling_ptr_context = None
    free_context = None
    use_context = None
    i = 0

    for i, line in enumerate(raw_uaf):
            if "API TO SOURCE" in line:
                    use_after_free.use.loc = use_context
                    break
            elif "SOURCE" in line:
                    use_after_free.dangling_ptr = Var()
                    dangling_ptr_context = Context()
            elif "FIRST SINK" in line:
                    use_after_free.dangling_ptr.loc = dangling_ptr_context
                    use_after_free.free = Var()
                    free_context = Context()
            elif "SECOND SINK" in line:
                    use_after_free.free.loc = free_context
                    use_after_free.use = Var()
                    use_context = Context()
            elif "File" in line:
                    if dangling_ptr_context and not free_context:
                            dangling_ptr_context.file_name = get_last_word(line)
                    elif free_context and not use_context:
                            free_context.file_name = get_last_word(line)
                    else:
                            use_context.file_name = get_last_word(line)
            elif "Function" in line:
                    if dangling_ptr_context and not free_context:
                            dangling_ptr_context.function_name = get_last_word(line)
                    elif free_context and not use_context:
                            free_context.function_name = get_last_word(line)
                    else:
                            use_context.function_name = get_last_word(line)
            elif "Line" in line:
                    if dangling_ptr_context and not free_context:
                            dangling_ptr_context.line = get_last_word(line)
                    elif free_context and not use_context:
                            free_context.line = get_last_word(line)
                    else:
                            use_context.line = get_last_word(line)
            elif "Name" in line:
                    if use_after_free.dangling_ptr:
                            use_after_free.dangling_ptr.name = get_last_word(line)

    raw_uaf = raw_uaf[i + 2:]

    i = parse_func_path(raw_uaf, "PATH TO FIRST SINK", use_after_free.api_path)

    raw_uaf = raw_uaf[i + 2:]

    i = parse_func_path(raw_uaf, "PATH TO SECOND SINK", use_after_free.free_path)

    raw_uaf = raw_uaf[i + 2:]

    i = parse_func_path(raw_uaf, "FUNCTION-LOCATION MAP", use_after_free.use_path)

    raw_uaf = raw_uaf[i:]

    i = parse_func_loc_map(raw_uaf, "Duration", use_after_free.function_locs)

    raw_uaf = raw_uaf[-1]

    use_after_free.duration = float(get_last_word(raw_uaf))

    return use_after_free


def parse_use_after_return(raw_uar: str):
    use_after_return = UseAfterReturn()
    dangling_ptr_context = None
    local_var_context = None
    i = 0

    for i, line in enumerate(raw_uar):
            if "API TO LOCALVAR" in line:
                    use_after_return.local_var.loc = local_var_context
                    break
            elif "DANGLING POINTER" in line:
                    use_after_return.dangling_ptr = Var()
                    dangling_ptr_context = Context()
            elif "LOCAL VARIABLE" in line:
                    use_after_return.dangling_ptr.loc = dangling_ptr_context
                    use_after_return.local_var = Var()
                    local_var_context = Context()
            elif "File" in line:
                    if dangling_ptr_context and not local_var_context:
                            dangling_ptr_context.file_name = get_last_word(line)
                    else:
                            local_var_context.file_name = get_last_word(line)
            elif "Function" in line:
                    if dangling_ptr_context and not local_var_context:
                            dangling_ptr_context.function_name = get_last_word(line)
                    else:
                            local_var_context.function_name = get_last_word(line)
            elif "Line" in line:
                    if dangling_ptr_context and not local_var_context:
                            dangling_ptr_context.line = get_last_word(line)
                    else:
                            local_var_context.line = get_last_word(line)
            elif "Name" in line:
                    if use_after_return.dangling_ptr and not use_after_return.local_var:
                            use_after_return.dangling_ptr.name = get_last_word(line)
                    else:
                            use_after_return.local_var.name = get_last_word(line)

    raw_uar = raw_uar[i + 2:]

    i = parse_func_path(raw_uar, "LOCALVAR TO OUTOFSCOPE", use_after_return.api_path)

    raw_uar = raw_uar[i + 2:]

    i = parse_func_path(raw_uar, "DANGLINGPTR TO OUTOFSCOPE", use_after_return.lvar_to_oos_path)

    raw_uar = raw_uar[i + 2:]

    i = parse_func_path(raw_uar, "FUNCTION-LOCATION MAP", use_after_return.dptr_to_oos_path)

    raw_uar = raw_uar[i:]

    i = parse_func_loc_map(raw_uar, "Duration", use_after_return.function_locs)

    raw_uar = raw_uar[-1]

    use_after_return.duration = float(get_last_word(raw_uar))

    return use_after_return


def parse_double_free(raw_dfree: str):
    double_free= DoubleFree()
    dangling_ptr_context = None
    free_context1 = None
    free_context2 = None
    i = 0

    for i, line in enumerate(raw_dfree):
            if "API TO SOURCE" in line:
                    double_free.free2.loc = free_context2
                    break
            elif "SOURCE" in line:
                    double_free.dangling_ptr = Var()
                    dangling_ptr_context = Context()
            elif "FIRST SINK" in line:
                    double_free.dangling_ptr.loc = dangling_ptr_context
                    double_free.free1 = Var()
                    free_context1 = Context()
            elif "SECOND SINK" in line:
                    double_free.free1.loc = free_context1
                    double_free.free2 = Var()
                    free_context2 = Context()
            elif "File" in line:
                    if dangling_ptr_context and not free_context1:
                            dangling_ptr_context.file_name = get_last_word(line)
                    elif free_context1 and not free_context2:
                            free_context1.file_name = get_last_word(line)
                    else:
                            free_context2.file_name = get_last_word(line)
            elif "Function" in line:
                    if dangling_ptr_context and not free_context1:
                            dangling_ptr_context.function_name = get_last_word(line)
                    elif free_context1 and not free_context2:
                            free_context1.function_name = get_last_word(line)
                    else:
                            free_context2.function_name = get_last_word(line)
            elif "Line" in line:
                    if dangling_ptr_context and not free_context1:
                            dangling_ptr_context.line = get_last_word(line)
                    elif free_context1 and not free_context2:
                            free_context1.line = get_last_word(line)
                    else:
                            free_context2.line = get_last_word(line)
            elif "Name" in line:
                    if double_free.dangling_ptr:
                            double_free.dangling_ptr.name = get_last_word(line)

    raw_dfree = raw_dfree[i + 2:]

    i = parse_func_path(raw_dfree, "PATH TO FIRST SINK", double_free.api_path)

    raw_dfree = raw_dfree[i + 2:]

    i = parse_func_path(raw_dfree, "PATH TO SECOND SINK", double_free.free_path1)

    raw_dfree = raw_dfree[i + 2:]

    i = parse_func_path(raw_dfree, "FUNCTION-LOCATION MAP", double_free.free_path2)

    raw_dfree = raw_dfree[i:]

    i = parse_func_loc_map(raw_dfree, "Duration", double_free.function_locs)

    raw_dfree = raw_dfree[-1]

    double_free.duration = float(get_last_word(raw_dfree))

    return double_free


def parse_memory_leak(raw_leak: str):
    memory_leak = MemoryLeak()
    memory_leak.leak_ptr = Var()
    leak_context = Context()

    for i, line in enumerate(raw_leak):
            if "API TO SOURCE" in line:
                    memory_leak.leak_ptr.loc = leak_context
                    break
            elif "Never Free at" in line:
                    memory_leak.never_free = True
            elif "Partial Leak at" in line:
                    memory_leak.never_free = False
            elif "File" in line:
                    leak_context.file_name = get_last_word(line)
            elif "Function" in line:
                    leak_context.function_name = get_last_word(line)
            elif "Line" in line:
                    leak_context.line = get_last_word(line)
            elif "Name" in line:
                    memory_leak.leak_ptr.name = get_last_word(line)

    raw_leak = raw_leak[i + 2:]

    i = parse_func_path(raw_leak, "Duration", memory_leak.api_path)

    raw_leak = raw_leak[-1]

    memory_leak.duration = float(get_last_word(raw_leak))

    return memory_leak


def parse_double_lock(raw_dlock: str):
    double_lock = DoubleLock()
    lock_var_context = None
    lock_context1 = None
    lock_context2 = None
    free_context2 = None
    i = 0

    for i, line in enumerate(raw_dlock):
            if "API TO SOURCE" in line:
                    double_lock.lock2.loc = free_context2
                    break
            elif "SOURCE" in line:
                    double_lock.lock_var = Var()
                    lock_var_context = Context()
            elif "FIRST SINK" in line:
                    double_lock.lock_var.loc = lock_var_context
                    double_lock.lock1 = Var()
                    lock_context1 = Context()
            elif "SECOND SINK" in line:
                    double_lock.lock1.loc = lock_context1
                    double_lock.lock2 = Var()
                    free_context2 = Context()
            elif "File" in line:
                    if lock_var_context and not lock_context1:
                            lock_var_context.file_name = get_last_word(line)
                    elif lock_context1 and not lock_context2:
                            lock_context1.file_name = get_last_word(line)
                    else:
                            lock_context2.file_name = get_last_word(line)
            elif "Function" in line:
                    if lock_var_context and not lock_context1:
                            lock_var_context.function_name = get_last_word(line)
                    elif lock_context1 and not lock_context2:
                            lock_context1.function_name = get_last_word(line)
                    else:
                            lock_context2.function_name = get_last_word(line)
            elif "Line" in line:
                    if lock_var_context and not lock_context1:
                            lock_var_context.line = get_last_word(line)
                    elif lock_context1 and not lock_context2:
                            lock_context1.line = get_last_word(line)
                    else:
                            lock_context2.line = get_last_word(line)
            elif "Name" in line:
                    if double_lock.lock_var:
                            double_lock.lock_var.name = get_last_word(line)

    raw_dlock = raw_dlock[i + 2:]

    i = parse_func_path(raw_dlock, "PATH TO FIRST SINK", double_lock.api_path)

    raw_dlock = raw_dlock[i + 2:]

    i = parse_func_path(raw_dlock, "PATH TO SECOND SINK", double_lock.lock_path1)

    raw_dlock = raw_dlock[i + 2:]

    i = parse_func_path(raw_dlock, "FUNCTION-LOCATION MAP", double_lock.lock_path2)

    raw_dlock = raw_dlock[i:]

    i = parse_func_loc_map(raw_dlock, "Duration", double_lock.function_locs)

    raw_dlock = raw_dlock[-1]

    double_lock.duration = float(get_last_word(raw_dlock))

    return double_lock


def parse_uaf_report(raw_uaf_report: str):
    uaf_report = list()

    raw_uaf = list()

    for line in raw_uaf_report:
            raw_uaf.append(line)

            if "Duration" in line:
                    use_after_free = parse_use_after_free(raw_uaf)
                    uaf_report.append(use_after_free)
                    del raw_uaf[:]

    return uaf_report


def parse_uar_report(raw_uar_report: str):
    uar_report = list()

    raw_uar = list()

    for line in raw_uar_report:
            raw_uar.append(line)

            if "Duration" in line:
                    use_after_return = parse_use_after_return(raw_uar)
                    uar_report.append(use_after_return)
                    del raw_uar[:]

    return uar_report


def parse_dfree_report(raw_dfree_report: str):
    dfree_report = list()

    raw_dfree = list()

    for line in raw_dfree_report:
            raw_dfree.append(line)

            if "Duration" in line:
                    double_free = parse_double_free(raw_dfree)
                    dfree_report.append(double_free)
                    del raw_dfree[:]

    return dfree_report


def parse_leak_report(raw_leak_report: str):
    leak_report = list()

    raw_leak = list()

    for line in raw_leak_report:
            raw_leak.append(line)

            if "Duration" in line:
                    memory_leak = parse_memory_leak(raw_leak)
                    leak_report.append(memory_leak)
                    del raw_leak[:]

    return leak_report


def parse_dlock_report(raw_dlock_report: str):
    dlock_report = list()

    raw_dlock = list()

    for line in raw_dlock_report:
            raw_dlock.append(line)

            if "Duration" in line:
                    double_lock = parse_double_lock(raw_dlock)
                    dlock_report.append(double_lock)
                    del raw_dlock[:]

    return dlock_report


def parse_uaf_checker_report(raw_uaf_checker_report: str):
    uaf_report = UseAfterFreeReport()

    raw_report = list()

    for line in raw_uaf_checker_report:
            if "Time" in line and "Timeout" not in line:
                    uaf_report.duration = get_last_word(line)
            elif "Num analyzed" in line:
                    uaf_report.num_analyzed_var = get_last_word(line)
            elif "Num bugs found" in line:
                    uaf_report.num_bugs = int(get_last_word(line))
            else:
                    raw_report.append(line)

    if uaf_report.num_bugs > 0:
            uaf_report.bugs = parse_uaf_report(raw_report)

    return uaf_report


def parse_uar_checker_report(raw_uar_checker_report: str):
    uar_report = UseAfterReturnReport()

    raw_report = list()

    for line in raw_uar_checker_report:
            if "Time" in line and "Timeout" not in line:
                    uar_report.duration = get_last_word(line)
            elif "Timeout" in line:
                    uar_report.timeout = get_last_word(line)
            elif "Num analyzed local variables" in line:
                    uar_report.num_analyzed_var = get_last_word(line)
            elif "Num bugs found" in line:
                    uar_report.num_bugs = int(get_last_word(line))
            else:
                    raw_report.append(line)

    if uar_report.num_bugs > 0:
            uar_report.bugs = parse_uar_report(raw_report)

    return uar_report


def parse_dfree_checker_report(raw_dfree_checker_report: str):
    dfree_report = DoubleFreeReport()

    raw_report = list()

    for line in raw_dfree_checker_report:
            if "Time" in line:
                    dfree_report.duration = get_last_word(line)
            elif "Num bugs found" in line:
                    dfree_report.num_bugs = int(get_last_word(line))
            elif "Num analyzed variables" in line:
                    dfree_report.num_analyzed_var = get_last_word(line)
            else:
                    raw_report.append(line)

    if dfree_report.num_bugs > 0:
            dfree_report.bugs = parse_dfree_report(raw_report)

    return dfree_report


def parse_leak_checker_report(raw_leak_checker_report: str):
    leak_report = MemoryLeakReport()

    raw_report = list()

    for line in raw_leak_checker_report:
            if "Time" in line:
                    leak_report.duration = get_last_word(line)
            elif "Num bugs found" in line:
                    leak_report.num_bugs = int(get_last_word(line))
            elif "Num analyzed variables" in line:
                    leak_report.num_analyzed_var = get_last_word(line)
            else:
                    raw_report.append(line)

    if leak_report.num_bugs > 0:
            leak_report.bugs = parse_leak_report(raw_report)

    return leak_report


def parse_dlock_checker_report(raw_dlock_checker_report: str):
    dlock_report = DoubleLockReport()

    raw_report = list()

    for line in raw_dlock_checker_report:
            if "Time" in line:
                    dlock_report.duration = get_last_word(line)
            elif "Num bugs found" in line:
                    dlock_report.num_bugs = int(get_last_word(line))
            elif "Num analyzed variables" in line:
                    dlock_report.num_analyzed_var = get_last_word(line)
            else:
                    raw_report.append(line)

    if dlock_report.num_bugs > 0:
            dlock_report.bugs = parse_dlock_report(raw_report)

    return dlock_report


def parse_report(raw_report: str):
    report = Report()
    i = 0

    raw_checker_report = list()
    raw_system_info = list()
    raw_partitioner_stat = list()

    # General Info
    for i, line in enumerate(raw_report):
            if "API" in line:
                    if "DRIVER" in get_last_word(line):
                        report.api_type = APIType.DRIVER
                    elif "SYSCALL" in get_last_word(line):
                        report.api_type = APIType.SYSCALL
                    else:
                        report.api_type = APIType.UNKNOWN
            elif "System call" in line:
                    assert(report.api_type == APIType.SYSCALL)
                    report.syscall_name = get_last_word(line)
            elif "Driver" in line:
                    assert(report.api_type == APIType.DRIVER)
                    report.driver_name = get_last_word(line)
            elif "Num Threads" in line:
                    report.num_threads = get_last_word(line)
            elif "Analysis Statistic" in line or "System Info" in line:
                    break

    raw_report = raw_report[i:]

    if "Use-After-Return Analysis Statistic" in raw_report[0]:
        for i, line in enumerate(raw_report[1:]):
            if "Analysis Statistic" not in line and "System Info" not in line:
                    raw_checker_report.append(line)
            else:
                    checker_report = parse_uar_checker_report(raw_checker_report)
                    report.uar_checker_report = checker_report
                    break
        del raw_checker_report[:]
        raw_report = raw_report[i+1:]

    if "Memory-Leak Analysis Statistic" in raw_report[0]:
        for i, line in enumerate(raw_report[1:]):
            if "Analysis Statistic" not in line and "System Info" not in line:
                    raw_checker_report.append(line)
            else:
                    checker_report = parse_leak_checker_report(raw_checker_report)
                    report.leak_checker_report = checker_report
                    break
        del raw_checker_report[:]
        raw_report = raw_report[i+1:]

    if "Use-After-Free Analysis Statistic" in raw_report[0]:
        for i, line in enumerate(raw_report[1:]):
            if "Analysis Statistic" not in line and "System Info" not in line:
                    raw_checker_report.append(line)
            else:
                    checker_report = parse_uaf_checker_report(raw_checker_report)
                    report.uaf_checker_report = checker_report
                    break
        del raw_checker_report[:]
        raw_report = raw_report[i+1:]

    if "Double-Free Analysis Statistic" in raw_report[0]:
        for i, line in enumerate(raw_report[1:]):
            if "Analysis Statistic" not in line and "System Info" not in line:
                    raw_checker_report.append(line)
            else:
                    checker_report = parse_dfree_checker_report(raw_checker_report)
                    report.dfree_checker_report = checker_report
                    break
        del raw_checker_report[:]
        raw_report = raw_report[i+1:]

    if "Double-Lock Analysis Statistic" in raw_report[0]:
        for i, line in enumerate(raw_report[1:]):
            if "Analysis Statistic" not in line and "System Info" not in line:
                    raw_checker_report.append(line)
            else:
                    checker_report = parse_dlock_checker_report(raw_checker_report)
                    report.dlock_checker_report = checker_report
                    break
        del raw_checker_report[:]
        raw_report = raw_report[i+1:]

    for i, line in enumerate(raw_report):
            if "Partitioner Statistic" not in line:
                    raw_system_info.append(line)
            else:
                    report.system_info = parse_system_info(raw_system_info)
                    break

    raw_report = raw_report[i:]

    for i, line in enumerate(raw_report):
            if "Blacklisted" not in line:
                    raw_partitioner_stat.append(line)
            else:
                    report.partitioner_stat = parse_partitioner_stat(raw_partitioner_stat, report.api_type)
                    break

    raw_report = raw_report[i:]

    for i, line in enumerate(raw_report):
            if "Sinks" not in line:
                    if "," in line:
                            report.blacklist.extend(line.split(","))
            else:
                    break

    raw_report = raw_report[i:]

    for i, line in enumerate(raw_report):
            if "Total" not in line:
                    if "," in line:
                            report.delete_functions.extend(line.split(","))
            else:
                    report.total_duration = float(get_last_word(line))
                    break

    return report


def parse_kernel_report(report_name: str):
    print("Parse KernelReport ...")

    try:
            with open(report_name) as report:
                    print("open file " + report_name)
                    kernel_report = KernelReport()
                    raw_kernel_report = report.readlines()
                    raw_kernel_report = [x.strip() for x in raw_kernel_report]
                    raw_report = list()

                    for line in raw_kernel_report:
                            if "REPORT" in line:
                                    if len(raw_report) > 1:
                                            report = parse_report(raw_report)
                                            if report.api_type != APIType.UNKOWN:
                                                    kernel_report.reports[report.getID()] = report
                                    del raw_report[:]

                            elif "Module" in line:
                                    tmp_module_name = get_last_word(line)
                                    assert(kernel_report.module_name == "" or kernel_report.module_name == tmp_module_name)
                                    kernel_report.module_name = tmp_module_name

                                    if re.search("v\d.*\.", tmp_module_name):
                                        tmp_version = re.search("v\d.*\.", tmp_module_name).group()[:-1]
                                    else:
                                        tmp_version = "v0.0"

                                    assert(kernel_report.version == "" or kernel_report.version == tmp_version)
                                    kernel_report.version = tmp_version
                            else:
                                    raw_report.append(line)

                    report = parse_report(raw_report)

                    if report.api_type != APIType.UNKOWN:
                            kernel_report.reports[report.getID()] = report

                    kernel_report.sort_reports()
                    return kernel_report
    except Exception as e:
            print("Error: " + str(e))
            return None
