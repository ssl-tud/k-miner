from collections import OrderedDict
from enum import Enum

FREE_ELECTRONS = "http://lxr.free-electrons.com/source/"


class APIType(Enum):
    SYSCALL = 1
    DRIVER = 2
    UNKOWN = 3


class MemCrpt(Enum):
    USE_AFTER_FREE = 1
    USE_AFTER_RETURN = 2
    DOUBLE_FREE = 3
    MEMORY_LEAK = 4
    DOUBLE_LOCK = 5
    UNKOWN = 6


class MemCrptVuln(object):
    TOTAL_NUM_IDs = 0

    def __init__(self, type: MemCrpt):
            self.type = type
            self.ID = MemCrptVuln.TOTAL_NUM_IDs
            self.status = "NotChecked"
            MemCrptVuln.TOTAL_NUM_IDs += 1

    def is_not_checked(self):
            return self.status == "NotChecked"

    def is_false_positive(self):
            return self.status == "False"

    def is_positive(self):
            return self.status == "Positive"

    def is_removed(self):
            return self.status == "Remove"

    def set_status(self, status: str="NotChecked"):
            self.status = status


class UseAfterFree(MemCrptVuln):
    def __init__(self):
            super().__init__(MemCrpt.USE_AFTER_FREE)
            self.dangling_ptr = None
            self.free = None
            self.use = None
            self.api_path = list()
            self.free_path = list()
            self.use_path = list()
            self.function_locs = dict()
            self.duration = 0

    def get_obj(self):
            return self.dangling_ptr

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.dangling_ptr == other.dangling_ptr and self.free == other.free and self.use == other.use
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash((self.dangling_ptr, self.free, self.use))

    def __str__(self):
            tmp_str = "UseAfterFree:\n"
            tmp_str += ("DanglingPtr=" + str(self.dangling_ptr) + "\n")
            tmp_str += ("Free=" + str(self.free) + "\n")
            tmp_str += ("Use=" + str(self.use) + "\n")
            tmp_str += (str(self.api_path) + "\n")
            tmp_str += (str(self.free_path) + "\n")
            tmp_str += (str(self.use_path) + "\n")

            for name, loc in self.function_locs.items():
                    tmp_str += (str(loc) + "\n")

            tmp_str += str(self.duration)

            return tmp_str


class UseAfterReturn(MemCrptVuln):
    def __init__(self):
            super().__init__(MemCrpt.USE_AFTER_RETURN)
            self.dangling_ptr = None
            self.local_var = None
            self.api_path = list()
            self.lvar_to_oos_path = list()
            self.dptr_to_oos_path = list()
            self.function_locs = dict()
            self.duration = 0

    def get_obj(self):
            return self.dangling_ptr

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.dangling_ptr == other.dangling_ptr and self.local_var == other.local_var
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash(tuple([self.dangling_ptr, self.local_var]))

    def __str__(self):
            tmp_str = "UseAfterReturn:\n"
            tmp_str += ("DanglingPtr=" + str(self.dangling_ptr) + "\n")
            tmp_str += ("LocalVar=" + str(self.local_var) + "\n\n")
            tmp_str += (str(self.api_path) + "\n\n")
            tmp_str += (str(self.lvar_to_oos_path) + "\n\n")
            tmp_str += (str(self.dptr_to_oos_path) + "\n\n")

            for name, loc in self.function_locs.items():
                    tmp_str += (str(loc) + "\n")

            tmp_str += str(self.duration)

            return tmp_str


class DoubleFree(MemCrptVuln):
    def __init__(self):
            super().__init__(MemCrpt.DOUBLE_FREE)
            self.dangling_ptr = None
            self.free1 = None
            self.free2 = None
            self.api_path = list()
            self.free_path1 = list()
            self.free_path2 = list()
            self.function_locs = dict()
            self.duration = 0

    def get_obj(self):
            return self.dangling_ptr

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.dangling_ptr == other.dangling_ptr and self.free1 == other.free1 and self.free2 == other.free2
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash((self.dangling_ptr, self.free1, self.free2))

    def __str__(self):
            tmp_str = "DoubleFree:\n"
            tmp_str += ("DanglingPtr=" + str(self.dangling_ptr) + "\n")
            tmp_str += ("Free1=" + str(self.free1) + "\n")
            tmp_str += ("Free2=" + str(self.free2) + "\n")
            tmp_str += (str(self.api_path) + "\n\n")
            tmp_str += (str(self.free_path1) + "\n\n")
            tmp_str += (str(self.free_path2) + "\n\n")

            for name, loc in self.function_locs.items():
                    tmp_str += (str(loc) + "\n")

            tmp_str += str(self.duration)

            return tmp_str


