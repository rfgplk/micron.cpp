#pragma once;

#include "../../types.hpp"

namespace micron
{

template <typename... Args>
int
prctl(int option, Args &&...args)
{
  return static_cast<int>(micron::syscall(SYS_prctl, option, static_cast<unsigned long>(args)...));
}

int
prctl(int option, unsigned long arg0 = 0, unsigned long arg1 = 0, unsigned long arg2 = 0, unsigned long arg3 = 0, unsigned long arg4 = 0)
{
  return static_cast<int>(micron::syscall(SYS_prctl, option, arg0, arg1, arg2, arg3, arg4));
}

struct prctl_mm_map {
  u64 start_code;
  u64 end_code;
  u64 start_data;
  u64 end_data;
  u64 start_brk;
  u64 brk;
  u64 start_stack;
  u64 arg_start;
  u64 arg_end;
  u64 env_start;
  u64 env_end;
  u64 *auxv;
  u32 auxv_size;
  u32 exe_fd;
};

constexpr int PR_SET_PDEATHSIG = 1;
constexpr int PR_GET_PDEATHSIG = 2;

constexpr int PR_GET_DUMPABLE = 3;
constexpr int PR_SET_DUMPABLE = 4;

constexpr int PR_GET_UNALIGN = 5;
constexpr int PR_SET_UNALIGN = 6;
constexpr int PR_UNALIGN_NOPRINT = 1;
constexpr int PR_UNALIGN_SIGBUS = 2;

constexpr int PR_GET_KEEPCAPS = 7;
constexpr int PR_SET_KEEPCAPS = 8;

constexpr int PR_GET_FPEMU = 9;
constexpr int PR_SET_FPEMU = 10;
constexpr int PR_FPEMU_NOPRINT = 1;
constexpr int PR_FPEMU_SIGFPE = 2;

constexpr int PR_GET_FPEXC = 11;
constexpr int PR_SET_FPEXC = 12;
constexpr int PR_FP_EXC_SW_ENABLE = 0x80;
constexpr int PR_FP_EXC_DIV = 0x010000;
constexpr int PR_FP_EXC_OVF = 0x020000;
constexpr int PR_FP_EXC_UND = 0x040000;
constexpr int PR_FP_EXC_RES = 0x080000;
constexpr int PR_FP_EXC_INV = 0x100000;
constexpr int PR_FP_EXC_DISABLED = 0;
constexpr int PR_FP_EXC_NONRECOV = 1;
constexpr int PR_FP_EXC_ASYNC = 2;
constexpr int PR_FP_EXC_PRECISE = 3;

constexpr int PR_GET_TIMING = 13;
constexpr int PR_SET_TIMING = 14;
constexpr int PR_TIMING_STATISTICAL = 0;
constexpr int PR_TIMING_TIMESTAMP = 1;

constexpr int PR_SET_NAME = 15;
constexpr int PR_GET_NAME = 16;

constexpr int PR_GET_ENDIAN = 19;
constexpr int PR_SET_ENDIAN = 20;
constexpr int PR_ENDIAN_BIG = 0;
constexpr int PR_ENDIAN_LITTLE = 1;
constexpr int PR_ENDIAN_PPC_LITTLE = 2;

constexpr int PR_GET_SECCOMP = 21;
constexpr int PR_SET_SECCOMP = 22;

constexpr int PR_CAPBSET_READ = 23;
constexpr int PR_CAPBSET_DROP = 24;

constexpr int PR_GET_TSC = 25;
constexpr int PR_SET_TSC = 26;
constexpr int PR_TSC_ENABLE = 1;
constexpr int PR_TSC_SIGSEGV = 2;

constexpr int PR_GET_SECUREBITS = 27;
constexpr int PR_SET_SECUREBITS = 28;

constexpr int PR_SET_TIMERSLACK = 29;
constexpr int PR_GET_TIMERSLACK = 30;

constexpr int PR_TASK_PERF_EVENTS_DISABLE = 31;
constexpr int PR_TASK_PERF_EVENTS_ENABLE = 32;

constexpr int PR_MCE_KILL = 33;
constexpr int PR_MCE_KILL_CLEAR = 0;
constexpr int PR_MCE_KILL_SET = 1;

constexpr int PR_MCE_KILL_LATE = 0;
constexpr int PR_MCE_KILL_EARLY = 1;
constexpr int PR_MCE_KILL_DEFAULT = 2;

constexpr int PR_MCE_KILL_GET = 34;

constexpr int PR_SET_MM = 35;
constexpr int PR_SET_MM_START_CODE = 1;
constexpr int PR_SET_MM_END_CODE = 2;
constexpr int PR_SET_MM_START_DATA = 3;
constexpr int PR_SET_MM_END_DATA = 4;
constexpr int PR_SET_MM_START_STACK = 5;
constexpr int PR_SET_MM_START_BRK = 6;
constexpr int PR_SET_MM_BRK = 7;
constexpr int PR_SET_MM_ARG_START = 8;
constexpr int PR_SET_MM_ARG_END = 9;
constexpr int PR_SET_MM_ENV_START = 10;
constexpr int PR_SET_MM_ENV_END = 11;
constexpr int PR_SET_MM_AUXV = 12;
constexpr int PR_SET_MM_EXE_FILE = 13;
constexpr int PR_SET_MM_MAP = 14;
constexpr int PR_SET_MM_MAP_SIZE = 15;

constexpr int PR_SET_PTRACER = 0x59616d61;
constexpr int PR_SET_PTRACER_ANY = ((unsigned long)-1);

constexpr int PR_SET_CHILD_SUBREAPER = 36;
constexpr int PR_GET_CHILD_SUBREAPER = 37;

constexpr int PR_SET_NO_NEW_PRIVS = 38;
constexpr int PR_GET_NO_NEW_PRIVS = 39;

constexpr int PR_GET_TID_ADDRESS = 40;

constexpr int PR_SET_THP_DISABLE = 41;
constexpr int PR_GET_THP_DISABLE = 42;

constexpr int PR_MPX_ENABLE_MANAGEMENT = 43;
constexpr int PR_MPX_DISABLE_MANAGEMENT = 44;

constexpr int PR_SET_FP_MODE = 45;
constexpr int PR_GET_FP_MODE = 46;
constexpr int PR_FP_MODE_FR = (1 << 0);
constexpr int PR_FP_MODE_FRE = (1 << 1);

constexpr int PR_CAP_AMBIENT = 47;
constexpr int PR_CAP_AMBIENT_IS_SET = 1;
constexpr int PR_CAP_AMBIENT_RAISE = 2;
constexpr int PR_CAP_AMBIENT_LOWER = 3;
constexpr int PR_CAP_AMBIENT_CLEAR_ALL = 4;

constexpr int PR_SVE_SET_VL = 50;
constexpr int PR_SVE_SET_VL_ONEXEC = (1 << 18);
constexpr int PR_SVE_GET_VL = 51;
constexpr int PR_SVE_VL_LEN_MASK = 0xffff;
constexpr int PR_SVE_VL_INHERIT = (1 << 17);

constexpr int PR_GET_SPECULATION_CTRL = 52;
constexpr int PR_SET_SPECULATION_CTRL = 53;
constexpr int PR_SPEC_STORE_BYPASS = 0;
constexpr int PR_SPEC_INDIRECT_BRANCH = 1;
constexpr int PR_SPEC_L1D_FLUSH = 2;
constexpr int PR_SPEC_NOT_AFFECTED = 0;
constexpr int PR_SPEC_PRCTL = (1UL << 0);
constexpr int PR_SPEC_ENABLE = (1UL << 1);
constexpr int PR_SPEC_DISABLE = (1UL << 2);
constexpr int PR_SPEC_FORCE_DISABLE = (1UL << 3);
constexpr int PR_SPEC_DISABLE_NOEXEC = (1UL << 4);

constexpr int PR_PAC_RESET_KEYS = 54;
constexpr int PR_PAC_APIAKEY = (1UL << 0);
constexpr int PR_PAC_APIBKEY = (1UL << 1);
constexpr int PR_PAC_APDAKEY = (1UL << 2);
constexpr int PR_PAC_APDBKEY = (1UL << 3);
constexpr int PR_PAC_APGAKEY = (1UL << 4);

constexpr int PR_SET_TAGGED_ADDR_CTRL = 55;
constexpr int PR_GET_TAGGED_ADDR_CTRL = 56;
constexpr int PR_TAGGED_ADDR_ENABLE = (1UL << 0);
constexpr int PR_MTE_TCF_NONE = 0UL;
constexpr int PR_MTE_TCF_SYNC = (1UL << 1);
constexpr int PR_MTE_TCF_ASYNC = (1UL << 2) c;
constexpr int PR_MTE_TCF_MASK = (PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC);
constexpr int PR_MTE_TAG_SHIFT = 3;
constexpr int PR_MTE_TAG_MASK = (0xffffUL << PR_MTE_TAG_SHIFT);
constexpr int PR_MTE_TCF_SHIFT = 1;
constexpr int PR_PMLEN_SHIFT = 24;
constexpr int PR_PMLEN_MASK = (0x7fUL << PR_PMLEN_SHIFT);

constexpr int PR_SET_IO_FLUSHER = 57;
constexpr int PR_GET_IO_FLUSHER = 58;

constexpr int PR_SET_SYSCALL_USER_DISPATCH = 59;
constexpr int PR_SYS_DISPATCH_OFF = 0;
constexpr int PR_SYS_DISPATCH_ON = 1;
constexpr int SYSCALL_DISPATCH_FILTER_ALLOW = 0;
constexpr int SYSCALL_DISPATCH_FILTER_BLOCK = 1;

constexpr int PR_PAC_SET_ENABLED_KEYS = 60;
constexpr int PR_PAC_GET_ENABLED_KEYS = 61;

constexpr int PR_SCHED_CORE = 62;
constexpr int PR_SCHED_CORE_GET = 0;
constexpr int PR_SCHED_CORE_CREATE = 1;
constexpr int PR_SCHED_CORE_SHARE_TO = 2;
constexpr int PR_SCHED_CORE_SHARE_FROM = 3;
constexpr int PR_SCHED_CORE_MAX = 4;
constexpr int PR_SCHED_CORE_SCOPE_THREAD = 0;
constexpr int PR_SCHED_CORE_SCOPE_THREAD_GROUP = 1;
constexpr int PR_SCHED_CORE_SCOPE_PROCESS_GROUP = 2;

constexpr int PR_SME_SET_VL = 63;
constexpr int PR_SME_SET_VL_ONEXEC = (1 << 18);
constexpr int PR_SME_GET_VL = 64;
constexpr int PR_SME_VL_LEN_MASK = 0xffff;
constexpr int PR_SME_VL_INHERIT = (1 << 17);

constexpr int PR_SET_MDWE = 65;
constexpr int PR_MDWE_REFUSE_EXEC_GAIN = (1UL << 0);
constexpr int PR_MDWE_NO_INHERIT = (1UL << 1);

constexpr int PR_GET_MDWE = 66;

constexpr int PR_SET_VMA = 0x53564d41;
constexpr int PR_SET_VMA_ANON_NAME = 0;

constexpr int PR_GET_AUXV = 0x41555856;

constexpr int PR_SET_MEMORY_MERGE = 67;
constexpr int PR_GET_MEMORY_MERGE = 68;

constexpr int PR_RISCV_V_SET_CONTROL = 69;
constexpr int PR_RISCV_V_GET_CONTROL = 70;
constexpr int PR_RISCV_V_VSTATE_CTRL_DEFAULT = 0;
constexpr int PR_RISCV_V_VSTATE_CTRL_OFF = 1;
constexpr int PR_RISCV_V_VSTATE_CTRL_ON = 2;
constexpr int PR_RISCV_V_VSTATE_CTRL_INHERIT = (1 << 4);
constexpr int PR_RISCV_V_VSTATE_CTRL_CUR_MASK = 0x3;
constexpr int PR_RISCV_V_VSTATE_CTRL_NEXT_MASK = 0xc;
constexpr int PR_RISCV_V_VSTATE_CTRL_MASK = 0x1f;

constexpr int PR_RISCV_SET_ICACHE_FLUSH_CTX = 71;
constexpr int PR_RISCV_CTX_SW_FENCEI_ON = 0;
constexpr int PR_RISCV_CTX_SW_FENCEI_OFF = 1;
constexpr int PR_RISCV_SCOPE_PER_PROCESS = 0;
constexpr int PR_RISCV_SCOPE_PER_THREAD = 1;

constexpr int PR_PPC_GET_DEXCR = 72;
constexpr int PR_PPC_SET_DEXCR = 73;
constexpr int PR_PPC_DEXCR_SBHE = 0;
constexpr int PR_PPC_DEXCR_IBRTPD = 1;
constexpr int PR_PPC_DEXCR_SRAPD = 2;
constexpr int PR_PPC_DEXCR_NPHIE = 3;
constexpr int PR_PPC_DEXCR_CTRL_EDITABLE = 0x1;
constexpr int PR_PPC_DEXCR_CTRL_SET = 0x2;
constexpr int PR_PPC_DEXCR_CTRL_CLEAR = 0x4;
constexpr int PR_PPC_DEXCR_CTRL_SET_ONEXEC = 0x8;
constexpr int PR_PPC_DEXCR_CTRL_CLEAR_ONEXEC = 0x10;
constexpr int PR_PPC_DEXCR_CTRL_MASK = 0x1f;

constexpr int PR_GET_SHADOW_STACK_STATUS = 74;

constexpr int PR_SET_SHADOW_STACK_STATUS = 75;
constexpr int PR_SHADOW_STACK_ENABLE = (1UL << 0);
constexpr int PR_SHADOW_STACK_WRITE = (1UL << 1);
constexpr int PR_SHADOW_STACK_PUSH = (1UL << 2);

constexpr int PR_LOCK_SHADOW_STACK_STATUS = 76;

constexpr int PR_TIMER_CREATE_RESTORE_IDS = 77;
constexpr int PR_TIMER_CREATE_RESTORE_IDS_OFF = 0;
constexpr int PR_TIMER_CREATE_RESTORE_IDS_ON = 1;
constexpr int PR_TIMER_CREATE_RESTORE_IDS_GET = 2;

constexpr int PR_FUTEX_HASH = 78;
constexpr int PR_FUTEX_HASH_SET_SLOTS = 1;
constexpr int FH_FLAG_IMMUTABLE = (1ULL << 0);
constexpr int PR_FUTEX_HASH_GET_SLOTS = 2;
constexpr int PR_FUTEX_HASH_GET_IMMUTABLE = 3;

};     // namespace micron
