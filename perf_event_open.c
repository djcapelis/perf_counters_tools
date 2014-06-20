/***********************************************************************
 * perf_event_open.c                                                   *
 *                                                                     *
 * This is a wrapper for the perf_event_open syscall, because glibc    *
 * doesn't have one, for some reason.                                  * 
 *                                                                     *
 **********************************************************************/

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include<linux/perf_event.h>
#include<asm/unistd.h>

long perf_event_open(struct perf_event_attr * eventinfo, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, eventinfo, pid, cpu, group_fd, flags);
}