class MemoryLeak(MemCrptVuln):
    def __init__(self):
            super().__init__(MemCrpt.MEMORY_LEAK)
            self.leak_ptr = None
            self.never_free = False
            self.api_path = list()
            self.duration = 0

    def get_obj(self):
            return self.leak_ptr

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.leak_ptr == other.leak_ptr and self.never_free == other.never_free
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash(tuple([self.leak_ptr, self.never_free]))

    def __str__(self):
            tmp_str = "MemoryLeak:\n"
            tmp_str += ("DanglingPtr=" + str(self.leak_ptr) + "\n")
            tmp_str += (str(self.never_free) + "\n")
            tmp_str += (str(self.api_path) + "\n\n")
            tmp_str += str(self.duration)
            return tmp_str


class DoubleLock(MemCrptVuln):
    def __init__(self):
            super().__init__(MemCrpt.DOUBLE_LOCK)
            self.lock_var = None
            self.lock1 = None
            self.lock2 = None
            self.api_path = list()
            self.lock_path1 = list()
            self.lock_path2 = list()
            self.function_locs = dict()
            self.duration = 0

    def get_obj(self):
            return self.lock_var

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return hash((self.lock_var, self.lock1, self.lock2))
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash((self.lock_var, self.lock1, self.lock2))

    def __str__(self):
            tmp_str = "DoubleLock:\n"
            tmp_str += ("Lock=" + str(self.lock_var) + "\n")
            tmp_str += (str(self.api_path) + "\n\n")
            tmp_str += (str(self.lock_path1) + "\n\n")
            tmp_str += (str(self.lock_path2) + "\n\n")

            for name, loc in self.function_locs.items():
                    tmp_str += (str(loc) + "\n")

            tmp_str += str(self.duration)

            return tmp_str


class CheckerReport(object):
    def __init__(self):
            self.duration = 0
            self.num_bugs = 0
            self.bugs = list()

    def get_num_bugs(self):
            return len(self.bugs)

    def __str__(self):
            return str(self.num_bugs) + " bugs in " + str(self.duration) + " sec"


class UseAfterFreeReport(CheckerReport):
    def __init__(self):
            super().__init__()
            self.num_analyzed_var = 0

    def __str__(self):
            tmp_str = "UseAfterFreeReport:" + super(UseAfterFreeReport, self).__str__() + "\n"
            tmp_str += ("Num Analyzed Variables=" + str(self.num_analyzed_var) + "\n")

            for b in self.bugs:
                    tmp_str += (str(b) + "\n")

            return tmp_str


class UseAfterReturnReport(CheckerReport):
    def __init__(self):
            super().__init__()
            self.timeout = 0
            self.num_analyzed_var = 0

    def __str__(self):
            tmp_str = "UseAfterReturnReport:" + super(UseAfterReturnReport, self).__str__() + "\n"
            tmp_str += ("Timeout=" + str(self.timeout) + "\n")
            tmp_str += ("Num Analyzed Variables=" + str(self.num_analyzed_var) + "\n")

            for b in self.bugs:
                    tmp_str += (str(b) + "\n")

            return tmp_str


class DoubleFreeReport(CheckerReport):
    def __init__(self):
            super().__init__()
            self.num_analyzed_var = 0

    def __str__(self):
            tmp_str = "DoubleFreeReport:" + super(DoubleFreeReport, self).__str__() + "\n"
            tmp_str += ("Num Analyzed Variables=" + str(self.num_analyzed_var) + "\n")

            for b in self.bugs:
                    tmp_str += (str(b) + "\n")

            return tmp_str


