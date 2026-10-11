#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

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
        std::cout << "Child: my PID is " << getpid() << ", my parent's PID is " << getppid()
                  << '\n';
        return EXIT_SUCCESS;
    }

    std::cout << "Parent: my PID is " << getpid() << ", my child's PID is " << fork_result << '\n';

    // Wait for the child to finish and collect its exit status.
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