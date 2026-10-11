#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    auto measurement = std::make_unique<int>(100);

    const pid_t fork_result = fork();

    if (fork_result == -1)
    {
        std::perror("fork");
        return EXIT_FAILURE;
    }

    if (fork_result == 0)
    {
        *measurement = 250;

        std::cout << "Child PID: " << getpid() << "\nmeasurement = " << *measurement
                  << "\nInteger's virtual address: " << measurement.get() << '\n'
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

    std::cout << "Parent: measurement = " << *measurement << '\n';

    return EXIT_SUCCESS;
}