class MemoryLeakReport(CheckerReport):
    def __init__(self):
            super().__init__()
            self.num_analyzed_var = 0

    def __str__(self):
            tmp_str = "MemoryLeakReport:" + super(MemoryLeakReport, self).__str__() + "\n"
            tmp_str += ("Num Analyzed Variables=" + str(self.num_analyzed_var) + "\n")

            for b in self.bugs:
                    tmp_str += (str(b) + "\n")

            return tmp_str


class DoubleLockReport(CheckerReport):
    def __init__(self):
            super().__init__()
            self.num_analyzed_var = 0

    def __str__(self):
            tmp_str = "DoubleLockReport:" + super(DoubleLockReport, self).__str__() + "\n"
            tmp_str += ("Num Analyzed Variables=" + str(self.num_analyzed_var) + "\n")

            for b in self.bugs:
                    tmp_str += (str(b) + "\n")

            return tmp_str


# class CheckerReport(object):
#    def __init__(self):
#            self.duration = 0
#            self.num_bugs = 0
#
#    def __str__(self):
#            return str(self.num_bugs) + " bugs in " + str(self.duration) + " sec"


class Context(object):
    def __init__(self, function_name: str="", file_name: str="", line: int=0):
            self.function_name = function_name
            self.file_name = file_name
            self.line = line
            self.link = None

    def getLink(self, kernel_version: str):
            return FREE_ELECTRONS + self.file_name + "?" + kernel_version.replace("v", "v=") + "#L" + str(self.line)

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.__dict__ == other.__dict__
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash(tuple(sorted(self.__dict__.items())))

    def __str__(self):
            return self.function_name + " in " + self.file_name + " (ln:" + str(self.line) + ")"


class Var(object):
    def __init__(self, name: str="", loc: Context=None):
            self.name = name
            self.loc = loc

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                    return self.__dict__ == other.__dict__
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                    return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash(tuple(sorted(self.__dict__.items())))

    def __str__(self):
            return self.name + " at " + str(self.loc)


class Function(Context):
    def __init__(self):
            super.__init__()


class PortableMemoryLeak(object):
    def __init__(self, type: str, dangling_ptr: Var, is_leak: bool=False):
            self.type = type
            self.dangling_ptr = dangling_ptr
            self.is_leak = is_leak

    @staticmethod
    def get_file_name():
            return "MemoryLeak.json"

    def as_dict(self):
            return dict({"type": str(self.type),
                         "ptr_name": str(self.dangling_ptr.name),
                         "loc_function_name": str(self.dangling_ptr.loc.function_name),
                         "loc_file_name": str(self.dangling_ptr.loc.file_name),
                         "loc_line": str(self.dangling_ptr.loc.line),
                         "is_leak": self.is_leak})

    def __eq__(self, other):
            """Override the default Equals behavior"""
            if isinstance(other, self.__class__):
                            return self.type == other.type and self.dangling_ptr == other.dangling_ptr
            return NotImplemented

    def __ne__(self, other):
            """Define a non-equality test"""
            if isinstance(other, self.__class__):
                            return not self.__eq__(other)
            return NotImplemented

    def __hash__(self):
            """Override the default hash behavior (that returns the id or the object)"""
            return hash(tuple([self.type, self.dangling_ptr]))


class SystemInfo(object):
    def __init__(self):
            self.kernel_name = ""
            self.kernel_release = ""
            self.kernel_version = ""
            self.processor = ""
            self.num_cores = 0
            self.mem_usage= 0
            self.mem_max_usage= 0
            self.mem_total = 0

    def __str__(self):
            tmp_str = "SystemInfo:\n"
            tmp_str += (self.kernel_name + "\n")
            tmp_str += (self.kernel_release + "\n")
            tmp_str += (self.kernel_version + "\n")
            tmp_str += (self.processor + "\n")
            tmp_str += (str(self.num_cores) + "\n")
            tmp_str += (str(self.mem_usage) + "\n")
            tmp_str += (str(self.mem_max_usage) + "\n")
            tmp_str += (str(self.mem_total) + "\n")
            return tmp_str


