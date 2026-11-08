
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
}
