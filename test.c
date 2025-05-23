#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <string.h>
#include <time.h>
#include <linux/types.h>
#include "/usr/src/linux-headers-6.5.13-softwarn2/include/uapi/linux/sched.h"
#include "monitoring.h"
// #include "/usr/src/linux-headers-6.5.13-rtwarn/include/linux/sched.h"

#define gettid() syscall(SYS_gettid)
#define RUNTIME_NS  6 * 1000 * 1000   // 3ms
#define DEADLINE_NS 1000 * 1000 * 1000  // 10ms
#define PERIOD_NS   1000 * 1000 * 1000  // 10ms
void handle_sigusr1(int sig) {
    printf("🟡 [SIGUSR1] softwarn 시그널 수신! tid=%ld\n", gettid());
}

void handler(int sig) {
    printf("[SIGXCPU] overrun detected\n");
}

struct sched_attr {
   __u32 size;
   __u32 sched_policy;
   __u64 sched_flags;
   __s32 sched_nice;
   __u32 sched_priority;
   __u64 sched_runtime;
   __u64 sched_deadline;
   __u64 sched_period;
   __u32 sched_util_min;
   __u32 sched_util_max;
};

int main() {
    struct sched_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_runtime = RUNTIME_NS;
    attr.sched_deadline = DEADLINE_NS;
    attr.sched_period = PERIOD_NS;

    monitor();

    //signal(SIGUSR1, handle_sigusr1);
    //signal(SIGXCPU, handler);
    // sigset_t set;
    // sigemptyset(&set);
    // sigaddset(&set, SIGXCPU);
    // sigprocmask(SIG_BLOCK, &set, NULL);
    printf("sizeof(attr) = %zu\n", sizeof(attr));
    printf("🟢 SCHED_DEADLINE 테스트 시작 (tid=%ld)\n", gettid());
    if (syscall(SYS_sched_setattr, 0, &attr, 0) < 0) {
        perror("sched_setattr");
        return 1;
    }

    // 일부러 runtime 근접 시까지 연산
    while (1) {
        volatile unsigned long long i = 0;
        for (i = 0; i < 1e8; ++i);  // 바쁜 루프

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("현재 시간: %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        usleep(1000000); // 0.5초 대기
    }

    return 0;
}