class PartitionerStat(object):
    def __init__(self, type: APIType):
            self.type = type
            # Initcall Stat
            self.numIO = 0
            self.numIR = 0

            self.numIOFuncs = 0
            self.numIRFuncs = 0

            self.numIOGVs = 0
            self.numIRGVs = 0

            self.numIONDVs = 0
            self.numIRNDVs = 0

            self.iOCGDepth = 0
            self.iRCGDepth = 0

            # API (Syscall/Driver) Stat
            self.numAOFuncs = 0
            self.numARFuncs = 0

            self.numAOGVs = 0
            self.numARGVs = 0

            self.aOCGDepth = 0
            self.aRCGDepth = 0

            # Kernel Stat
            self.numKOFuncs = 0
            self.numKRFuncs = 0

            self.numKOGVs = 0
            self.numKRGVs = 0

            self.api_func_limit = 0
            self.initcall_func_limit = 0

    def __str__(self):
            tmp_str = "PartitionerStat:\n"
            tmp_str += ("Num Initcalls: " + str(self.numIR) + "/" + str(self.numIO) + "\n")
            tmp_str += ("Num Initcall Funcs: " + str(self.numIRFuncs) + "/" + str(self.numIOFuncs) + "\n")
            tmp_str += ("Num Initcall GVs: " + str(self.numIRGVs) + "/" + str(self.numIOGVs) + "\n")
            tmp_str += ("Num Initcall NGVs: " + str(self.numIRNDVs) + "/" + str(self.numIONDVs) + "\n")
            tmp_str += ("Initcall CGDepth: " + str(self.iRCGDepth) + "/" + str(self.iOCGDepth) + "\n")

            tmp_str += ("Num " + self.type.name + " Funcs: " + str(self.numARFuncs) + "/" + str(self.numAOFuncs) + "\n")
            tmp_str += ("Num " + self.type.name + " GVs: " + str(self.numARGVs) + "/" + str(self.numAOGVs) + "\n")
            tmp_str += (self.type.name + " CGDepth: " + str(self.aRCGDepth) + "/" + str(self.aOCGDepth) + "\n")

            tmp_str += ("Num Kernel Funcs: " + str(self.numKRFuncs) + "/" + str(self.numKOFuncs) + "\n")
            tmp_str += ("Num Kernel GVs: " + str(self.numKRGVs) + "/" + str(self.numKOGVs) + "\n")

            tmp_str += (self.type.name + "Limit= " + str(self.api_func_limit) + "\n")
            tmp_str += ("InitcallLimit= " + str(self.initcall_func_limit) + "\n")

            return tmp_str


class Comment(object):
    def __init__(self, ID: str, text: str, date: str):
            self.ID = ID
            self.text = text
            self.date = date

    def get_kernel_version(self):
            return self.ID.split(":")[0]

    def get_systemcall(self):
            return self.ID.split(":")[1]

    def get_local_id(self):
            return self.ID.split(":")[2]

    def get_html_printable_text(self):
            html_newline = "<br>"
            return self.text.replace("\r\n", html_newline).replace("\n\r", html_newline).replace("\r", html_newline).replace("\n", html_newline)

    def __str__(self):
            return "ID: " + self.ID + " Text: <" + self.text + "> " + self.date

    @staticmethod
    def get_file_name():
            return "comment_db.json"


