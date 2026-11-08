// child_physical_address.cpp
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void print_physical_address(const void* address)
{
    const long result = sysconf(_SC_PAGESIZE);
    if (result <= 0)
    {
        std::cerr << "Cannot determine page size\n";
        return;
    }

    const auto page_size = static_cast<std::uint64_t>(result);
    const auto virtual_address = reinterpret_cast<std::uintptr_t>(address);

    const int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd == -1)
    {
        std::perror("open pagemap");
        return;
    }

    const auto entry_offset =
        static_cast<off_t>((virtual_address / page_size) * sizeof(std::uint64_t));

    std::uint64_t entry = 0;
    ssize_t bytes_read;

    do
    {
        bytes_read = pread(fd, &entry, sizeof(entry), entry_offset);
    } while (bytes_read == -1 && errno == EINTR);

    if (bytes_read == -1)
    {
        std::perror("pread pagemap");
        close(fd);
        return;
    }

    close(fd);

    if (bytes_read != static_cast<ssize_t>(sizeof(entry)))
    {
        std::cerr << "Incomplete pagemap entry\n";
        return;
    }

    // Bit 63 indicates whether the page is resident in RAM.
    if ((entry & (std::uint64_t{1} << 63)) == 0)
    {
        std::cout << "Page is not resident in RAM\n";
        return;
    }

    // Bits 0–54 contain the physical page frame number.
    const auto frame_number = entry & ((std::uint64_t{1} << 55) - 1);

    if (frame_number == 0)
    {
        std::cout << "PFN is zero; physical address unavailable"
                  << " (normally hidden without permission)\n";
        return;
    }

    const auto physical_address = frame_number * page_size + virtual_address % page_size;

    std::cout << "Physical address: 0x" << std::hex << physical_address << std::dec << '\n';
}

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
        // measurement = 250;

        std::cout << "Child: measurement = " << measurement
                  << ", virtual address = " << &measurement << '\n';

        print_physical_address(&measurement);
        std::cout.flush();

        // Keep the child's page allocated during inspection.
        sleep(10);

        return EXIT_SUCCESS;
    }

    std::cout << "Parent: measurement = " << measurement << ", virtual address = " << &measurement
              << '\n';

    print_physical_address(&measurement);
    std::cout.flush();

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