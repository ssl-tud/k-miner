#include "Util/SyscallAPI.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>

static const SyscallAPI::SysTypeToStrPair sysTypeToFuncNameArry[] = {
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_READ, "sys_read"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_WRITE, "sys_write"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_OPEN, "sys_open"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOSE, "sys_close"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NEWSTAT, "sys_newstat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NEWFSTAT, "sys_newfstat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NEWLSTAT, "sys_newlstat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_POLL, "sys_poll"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LSEEK, "sys_lseek"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MMAP, "sys_mmap"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MPROTECT, "sys_mprotect"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MUNMAP, "sys_munmap"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_BRK, "sys_brk"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGACTION, "sys_rt_sigaction"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGPROCMASK, "sys_rt_sigprocmask"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGRETURN, "sys_rt_sigreturn"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IOCTL, "sys_ioctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PREAD64, "sys_pread64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PWRITE64, "sys_pwrite64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_READV, "sys_readv"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_WRITEV, "sys_writev"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ACCESS, "sys_access"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PIPE, "sys_pipe"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SELECT, "sys_select"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_YIELD, "sys_sched_yield"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MREMAP, "sys_mremap"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MSYNC, "sys_msync"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MINCORE, "sys_mincore"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MADVISE, "sys_madvise"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SHMGET, "sys_shmget"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SHMAT, "sys_shmat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SHMCTL, "sys_shmctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_DUP, "sys_dup"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_DUP2, "sys_dup2"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PAUSE, "sys_pause"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NANOSLEEP, "sys_nanosleep"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETITIMER, "sys_getitimer"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ALARM, "sys_alarm"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETITIMER, "sys_setitimer"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPID, "sys_getpid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SENDFILE64, "sys_sendfile64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SOCKET, "sys_socket"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CONNECT, "sys_connect"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ACCEPT, "sys_accept"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SENDTO, "sys_sendto"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RECVFROM, "sys_recvfrom"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SENDMSG, "sys_sendmsg"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RECVMSG, "sys_recvmsg"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SHUTDOWN, "sys_shutdown"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_BIND, "sys_bind"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LISTEN, "sys_listen"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETSOCKNAME, "sys_getsockname"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPEERNAME, "sys_getpeername"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SOCKETPAIR, "sys_socketpair"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETSOCKOPT, "sys_setsockopt"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETSOCKOPT, "sys_getsockopt"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLONE, "sys_clone"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FORK, "sys_fork"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_VFORK, "sys_vfork"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EXECVE, "sys_execve"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EXIT, "sys_exit"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_WAIT4, "sys_wait4"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_KILL, "sys_kill"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NEWUNAME, "sys_newuname"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SEMGET, "sys_semget"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SEMOP, "sys_semop"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SEMCTL, "sys_semctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SHMDT, "sys_shmdt"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MSGGET, "sys_msgget"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MSGSND, "sys_msgsnd"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MSGRCV, "sys_msgrcv"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MSGCTL, "sys_msgctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCNTL, "sys_fcntl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FLOCK, "sys_flock"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FSYNC, "sys_fsync"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FDATASYNC, "sys_fdatasync"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TRUNCATE, "sys_truncate"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FTRUNCATE, "sys_ftruncate"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETDENTS, "sys_getdents"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETCWD, "sys_getcwd"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CHDIR, "sys_chdir"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCHDIR, "sys_fchdir"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RENAME, "sys_rename"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MKDIR, "sys_mkdir"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RMDIR, "sys_rmdir"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CREAT, "sys_creat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LINK, "sys_link"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UNLINK, "sys_unlink"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYMLINK, "sys_symlink"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_READLINK, "sys_readlink"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CHMOD, "sys_chmod"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCHMOD, "sys_fchmod"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CHOWN, "sys_chown"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCHOWN, "sys_fchown"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LCHOWN, "sys_lchown"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UMASK, "sys_umask"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETTIMEOFDAY, "sys_gettimeofday"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETRLIMIT, "sys_getrlimit"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETRUSAGE, "sys_getrusage"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYSINFO, "sys_sysinfo"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMES, "sys_times"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PTRACE, "sys_ptrace"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETUID, "sys_getuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYSLOG, "sys_syslog"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETGID, "sys_getgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETUID, "sys_setuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETGID, "sys_setgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETEUID, "sys_geteuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETEGID, "sys_getegid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETPGID, "sys_setpgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPPID, "sys_getppid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPGRP, "sys_getpgrp"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETSID, "sys_setsid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETREUID, "sys_setreuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETREGID, "sys_setregid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETGROUPS, "sys_getgroups"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETGROUPS, "sys_setgroups"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETRESUID, "sys_setresuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETRESUID, "sys_getresuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETRESGID, "sys_setresgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETRESGID, "sys_getresgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPGID, "sys_getpgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETFSUID, "sys_setfsuid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETFSGID, "sys_setfsgid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETSID, "sys_getsid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CAPGET, "sys_capget"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CAPSET, "sys_capset"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGPENDING, "sys_rt_sigpending"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGTIMEDWAIT, "sys_rt_sigtimedwait"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGQUEUEINFO, "sys_rt_sigqueueinfo"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_SIGSUSPEND, "sys_rt_sigsuspend"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SIGALTSTACK, "sys_sigaltstack"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UTIME, "sys_utime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MKNOD, "sys_mknod"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PERSONALITY, "sys_personality"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_USTAT, "sys_ustat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_STATFS, "sys_statfs"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FSTATFS, "sys_fstatfs"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYSFS, "sys_sysfs"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETPRIORITY, "sys_getpriority"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETPRIORITY, "sys_setpriority"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_SETPARAM, "sys_sched_setparam"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_GETPARAM, "sys_sched_getparam"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_SETSCHEDULER, "sys_sched_setscheduler"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_GETSCHEDULER, "sys_sched_getscheduler"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_GET_PRIORITY_MAX, "sys_sched_get_priority_max"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_GET_PRIORITY_MIN, "sys_sched_get_priority_min"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_RR_GET_INTERVAL, "sys_sched_rr_get_interval"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MLOCK, "sys_mlock"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MUNLOCK, "sys_munlock"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MLOCKALL, "sys_mlockall"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MUNLOCKALL, "sys_munlockall"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_VHANGUP, "sys_vhangup"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MODIFY_LDT, "sys_modify_ldt"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PIVOT_ROOT, "sys_pivot_root"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYSCTL, "sys_sysctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PRCTL, "sys_prctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ARCH_PRCTL, "sys_arch_prctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ADJTIMEX, "sys_adjtimex"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETRLIMIT, "sys_setrlimit"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CHROOT, "sys_chroot"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYNC, "sys_sync"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ACCT, "sys_acct"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETTIMEOFDAY, "sys_settimeofday"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MOUNT, "sys_mount"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UMOUNT, "sys_umount"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SWAPON, "sys_swapon"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SWAPOFF, "sys_swapoff"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_REBOOT, "sys_reboot"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETHOSTNAME, "sys_sethostname"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETDOMAINNAME, "sys_setdomainname"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IOPL, "sys_iopl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IOPERM, "sys_ioperm"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_INIT_MODULE, "sys_init_module"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_DELETE_MODULE, "sys_delete_module"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_QUOTACTL, "sys_quotactl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETTID, "sys_gettid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_READAHEAD, "sys_readahead"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETXATTR, "sys_setxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LSETXATTR, "sys_lsetxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FSETXATTR, "sys_fsetxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETXATTR, "sys_getxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LGETXATTR, "sys_lgetxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FGETXATTR, "sys_fgetxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LISTXATTR, "sys_listxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LLISTXATTR, "sys_llistxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FLISTXATTR, "sys_flistxattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_REMOVEXATTR, "sys_removexattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LREMOVEXATTR, "sys_lremovexattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FREMOVEXATTR, "sys_fremovexattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TKILL, "sys_tkill"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIME, "sys_time"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FUTEX, "sys_futex"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_SETAFFINITY, "sys_sched_setaffinity"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SCHED_GETAFFINITY, "sys_sched_getaffinity"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IO_SETUP, "sys_io_setup"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IO_DESTROY, "sys_io_destroy"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IO_GETEVENTS, "sys_io_getevents"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IO_SUBMIT, "sys_io_submit"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IO_CANCEL, "sys_io_cancel"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LOOKUP_DCOOKIE, "sys_lookup_dcookie"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EPOLL_CREATE, "sys_epoll_create"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_REMAP_FILE_PAGES, "sys_remap_file_pages"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETDENTS64, "sys_getdents64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SET_TID_ADDRESS, "sys_set_tid_address"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RESTART_SYSCALL, "sys_restart_syscall"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SEMTIMEDOP, "sys_semtimedop"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FADVISE64, "sys_fadvise64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMER_CREATE, "sys_timer_create"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMER_SETTIME, "sys_timer_settime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMER_GETTIME, "sys_timer_gettime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMER_GETOVERRUN, "sys_timer_getoverrun"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMER_DELETE, "sys_timer_delete"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOCK_SETTIME, "sys_clock_settime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOCK_GETTIME, "sys_clock_gettime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOCK_GETRES, "sys_clock_getres"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOCK_NANOSLEEP, "sys_clock_nanosleep"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EXIT_GROUP, "sys_exit_group"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EPOLL_WAIT, "sys_epoll_wait"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EPOLL_CTL, "sys_epoll_ctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TGKILL, "sys_tgkill"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UTIMES, "sys_utimes"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MBIND, "sys_mbind"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SET_MEMPOLICY, "sys_set_mempolicy"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GET_MEMPOLICY, "sys_get_mempolicy"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_OPEN, "sys_mq_open"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_UNLINK, "sys_mq_unlink"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_TIMEDSEND, "sys_mq_timedsend"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_TIMEDRECEIVE, "sys_mq_timedreceive"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_NOTIFY, "sys_mq_notify"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MQ_GETSETATTR, "sys_mq_getsetattr"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_KEXEC_LOAD, "sys_kexec_load"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_WAITID, "sys_waitid"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ADD_KEY, "sys_add_key"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_REQUEST_KEY, "sys_request_key"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_KEYCTL, "sys_keyctl"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IOPRIO_SET, "sys_ioprio_set"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_IOPRIO_GET, "sys_ioprio_get"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_INOTIFY_INIT, "sys_inotify_init"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_INOTIFY_ADD_WATCH, "sys_inotify_add_watch"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_INOTIFY_RM_WATCH, "sys_inotify_rm_watch"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MIGRATE_PAGES, "sys_migrate_pages"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_OPENAT, "sys_openat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MKDIRAT, "sys_mkdirat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MKNODAT, "sys_mknodat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCHOWNAT, "sys_fchownat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FUTIMESAT, "sys_futimesat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NEWFSTATAT, "sys_newfstatat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UNLINKAT, "sys_unlinkat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RENAMEAT, "sys_renameat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_LINKAT, "sys_linkat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYMLINKAT, "sys_symlinkat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_READLINKAT, "sys_readlinkat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FCHMODAT, "sys_fchmodat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FACCESSAT, "sys_faccessat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PSELECT6, "sys_pselect6"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PPOLL, "sys_ppoll"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UNSHARE, "sys_unshare"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SET_ROBUST_LIST, "sys_set_robust_list"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GET_ROBUST_LIST, "sys_get_robust_list"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SPLICE, "sys_splice"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TEE, "sys_tee"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYNC_FILE_RANGE, "sys_sync_file_range"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_VMSPLICE, "sys_vmsplice"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_MOVE_PAGES, "sys_move_pages"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_UTIMENSAT, "sys_utimensat"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EPOLL_PWAIT, "sys_epoll_pwait"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SIGNALFD, "sys_signalfd"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMERFD_CREATE, "sys_timerfd_create"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EVENTFD, "sys_eventfd"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FALLOCATE, "sys_fallocate"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMERFD_SETTIME, "sys_timerfd_settime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TIMERFD_GETTIME, "sys_timerfd_gettime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_ACCEPT4, "sys_accept4"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SIGNALFD4, "sys_signalfd4"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EVENTFD2, "sys_eventfd2"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_EPOLL_CREATE1, "sys_epoll_create1"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_DUP3, "sys_dup3"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PIPE2, "sys_pipe2"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_INOTIFY_INIT1, "sys_inotify_init1"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PREADV, "sys_preadv"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PWRITEV, "sys_pwritev"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RT_TGSIGQUEUEINFO, "sys_rt_tgsigqueueinfo"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PERF_EVENT_OPEN, "sys_perf_event_open"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_RECVMMSG, "sys_recvmmsg"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FANOTIFY_INIT, "sys_fanotify_init"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FANOTIFY_MARK, "sys_fanotify_mark"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PRLIMIT64, "sys_prlimit64"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_NAME_TO_HANDLE_AT, "sys_name_to_handle_at"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_OPEN_BY_HANDLE_AT, "sys_open_by_handle_at"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_CLOCK_ADJTIME, "sys_clock_adjtime"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SYNCFS, "sys_syncfs"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SENDMMSG, "sys_sendmmsg"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_SETNS, "sys_setns"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_GETCPU, "sys_getcpu"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PROCESS_VM_READV, "sys_process_vm_readv"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_PROCESS_VM_WRITEV, "sys_process_vm_writev"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_KCMP, "sys_kcmp"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_FINIT_MODULE, "sys_finit_module"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_TEST, "sys_test"),
	SyscallAPI::SysTypeToStrPair(SyscallAPI::SYS_DUMMY, "")
};