class Report(object):
    def __init__(self):
            self.api_type = APIType.UNKOWN
            self.syscall_name = ""
            self.driver_name = ""
            self.num_threads = 0
            self.uaf_checker_report = UseAfterFreeReport()
            self.uar_checker_report = UseAfterReturnReport()
            self.dfree_checker_report = DoubleFreeReport()
            self.leak_checker_report = MemoryLeakReport()
            self.dlock_checker_report = DoubleLockReport()
            self.partitioner_stat = None
            self.system_info = None
            self.blacklist = list()
            self.delete_functions = list()
            self.total_duration = 0
            self.comments = list()

    def getID(self):
        if(self.api_type == APIType.SYSCALL):
            return self.syscall_name
        elif(self.api_type == APIType.DRIVER):
            return self.driver_name
        else:
            return "NA"

    def is_syscall_report(self):
        return self.api_type == APIType.SYSCALL

    def is_driver_report(self):
        return self.api_type == APIType.DRIVER

    def get_num_bugs(self):
            return self.uar_checker_report.get_num_bugs() + self.uaf_checker_report.get_num_bugs() + self.dfree_checker_report.get_num_bugs()

    def get_num_uaf(self):
            return self.uaf_checker_report.get_num_bugs()

    def get_num_uar(self):
            return self.uar_checker_report.get_num_bugs()

    def get_num_dfree(self):
            return self.dfree_checker_report.get_num_bugs()

    def get_num_leaks(self):
            return self.leak_checker_report.get_num_bugs()

    def get_num_dlock(self):
            return self.dlock_checker_report.get_num_bugs()

    def add_comment(self, new_comment: Comment):
            self.comments.append(new_comment)

    def get_schedule_map(self):
            schedule = OrderedDict()
            schedule["UAR"] = self.uar_checker_report.duration
            schedule["LEAK"] = self.leak_checker_report.duration
            schedule["DFREE"] = self.dfree_checker_report.duration
            return schedule

    def get_mem_usage(self):
            return self.system_info.mem_usage

    def get_mem_max_usage(self):
            return self.system_info.mem_max_usage

    def has_leak(self, wanted: str):
            for l in self.uar_checker_report.bugs:
                    if l.dangling_ptr.loc.function_name == wanted or \
                            l.local_var.loc.function_name == wanted or \
                            (wanted.isdigit() and l.ID == int(wanted)):
                            return True
            for l in self.uaf_checker_report.bugs:
                    if l.dangling_ptr.loc.function_name == wanted or \
                            (wanted.isdigit() and l.ID == int(wanted)):
                            return True
            for l in self.leak_checker_report.bugs:
                    if l.leak_ptr.loc.function_name == wanted or \
                            (wanted.isdigit() and l.ID == int(wanted)):
                            return True
            for l in self.dfree_checker_report.bugs:
                    if l.dangling_ptr.loc.function_name == wanted or \
                            (wanted.isdigit() and l.ID == int(wanted)):
                            return True
            for l in self.dlock_checker_report.bugs:
                    if l.lock_var.loc.function_name == wanted or \
                            (wanted.isdigit() and l.ID == int(wanted)):
                            return True
            return False

    def __str__(self):
            tmp_str = "Report:\n"
            tmp_str += "APIType= " + self.api_type.name + "\n"
            tmp_str += ("System call= " + self.syscall_name + "\n")
            tmp_str += ("Driver= " + self.driver_name + "\n")
            tmp_str += ("NumThreads= " + str(self.num_threads) + "\n")
            tmp_str += "--------------------\n"
            tmp_str += (str(self.uaf_checker_report) + "\n")
            tmp_str += "--------------------\n"
            tmp_str += (str(self.uar_checker_report) + "\n")
            tmp_str += "--------------------\n"
            tmp_str += (str(self.dfree_checker_report) + "\n")
            tmp_str += "--------------------\n"
            tmp_str += (str(self.leak_checker_report) + "\n")
            tmp_str += "--------------------\n"
            tmp_str += (str(self.dlock_checker_report) + "\n")
            tmp_str += "--------------------\n"

            tmp_str += "=====================\n"
            tmp_str += (str(self.partitioner_stat) + "\n\n")
            tmp_str += "=====================\n"
            tmp_str += (str(self.system_info) + "\n\n")
            tmp_str += ("Blacklist: " + str(self.blacklist) + "\n\n")
            tmp_str += ("DeleteFunctions: " + str(self.delete_functions) + "\n\n")
            tmp_str += ("Duration= " + str(self.total_duration) + "\n")

            return tmp_str


