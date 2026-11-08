
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sched.h>

/**
 * Returns a human-readable name for a Linux scheduler policy.
 *
 * @param policy The scheduler policy identifier to convert.
 * @return A descriptive string for the provided policy.
 */
const char* policy_name(int policy)
{
    switch (policy)
    {
    case SCHED_OTHER:
        return "SCHED_OTHER"; // Default time-sharing policy
    case SCHED_BATCH:
        return "SCHED_BATCH"; // For batch processing
    case SCHED_IDLE:
        return "SCHED_IDLE"; // For very low priority background jobs
    case SCHED_FIFO:
        return "SCHED_FIFO"; // First-in, first-out real-time policy
    case SCHED_RR:
        return "SCHED_RR"; // Round-robin real-time policy. Different from FIFO in that it allows
                           // time slicing between threads of equal priority.
    default:
        return "Other or unknown policy";
    }
}

/**
 * Displays the current scheduling policy, static priority, and CPU affinity
 * information for the calling thread.
 *
 * @return 0 on success, or 1 if a scheduling query fails.
 */
int main()
{
    // Get the current scheduling policy for the calling thread.
    const int policy = sched_getscheduler(0);

    if (policy == -1)
    {
        std::cerr << "sched_getscheduler failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    sched_param parameters{};

    // Get the static priority for the calling thread.
    if (sched_getparam(0, &parameters) == -1)
    {
        std::cerr << "sched_getparam failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // policy_name() is a helper function that converts the numeric policy identifier to a
    // human-readable string.
    std::cout << "Scheduling policy: " << policy_name(policy) << '\n';

    std::cout << "Static priority: " << parameters.sched_priority << '\n';

    // Get the CPUs on which this thread is allowed to execute.
    cpu_set_t affinity;
    CPU_ZERO(&affinity);

    // sched_getaffinity() retrieves the CPU affinity mask for the calling thread.
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
