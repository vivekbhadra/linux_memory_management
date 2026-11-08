
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sched.h>

const char* policy_name(int policy)
{
    switch (policy)
    {
    case SCHED_OTHER:
        return "SCHED_OTHER";
    case SCHED_BATCH:
        return "SCHED_BATCH";
    case SCHED_IDLE:
        return "SCHED_IDLE";
    case SCHED_FIFO:
        return "SCHED_FIFO";
    case SCHED_RR:
        return "SCHED_RR";
    default:
        return "Other or unknown policy";
    }
}

int main()
{
    const int policy = sched_getscheduler(0);

    if (policy == -1)
    {
        std::cerr << "sched_getscheduler failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    sched_param parameters{};

    if (sched_getparam(0, &parameters) == -1)
    {
        std::cerr << "sched_getparam failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "Scheduling policy: " << policy_name(policy) << '\n';

    std::cout << "Static priority: " << parameters.sched_priority << '\n';

    // Get the CPUs on which this thread is allowed to execute.
    cpu_set_t affinity;
    CPU_ZERO(&affinity);

    if (sched_getaffinity(0, sizeof(affinity), &affinity) == -1)
    {
        std::cerr << "sched_getaffinity failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // Get the CPU currently executing this thread.
    const int cpu = sched_getcpu();

    if (cpu == -1)
    {
        std::cerr << "sched_getcpu failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "Currently executing on CPU: " << cpu << '\n';

    std::cout << "Allowed CPUs: ";

    for (int i = 0; i < CPU_SETSIZE; ++i)
    {
        if (CPU_ISSET(i, &affinity))
        {
            std::cout << i << ' ';
        }
    }

    std::cout << '\n';

    return 0;
}