class KernelReport(object):
    def __init__(self):
            self.version = ""
            self.module_name = ""
            self.reports = OrderedDict()
            self.all_memory_corruptions = OrderedDict()

    def get_file_name(self):
            return "KernelReport_" + self.version + ".json"

    def get_syscall_reports(self):
            reports = list()

            for name, report in self.reports.items():
                if report.is_syscall_report():
                    reports.append(report)

            return reports

    def get_driver_reports(self):
            reports = list()

            for name, report in self.reports.items():
                if report.is_driver_report():
                    reports.append(report)

            return reports

    def sort_reports(self):
            tmp_dict = self.reports.items()
            self.reports = OrderedDict(sorted(tmp_dict))

    def get_prev_report(self, cur_report: Report, leak_only: bool):
            if not cur_report:
                    return None

            reverse_reports = OrderedDict(reversed(list(self.reports.items())))

            for i, sn in enumerate(reverse_reports.keys()):
                    if sn == cur_report.syscall_name:
                            prev_report = None

                            if i == len(reverse_reports) - 1:
                                    prev_report = list(reverse_reports.values())[1]
                            else:
                                    prev_report = list(reverse_reports.values())[i + 1]

                            if leak_only and prev_report.get_num_bugs() <= 0:
                                    cur_report = prev_report
                            else:
                                    return prev_report

            return cur_report

    def get_next_report(self, cur_report: Report, leak_only: bool):
            if not cur_report:
                    return None

            for i, sn in enumerate(self.reports.keys()):
                    if sn == cur_report.syscall_name:
                            next_report = None

                            if i == len(self.reports) - 1:
                                    next_report = list(self.reports.values())[1]
                            else:
                                    next_report = list(self.reports.values())[i + 1]

                            if leak_only and next_report.get_num_bugs() <= 0:
                                    cur_report = next_report
                            else:
                                    return next_report

            return cur_report

    def get_memory_corruptions(self):
            if not self.all_memory_corruptions:
                    self.all_memory_corruptions = OrderedDict(list())

            if len(self.all_memory_corruptions.values()) > 1:
                    return self.all_memory_corruptions

            for sr in self.reports.values():
                    for l in sr.uaf_checker_report.bugs:
                            lst = self.all_memory_corruptions.setdefault("UAF", [])
                            if l not in lst and not l.is_removed():
                                    lst.append(l)
                    for l in sr.leak_checker_report.bugs:
                            lst = self.all_memory_corruptions.setdefault("LEAK", [])
                            if l not in lst and not l.is_removed():
                                    lst.append(l)
                    for l in sr.uar_checker_report.bugs:
                            lst = self.all_memory_corruptions.setdefault("UAR", [])
                            if l not in lst and not l.is_removed():
                                    lst.append(l)
                    for l in sr.dfree_checker_report.bugs:
                            lst = self.all_memory_corruptions.setdefault("DFREE", [])
                            if l not in lst and not l.is_removed():
                                    lst.append(l)
                    for l in sr.dlock_checker_report.bugs:
                            lst = self.all_memory_corruptions.setdefault("DLOCK", [])
                            if l not in lst and not l.is_removed():
                                    lst.append(l)

            return self.all_memory_corruptions

    def get_actual_num_memory_corruptions(self, leak_type: str):
            num_actual_corruptions = 0
            corruptions = self.get_memory_corruptions()

            for ly, leaks in corruptions.items():
                    if not ly == leak_type and not leak_type == "TOTAL":
                            continue

                    for l in leaks:
                            if l.is_positive():
                                    num_actual_corruptions += 1

            return num_actual_corruptions

    def get_total_num_memory_corruptions(self, leak_type: str):
            corruptions = self.get_memory_corruptions()
            cnt_total = 0

            for ly, leaks in corruptions.items():
                    if ly == leak_type:
                            return len(leaks)
                    elif "TOTAL" == leak_type:
                            cnt_total += len(leaks)

            return cnt_total

    def delete_memory_corruption(self, mem_corruption: MemCrpt):
            corruptions = self.get_memory_corruptions()

            for ly, leaks in corruptions.items():
                    for l in leaks:
                            if mem_corruption.ID == l.ID:
                                    leaks.remove(l)
                                    return

    def get_average_duration(self):
            total_t = 0

            for sn, sr in self.reports.items():
                    total_t += float(sr.total_duration)

            return format(total_t / len(self.reports), ".2f")

    def __str__(self):
            tmp_str = "KernelReport:\n"
            tmp_str += ("Version= " + self.version + "\n")
            tmp_str += ("ModuleName= " + self.module_name + "\n")

            for name, report in self.reports.items():
                    tmp_str += (str(report) + "\n")
                    tmp_str += "#####################\n"

            return tmp_str