// Related system calls
static const SyscallAPI::SysTypeSet syscallGroups[] = {
	// arch/x86/kernel/ioport.c
	{SyscallAPI::SYS_IOPL, 
	 SyscallAPI::SYS_IOPERM},

	// arch/x86/kernel/signal.c
	{SyscallAPI::SYS_RT_SIGRETURN},

	// arch/x86/kernel/sys_x86_64.c
	{SyscallAPI::SYS_MMAP},

	// arch/x86/um/ldt.c
	{SyscallAPI::SYS_MODIFY_LDT},

	// arch/x86/um/syscalls_64.c
	{SyscallAPI::SYS_ARCH_PRCTL},

	// fs/aio.c
	{SyscallAPI::SYS_IO_CANCEL, 
   	 SyscallAPI::SYS_IO_DESTROY, 
	 SyscallAPI::SYS_IO_GETEVENTS, 
	 SyscallAPI::SYS_IO_SETUP, 
	 SyscallAPI::SYS_IO_SUBMIT},

	// fs/dcache.c
	{SyscallAPI::SYS_GETCWD},

	// fs/dcookies.c
	{SyscallAPI::SYS_LOOKUP_DCOOKIE},

	// fs/eventfd.c
	{SyscallAPI::SYS_EVENTFD2,
	 SyscallAPI::SYS_EVENTFD,
	 SyscallAPI::SYS_EPOLL_CREATE1,
	 SyscallAPI::SYS_EPOLL_CREATE,
	 SyscallAPI::SYS_EPOLL_CTL,
	 SyscallAPI::SYS_EPOLL_PWAIT,
	 SyscallAPI::SYS_EPOLL_WAIT},

	// fs/exec.c
	{SyscallAPI::SYS_EXECVE},

	// fs/fcntl.c
	{SyscallAPI::SYS_FCNTL},

	// fs/fhandle.c
	{SyscallAPI::SYS_NAME_TO_HANDLE_AT, 
	SyscallAPI::SYS_OPEN_BY_HANDLE_AT},

	// fs/file.c
	{SyscallAPI::SYS_DUP2,
	SyscallAPI::SYS_DUP3,
	SyscallAPI::SYS_DUP},

	// fs/filesystems.c
	{SyscallAPI::SYS_SYSFS},

	// fs/ioctl.c
	{SyscallAPI::SYS_IOCTL},

	// fs/ioprio.c
	{SyscallAPI::SYS_IOPRIO_GET,
	SyscallAPI::SYS_IOPRIO_SET},

	// fs/locks.c
	{SyscallAPI::SYS_FLOCK},

	// fs/namei.c
	{SyscallAPI::SYS_LINKAT, 
	SyscallAPI::SYS_LINK, 
	SyscallAPI::SYS_MKDIRAT, 
	SyscallAPI::SYS_MKDIR, 
	SyscallAPI::SYS_MKNODAT, 
	SyscallAPI::SYS_MKNOD, 
	SyscallAPI::SYS_RENAMEAT, 
	SyscallAPI::SYS_RENAME, 
	SyscallAPI::SYS_RMDIR, 
	SyscallAPI::SYS_SYMLINKAT, 
	SyscallAPI::SYS_SYMLINK, 
	SyscallAPI::SYS_UNLINKAT, 
	SyscallAPI::SYS_UNLINK},

	// fs/namespace.c
	{SyscallAPI::SYS_MOUNT,
	SyscallAPI::SYS_PIVOT_ROOT,
       	SyscallAPI::SYS_UMOUNT},

	// fs/notify/fanotify/fanotify_user.c
	{SyscallAPI::SYS_FANOTIFY_INIT, 
	SyscallAPI::SYS_FANOTIFY_MARK, 
	SyscallAPI::SYS_INOTIFY_ADD_WATCH, 
	SyscallAPI::SYS_INOTIFY_INIT1, 
	SyscallAPI::SYS_INOTIFY_INIT, 
	SyscallAPI::SYS_INOTIFY_RM_WATCH},

	// fs/open.c
	{SyscallAPI::SYS_ACCESS, 
	SyscallAPI::SYS_CHDIR, 
	SyscallAPI::SYS_CHMOD, 
	SyscallAPI::SYS_CHOWN, 
	SyscallAPI::SYS_CHROOT, 
	SyscallAPI::SYS_CLOSE, 
	SyscallAPI::SYS_CREAT, 
	SyscallAPI::SYS_FACCESSAT, 
	SyscallAPI::SYS_FALLOCATE, 
	SyscallAPI::SYS_FCHDIR, 
	SyscallAPI::SYS_FCHMODAT, 
	SyscallAPI::SYS_FCHMOD, 
	SyscallAPI::SYS_FCHOWNAT, 
	SyscallAPI::SYS_FCHOWN, 
	SyscallAPI::SYS_FTRUNCATE, 
	SyscallAPI::SYS_LCHOWN, 
	SyscallAPI::SYS_OPENAT, 
	SyscallAPI::SYS_OPEN, 
	SyscallAPI::SYS_TRUNCATE, 
	SyscallAPI::SYS_VHANGUP},

	// fs/pipe.c
	{SyscallAPI::SYS_PIPE2, 
	SyscallAPI::SYS_PIPE},

	// fs/quota/quota.c
	{SyscallAPI::SYS_QUOTACTL},

	// fs/readdir.c
	{SyscallAPI::SYS_GETDENTS64,
	SyscallAPI::SYS_GETDENTS},

	// fs/read_write.c
	{SyscallAPI::SYS_LSEEK, 
	SyscallAPI::SYS_PREAD64, 
	SyscallAPI::SYS_PREADV, 
	SyscallAPI::SYS_PWRITE64, 
	SyscallAPI::SYS_PWRITEV, 
	SyscallAPI::SYS_READ, 
	SyscallAPI::SYS_READV, 
	SyscallAPI::SYS_SENDFILE64, 
	SyscallAPI::SYS_WRITE, 
	SyscallAPI::SYS_WRITEV},

	// fs/select.c
	{SyscallAPI::SYS_POLL, 
	SyscallAPI::SYS_PPOLL, 
	SyscallAPI::SYS_PSELECT6, 
	SyscallAPI::SYS_SELECT},

	// fs/signalfd.c
	{SyscallAPI::SYS_SIGNALFD4,
	SyscallAPI::SYS_SIGNALFD},

	// fs/splice.c
	{SyscallAPI::SYS_SPLICE, 
	SyscallAPI::SYS_TEE, 
	SyscallAPI::SYS_VMSPLICE},

	// fs/stat.c
	{SyscallAPI::SYS_NEWFSTATAT, 
	SyscallAPI::SYS_NEWFSTAT, 
	SyscallAPI::SYS_NEWLSTAT, 
	SyscallAPI::SYS_NEWSTAT, 
	SyscallAPI::SYS_READLINKAT, 
	SyscallAPI::SYS_READLINK},

	// fs/statfs.c
	{SyscallAPI::SYS_FSTATFS, 
	SyscallAPI::SYS_STATFS, 
	SyscallAPI::SYS_USTAT},

	// fs/sync.c
	{SyscallAPI::SYS_FDATASYNC, 
	SyscallAPI::SYS_FSYNC, 
	SyscallAPI::SYS_SYNC_FILE_RANGE, 
	SyscallAPI::SYS_SYNCFS, 
	SyscallAPI::SYS_SYNC},

	// fs/timerfd.c
	{SyscallAPI::SYS_TIMERFD_CREATE, 
	SyscallAPI::SYS_TIMERFD_GETTIME, 
	SyscallAPI::SYS_TIMERFD_SETTIME},

	// fs/utimes.c
	{SyscallAPI::SYS_FUTIMESAT, 
	SyscallAPI::SYS_UTIME, 
	SyscallAPI::SYS_UTIMENSAT, 
	SyscallAPI::SYS_UTIMES},

	// fs/xattr.c
	{SyscallAPI::SYS_FGETXATTR, 
	SyscallAPI::SYS_FLISTXATTR, 
	SyscallAPI::SYS_FREMOVEXATTR, 
	SyscallAPI::SYS_FSETXATTR, 
	SyscallAPI::SYS_GETXATTR, 
	SyscallAPI::SYS_LGETXATTR, 
	SyscallAPI::SYS_LISTXATTR, 
	SyscallAPI::SYS_LLISTXATTR, 
	SyscallAPI::SYS_LREMOVEXATTR, 
	SyscallAPI::SYS_LSETXATTR, 
	SyscallAPI::SYS_REMOVEXATTR, 
	SyscallAPI::SYS_SETXATTR},

	// ipc/mqueue.c
	{SyscallAPI::SYS_MQ_GETSETATTR, 
	SyscallAPI::SYS_MQ_NOTIFY, 
	SyscallAPI::SYS_MQ_OPEN, 
	SyscallAPI::SYS_MQ_TIMEDRECEIVE, 
	SyscallAPI::SYS_MQ_TIMEDSEND, 
	SyscallAPI::SYS_MQ_UNLINK},

	// ipc/msg.c
	{SyscallAPI::SYS_MSGCTL, 
	SyscallAPI::SYS_MSGGET, 
	SyscallAPI::SYS_MSGRCV, 
	SyscallAPI::SYS_MSGSND, 
	SyscallAPI::SYS_SEMCTL, 
	SyscallAPI::SYS_SEMGET, 
	SyscallAPI::SYS_SEMOP, 
	SyscallAPI::SYS_SEMTIMEDOP, 
	SyscallAPI::SYS_SHMAT, 
	SyscallAPI::SYS_SHMCTL, 
	SyscallAPI::SYS_SHMDT, 
	SyscallAPI::SYS_SHMGET},

	// kernel/acct.c
	{SyscallAPI::SYS_ACCT},

	// kernel/capability.c
	{SyscallAPI::SYS_CAPGET, 
	SyscallAPI::SYS_CAPSET},

	// kernel/events/core.c
	{SyscallAPI::SYS_PERF_EVENT_OPEN},

	// kernel/exec_domain.c
	{SyscallAPI::SYS_PERSONALITY},

	// kernel/exit.c
	{SyscallAPI::SYS_EXIT_GROUP, 
	SyscallAPI::SYS_EXIT, 
	SyscallAPI::SYS_WAIT4, 
	SyscallAPI::SYS_WAITID},

	// kernel/fork.c
	{SyscallAPI::SYS_CLONE, 
	SyscallAPI::SYS_FORK, 
	SyscallAPI::SYS_VFORK, 
	SyscallAPI::SYS_SET_TID_ADDRESS, 
	SyscallAPI::SYS_UNSHARE},

	// kernel/futex.c
	{SyscallAPI::SYS_FUTEX, 
	SyscallAPI::SYS_GET_ROBUST_LIST, 
	SyscallAPI::SYS_SET_ROBUST_LIST},

	// kernel/groups.c
	{SyscallAPI::SYS_GETGROUPS, 
	SyscallAPI::SYS_SETGROUPS},

	// kernel/hrtimer.c
	{SyscallAPI::SYS_NANOSLEEP},

	// kernel/itimer.c
	{SyscallAPI::SYS_GETITIMER, 
	SyscallAPI::SYS_SETITIMER},

	// kernel/kcmp.c
	{SyscallAPI::SYS_KCMP},

	// kernel/kexec.c
	{SyscallAPI::SYS_KEXEC_LOAD},

	// kernel/module.c
	{SyscallAPI::SYS_DELETE_MODULE, 
	SyscallAPI::SYS_FINIT_MODULE, 
	SyscallAPI::SYS_INIT_MODULE},

	// kernel/nsproxy.c
	{SyscallAPI::SYS_SETNS},

	// kernel/posix-timers.c
	{SyscallAPI::SYS_CLOCK_ADJTIME, 
	SyscallAPI::SYS_CLOCK_GETRES, 
	SyscallAPI::SYS_CLOCK_GETTIME, 
	SyscallAPI::SYS_CLOCK_NANOSLEEP, 
	SyscallAPI::SYS_CLOCK_SETTIME, 
	SyscallAPI::SYS_TIMER_CREATE, 
	SyscallAPI::SYS_TIMER_DELETE, 
	SyscallAPI::SYS_TIMER_GETOVERRUN, 
	SyscallAPI::SYS_TIMER_GETTIME, 
	SyscallAPI::SYS_TIMER_SETTIME},

	// kernel/printk/printk.c
	{SyscallAPI::SYS_SYSLOG},

	// kernel/ptrace.c
	{SyscallAPI::SYS_PTRACE},

	// kernel/reboot.c
	{SyscallAPI::SYS_REBOOT},

	// kernel/sched/core.c
	{SyscallAPI::SYS_SCHED_GETAFFINITY, 
	SyscallAPI::SYS_SCHED_GETPARAM, 
	SyscallAPI::SYS_SCHED_GET_PRIORITY_MAX, 
	SyscallAPI::SYS_SCHED_GET_PRIORITY_MIN, 
	SyscallAPI::SYS_SCHED_GETSCHEDULER, 
	SyscallAPI::SYS_SCHED_RR_GET_INTERVAL, 
	SyscallAPI::SYS_SCHED_SETAFFINITY, 
	SyscallAPI::SYS_SCHED_SETPARAM, 
	SyscallAPI::SYS_SCHED_SETSCHEDULER, 
	SyscallAPI::SYS_SCHED_YIELD},

	// kernel/signal.c
	{SyscallAPI::SYS_KILL, 
	SyscallAPI::SYS_PAUSE, 
	SyscallAPI::SYS_RESTART_SYSCALL, 
	SyscallAPI::SYS_RT_SIGACTION, 
	SyscallAPI::SYS_RT_SIGPENDING, 
	SyscallAPI::SYS_RT_SIGPROCMASK, 
	SyscallAPI::SYS_RT_SIGQUEUEINFO, 
	SyscallAPI::SYS_RT_SIGSUSPEND, 
	SyscallAPI::SYS_RT_SIGTIMEDWAIT, 
	SyscallAPI::SYS_RT_TGSIGQUEUEINFO, 
	SyscallAPI::SYS_SIGALTSTACK, 
	SyscallAPI::SYS_TGKILL, 
	SyscallAPI::SYS_TKILL},

	// kernel/sys.c
	{SyscallAPI::SYS_GETCPU, 
	SyscallAPI::SYS_GETEGID, 
	SyscallAPI::SYS_GETEUID, 
	SyscallAPI::SYS_GETGID, 
	SyscallAPI::SYS_GETPGID, 
	SyscallAPI::SYS_GETPGRP, 
	SyscallAPI::SYS_GETPID, 
	SyscallAPI::SYS_GETPPID, 
	SyscallAPI::SYS_GETPRIORITY, 
	SyscallAPI::SYS_GETRESGID, 
	SyscallAPI::SYS_GETRESUID, 
	SyscallAPI::SYS_GETRLIMIT, 
	SyscallAPI::SYS_GETRUSAGE, 
	SyscallAPI::SYS_GETSID, 
	SyscallAPI::SYS_GETTID, 
	SyscallAPI::SYS_GETUID, 
	SyscallAPI::SYS_NEWUNAME, 
	SyscallAPI::SYS_PRCTL, 
	SyscallAPI::SYS_PRLIMIT64, 
	SyscallAPI::SYS_SETDOMAINNAME, 
	SyscallAPI::SYS_SETFSGID, 
	SyscallAPI::SYS_SETFSUID, 
	SyscallAPI::SYS_SETGID, 
	SyscallAPI::SYS_SETHOSTNAME, 
	SyscallAPI::SYS_SETPGID, 
	SyscallAPI::SYS_SETPRIORITY, 
	SyscallAPI::SYS_SETREGID, 
	SyscallAPI::SYS_SETRESGID, 
	SyscallAPI::SYS_SETRESUID, 
	SyscallAPI::SYS_SETREUID, 
	SyscallAPI::SYS_SETRLIMIT, 
	SyscallAPI::SYS_SETSID, 
	SyscallAPI::SYS_SETUID, 
	SyscallAPI::SYS_SYSINFO, 
	SyscallAPI::SYS_TIMES, 
	SyscallAPI::SYS_UMASK},

	// kernel/sysctl_binary.c
	{SyscallAPI::SYS_SYSCTL},

	// kernel/time.c
	{SyscallAPI::SYS_ADJTIMEX, 
	SyscallAPI::SYS_GETTIMEOFDAY, 
	SyscallAPI::SYS_SETTIMEOFDAY, 
	SyscallAPI::SYS_TIME},

	// kernel/timer.c
	{SyscallAPI::SYS_ALARM},

	// mm/fadvise.c
	{SyscallAPI::SYS_FADVISE64},

	// mm/fremap.c
	{SyscallAPI::SYS_REMAP_FILE_PAGES},

	// mm/madvise.c
	{SyscallAPI::SYS_MADVISE},

	// mm/mempolicy.c
	{SyscallAPI::SYS_GET_MEMPOLICY, 
	SyscallAPI::SYS_MBIND, 
	SyscallAPI::SYS_MIGRATE_PAGES, 
	SyscallAPI::SYS_SET_MEMPOLICY},

	// mm/migrate.c
	{SyscallAPI::SYS_MOVE_PAGES},

	// mm/mincore.c
	{SyscallAPI::SYS_MINCORE},

	// mm/mlock.c
	{SyscallAPI::SYS_MLOCKALL, 
	SyscallAPI::SYS_MLOCK, 
	SyscallAPI::SYS_MUNLOCKALL, 
	SyscallAPI::SYS_MUNLOCK},

	// mm/mmap.c
	{SyscallAPI::SYS_BRK, 
	SyscallAPI::SYS_MREMAP,
	SyscallAPI::SYS_MUNMAP},

	// mm/mprotect.c
	{SyscallAPI::SYS_MPROTECT},

	// mm/msync.c
	{SyscallAPI::SYS_MSYNC},

	// mm/process_vm_access.c
	{SyscallAPI::SYS_PROCESS_VM_READV,
	SyscallAPI::SYS_PROCESS_VM_WRITEV},

	// mm/readahead.c
	{SyscallAPI::SYS_READAHEAD},

	// mm/swapfile.c
	{SyscallAPI::SYS_SWAPOFF,
	SyscallAPI::SYS_SWAPON},

	// net/socket.c
	{SyscallAPI::SYS_ACCEPT4,
	SyscallAPI::SYS_ACCEPT,
	SyscallAPI::SYS_BIND,
	SyscallAPI::SYS_CONNECT,
	SyscallAPI::SYS_GETPEERNAME,
	SyscallAPI::SYS_GETSOCKNAME,
	SyscallAPI::SYS_GETSOCKOPT, 
	SyscallAPI::SYS_LISTEN, 
	SyscallAPI::SYS_RECVFROM, 
	SyscallAPI::SYS_RECVMMSG, 
	SyscallAPI::SYS_RECVMSG, 
	SyscallAPI::SYS_SENDMMSG, 
	SyscallAPI::SYS_SENDMSG, 
	SyscallAPI::SYS_SENDTO, 
	SyscallAPI::SYS_SETSOCKOPT, 
	SyscallAPI::SYS_SHUTDOWN, 
	SyscallAPI::SYS_SOCKET, 
	SyscallAPI::SYS_SOCKETPAIR},

	// security/keys/keyctl.c
	{SyscallAPI::SYS_ADD_KEY, 
	SyscallAPI::SYS_KEYCTL, 
	SyscallAPI::SYS_REQUEST_KEY},

	{SyscallAPI::SYS_TEST,
	SyscallAPI::SYS_DUMMY}
};

