// child_variable_copy.cpp
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
        std::cout << "Child: initial measurement = " << measurement << '\n';

        measurement = 250;

        std::cout << "Child: changed measurement = " << measurement << '\n';

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

    std::cout << "Parent: measurement after child finishes = " << measurement << '\n';

    return EXIT_SUCCESS;
}