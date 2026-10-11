#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int measurement_limit = 100;

int main()
{
    const pid_t fork_result = fork();

    if (fork_result == -1)
    {
        std::perror("fork");
        return EXIT_FAILURE;
    }

    if (fork_result == 0)
    {
        measurement_limit = 250;

        std::cout << "Child PID: " << getpid() << "\nmeasurement_limit = " << measurement_limit
                  << "\nVirtual address: " << &measurement_limit << '\n'
                  << std::flush;

        sleep(60);

        return EXIT_SUCCESS;
    }

    int status = 0;
    while (waitpid(fork_result, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            std::perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    std::cout << "Parent: measurement_limit = " << measurement_limit << '\n';

    return EXIT_SUCCESS;
}