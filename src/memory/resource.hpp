#pragma once

#include "../types.hpp"

namespace micron
{

struct timeval_t {
  time64_t tv_sec;
  suseconds64_t tv_usec;
};

struct rusage_t {
  /* Total amount of user time used.  */
  timeval_t ru_utime;
  /* Total amount of system time used.  */
  timeval_t ru_stime;
  /* Maximum resident set size (in kilobytes).  */
  __extension__ union {
    long int ru_maxrss;
    slong_t __ru_maxrss_word;
  };
  /* Amount of sharing of text segment memory
     with other processes (kilobyte-seconds).  */
  __extension__ union {
    long int ru_ixrss;
    slong_t __ru_ixrss_word;
  };
  /* Amount of data segment memory used (kilobyte-seconds).  */
  __extension__ union {
    long int ru_idrss;
    slong_t __ru_idrss_word;
  };
  /* Amount of stack memory used (kilobyte-seconds).  */
  __extension__ union {
    long int ru_isrss;
    slong_t __ru_isrss_word;
  };
  /* Number of soft page faults (i.e. those serviced by reclaiming
     a page from the list of pages awaiting reallocation.  */
  __extension__ union {
    long int ru_minflt;
    slong_t __ru_minflt_word;
  };
  /* Number of hard page faults (i.e. those that required I/O).  */
  __extension__ union {
    long int ru_majflt;
    slong_t __ru_majflt_word;
  };
  /* Number of times a process was swapped out of physical memory.  */
  __extension__ union {
    long int ru_nswap;
    slong_t __ru_nswap_word;
  };
  /* Number of input operations via the file system.  Note: This
     and `ru_oublock' do not include operations with the cache.  */
  __extension__ union {
    long int ru_inblock;
    slong_t __ru_inblock_word;
  };
  /* Number of output operations via the file system.  */
  __extension__ union {
    long int ru_oublock;
    slong_t __ru_oublock_word;
  };
  /* Number of IPC messages sent.  */
  __extension__ union {
    long int ru_msgsnd;
    slong_t __ru_msgsnd_word;
  };
  /* Number of IPC messages received.  */
  __extension__ union {
    long int ru_msgrcv;
    slong_t __ru_msgrcv_word;
  };
  /* Number of signals delivered.  */
  __extension__ union {
    long int ru_nsignals;
    slong_t __ru_nsignals_word;
  };
  /* Number of voluntary context switches, i.e. because the process
     gave up the process before it had to (usually to wait for some
     resource to be available).  */
  __extension__ union {
    long int ru_nvcsw;
    slong_t __ru_nvcsw_word;
  };
  /* Number of involuntary context switches, i.e. a higher priority process
     became runnable or the current process used up its time slice.  */
  __extension__ union {
    long int ru_nivcsw;
    slong_t __ru_nivcsw_word;
  };
};

};