SyscallAPI* SyscallAPI::syscallAPI = nullptr;

void SyscallAPI::init() {
	int size = sizeof(sysTypeToFuncNameArry)/sizeof(*sysTypeToFuncNameArry);

	for(int i=0; i < size; i++) {
		SysTypeToStrPair stsp= sysTypeToFuncNameArry[i];
		sysTypeToFuncNameMap[stsp.first] = stsp.second;
		funcNameToSysTypeMap[stsp.second] = stsp.first;
	}

	size = sizeof(syscallGroups)/sizeof(*syscallGroups);
	StringSet syscallNameSingleGroup;

	for(int i=0; i < size; i++) {
		SysTypeSet syscallGroup = syscallGroups[i];
		syscallGroupList.push_back(syscallGroup);
		StringSet syscallNameGroup;

		for(auto iter = syscallGroup.begin(); iter != syscallGroup.end(); ++iter) {
			std::string syscallName = sysTypeToFuncNameMap[*iter];
			syscallNameGroup.insert(syscallName);	

			StringSet syscallNameSingleGroup;
			syscallNameSingleGroup.insert(syscallName);
			syscallNames.push_back(syscallNameSingleGroup);
		}

		syscallNameGroupList.push_back(syscallNameGroup);
	}
}

bool SyscallAPI::isSyscall(std::string syscall) const {
	return funcNameToSysTypeMap.find(syscall) != funcNameToSysTypeMap.end();
}

