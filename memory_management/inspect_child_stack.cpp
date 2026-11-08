// inspect_child_stack.cpp
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int measurement = 100;

    const pid_t fork_result = fork();

    if (fork_result == -1)
    {
        std::perror("fork");
        return EXIT_FAILURE;
    }

    if (fork_result == 0)
    {
        measurement = 250;

        std::cout << "Child PID: " << getpid() << "\nmeasurement = " << measurement
                  << "\nVirtual address: " << &measurement << '\n'
                  << std::flush;

        sleep(180);

        return EXIT_SUCCESS;
    }

    std::cout << "Parent PID: " << getpid() << "\nmeasurement = " << measurement
              << "\nVirtual address: " << &measurement << '\n'
              << std::flush;
    int status = 0;
    while (waitpid(fork_result, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            std::perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}