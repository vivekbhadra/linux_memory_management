#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sched.h>
#include <sys/resource.h>
#include <system_error>
#include <thread>
#include <unistd.h>

/**
 * Returns a human-readable string for a Linux scheduling policy.
 *
 * @param policy The scheduler policy identifier to describe.
 * @return A descriptive name for the policy.
 */
const char* policy_name(int policy)
{
    switch (policy)
    {
    case SCHED_OTHER:
        return "SCHED_OTHER (TS)";
    case SCHED_FIFO:
        return "SCHED_FIFO (FF)";
    case SCHED_RR:
        return "SCHED_RR (RR)";
    case SCHED_BATCH:
        return "SCHED_BATCH";
    case SCHED_IDLE:
        return "SCHED_IDLE";
    default:
        return "Other scheduling policy";
    }
}

/**
 * Prints a formatted error message for a failed scheduler or process call.
 *
 * @param operation The system call or operation that failed.
 */
void report_error(const char* operation)
{
    const int error = errno;
    std::cerr << operation << ": " << std::error_code(error, std::generic_category()).message()
              << '\n';
}

/**
 * Displays the calling process's scheduler policy, priority information,
 * and round-robin quantum when applicable.
 *
 * @return EXIT_SUCCESS on success, or EXIT_FAILURE if a scheduler query fails.
 */
int main()
{
    const int policy = sched_getscheduler(0);
    if (policy == -1)
    {
        report_error("sched_getscheduler");
        return EXIT_FAILURE;
    }

    sched_param parameters{};
    if (sched_getparam(0, &parameters) == -1)
    {
        report_error("sched_getparam");
        return EXIT_FAILURE;
    }

    errno = 0;
    const int nice_value = getpriority(PRIO_PROCESS, 0);
    if (nice_value == -1 && errno != 0)
    {
        report_error("getpriority");
        return EXIT_FAILURE;
    }

    const bool is_fifo_or_rr = policy == SCHED_FIFO || policy == SCHED_RR;
    std::cout << "PID: " << getpid() << '\n' << "Policy: " << policy_name(policy) << '\n';

    if (is_fifo_or_rr)
    {
        std::cout << "Real-time priority: " << parameters.sched_priority << '\n'
                  << "Nice value: not used by this policy\n";
    }
    else
    {
        std::cout << "Real-time priority: not applicable\n"
                  << "Nice value: " << nice_value << '\n';
    }

    if (policy == SCHED_RR)
    {
        timespec quantum{};
        if (sched_rr_get_interval(0, &quantum) == -1)
        {
            report_error("sched_rr_get_interval");
            return EXIT_FAILURE;
        }
        const auto interval =
            std::chrono::seconds{quantum.tv_sec} + std::chrono::nanoseconds{quantum.tv_nsec};
        std::cout << "RR quantum: " << std::chrono::duration<double, std::milli>{interval}.count()
                  << " ms\n";
    }

    std::cout << "Sleeping for 15 seconds so you can inspect this PID with ps.\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds{15});
}