SyscallAPI::SysTypeSet SyscallAPI::getRelatedSyscalls(std::string syscall) {
	int size = sizeof(syscallGroups)/sizeof(*syscallGroups);
	SYS_TYPE tmpSyscall = funcNameToSysTypeMap[syscall];

	for(int i=0; i < size; i++) {
		SysTypeSet syscalls= syscallGroups[i];

		if(syscalls.find(tmpSyscall) != syscalls.end())
			return syscalls;
	}

	// Return dummy.
	return syscallGroups[size-1];
}

StringSet SyscallAPI::getRelatedSyscallNames(std::string syscall) {
	for(auto iter = syscallNameGroupList.begin(); iter != syscallNameGroupList.end(); ++iter) {
		const StringSet &syscallNameGroup = *iter;
		
		if(syscallNameGroup.find(syscall) != syscallNameGroup.end())
			return syscallNameGroup;	
	}

	return syscallNameGroupList.back();
}

SyscallAPI::StringSetList SyscallAPI::getRelatedSyscallNamesByFile(std::string fileName, bool group) {
	std::ifstream ifs(fileName);
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	std::string syscallName;
	StringSetList syscallList;
	StringSet syscallNameGroup;

	while(std::getline(buffer, syscallName, '\n')) {
		// Remove potential spaces in the the given string.
		std::string::iterator end_pos = std::remove(syscallName.begin(), syscallName.end(), ' ');
		syscallName.erase(end_pos, syscallName.end());
		syscallNameGroup.insert(syscallName);	

		if(!group) {
			syscallList.push_back(syscallNameGroup);
			syscallNameGroup.clear();
		}
	}

	if(group)
		syscallList.push_back(syscallNameGroup);

	return syscallList;
}

const SyscallAPI::SysTypeSetList& SyscallAPI::getAllSyscallGroups() const {
	return syscallGroupList;
}

const SyscallAPI::StringSetList& SyscallAPI::getAllSyscallNameGroups() const {
	return syscallNameGroupList;
}

const SyscallAPI::StringSetList& SyscallAPI::getAllSyscallNames() const {
	return syscallNames;
}

std::string SyscallAPI::getSyscallName(SyscallAPI::SYS_TYPE type) const {
	return sysTypeToFuncNameMap.at(type);
}

SyscallAPI::SYS_TYPE SyscallAPI::getSyscallType(std::string syscallName) const {
	return funcNameToSysTypeMap.at(syscallName);
}
