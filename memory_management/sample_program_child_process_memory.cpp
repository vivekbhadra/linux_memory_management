// sample_program_child_process_memory.cpp
//
// Creates a child process with fork() and pauses the child at four
// checkpoints so its memory can be inspected with standard Linux tools.
//
// Build: g++ -std=c++20 -Wall -Wextra -O0 -g sample_program_child_process_memory.cpp -o
// child_memory Run:   ./child_memory          (physical frame numbers are hidden)
//        sudo ./child_memory     (physical frame numbers are shown)

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

constexpr std::size_t KiB = 1024;
constexpr std::size_t MiB = 1024 * KiB;

constexpr std::size_t sensor_buffer_size = 64 * MiB; // touched by the parent
constexpr std::size_t lazy_region_size = 32 * MiB;   // mapped by the child
constexpr std::size_t lazy_touch_size = 8 * MiB;     // part of it the child touches

int measurement_limit = 100; // initialized global: .data
int error_counts[1024];      // zero-initialized global: .bss

long page_size = 4096;

// Reads the physical page frame number (PFN) behind a virtual address from
// /proc/self/pagemap. The kernel reports the PFN as 0 to processes without
// CAP_SYS_ADMIN, so run the program with sudo to see real frame numbers.
std::uint64_t page_frame_number(const void* address)
{
    std::ifstream pagemap("/proc/self/pagemap", std::ios::binary);
    const auto virtual_page = reinterpret_cast<std::uintptr_t>(address) / page_size;
    pagemap.seekg(static_cast<std::streamoff>(virtual_page * sizeof(std::uint64_t)));

    std::uint64_t entry = 0;
    pagemap.read(reinterpret_cast<char*>(&entry), sizeof entry);
    if (!pagemap)
    {
        return 0;
    }

    const bool present = (entry >> 63) & 1U;             // bit 63: page is in RAM
    constexpr std::uint64_t pfn_mask = (1ULL << 55) - 1; // bits 0-54: PFN
    return present ? (entry & pfn_mask) : 0;
}

void print_frame(const char* who, const char* what, const void* address)
{
    const std::uint64_t pfn = page_frame_number(address);
    std::cout << who << ' ' << what << " at " << address << " -> physical frame ";
    if (pfn == 0)
    {
        std::cout << "(hidden: run with sudo)\n";
    }
    else
    {
        std::cout << pfn << '\n';
    }
}

// Stops the child with SIGSTOP. The process stays in state T until the
// shell sends SIGCONT, which gives time to run the inspection tools.
void pause_for_inspection(int checkpoint, const char* description)
{
    std::cout << "\n[child " << getpid() << "] Checkpoint " << checkpoint << ": " << description
              << "\n    Inspect now, then resume with:  kill -CONT " << getpid() << '\n'
              << std::flush;
    std::raise(SIGSTOP);
}

void run_child(std::byte* sensor_buffer, int* shared_counter)
{
    print_frame("[child] ", "measurement_limit", &measurement_limit);
    pause_for_inspection(1, "forked, nothing written yet (pages shared with the parent)");

    // Writing to inherited pages triggers copy-on-write.
    measurement_limit = 250;
    std::memset(sensor_buffer, 0x55, sensor_buffer_size);
    *shared_counter = 7; // MAP_SHARED: the parent will see this value
    print_frame("[child] ", "measurement_limit", &measurement_limit);
    pause_for_inspection(2, "wrote to measurement_limit and the 64 MiB buffer (copy-on-write)");

    // A new anonymous mapping: the VMA exists, but no physical pages yet.
    void* region =
        mmap(nullptr, lazy_region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED)
    {
        std::perror("mmap");
        _exit(EXIT_FAILURE);
    }
    auto* lazy = static_cast<std::byte*>(region);
    std::cout << "[child]  32 MiB anonymous mapping at " << region << '\n';
    pause_for_inspection(3, "mapped 32 MiB but touched none of it");

    // Touch one byte per page in the first 8 MiB: each first touch is a page fault.
    for (std::size_t offset = 0; offset < lazy_touch_size;
         offset += static_cast<std::size_t>(page_size))
    {
        lazy[offset] = std::byte{0x2A};
    }
    pause_for_inspection(4, "touched the first 8 MiB of the 32 MiB mapping");

    munmap(region, lazy_region_size);
    std::cout << "[child]  exiting; until the parent reaps it, the child is a zombie (state Z)\n"
              << std::flush;
    _exit(EXIT_SUCCESS); // skip the parent's inherited stdio buffers and destructors
}

} // namespace

int main()
{
    page_size = sysconf(_SC_PAGESIZE);

    int local_reading = 0;                        // stack
    auto small_value = std::make_unique<int>(42); // small heap block (brk heap)

    // Large heap block: glibc serves it with its own anonymous mmap.
    // make_unique_for_overwrite (C++20) skips zero-filling, so the memset
    // below is the first write and decides when the pages become resident.
    auto sensor_buffer = std::make_unique_for_overwrite<std::byte[]>(sensor_buffer_size);
    std::memset(sensor_buffer.get(), 0x11, sensor_buffer_size);

    // One shared anonymous page, visible to both parent and child after fork().
    void* shared_page = mmap(nullptr,
                             static_cast<std::size_t>(page_size),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS,
                             -1,
                             0);
    if (shared_page == MAP_FAILED)
    {
        std::perror("mmap");
        return EXIT_FAILURE;
    }
    auto* shared_counter = static_cast<int*>(shared_page);
    *shared_counter = 0;
    error_counts[0] = 0;

    std::cout << "[parent " << getpid() << "] page size " << page_size << " bytes\n"
              << "  .data   &measurement_limit = " << &measurement_limit << '\n'
              << "  .bss    error_counts       = " << error_counts << '\n'
              << "  heap    small_value        = " << small_value.get() << '\n'
              << "  mmap    sensor_buffer      = " << static_cast<void*>(sensor_buffer.get())
              << '\n'
              << "  shared  shared_counter     = " << shared_page << '\n'
              << "  stack   &local_reading     = " << &local_reading << '\n';
    print_frame("[parent]", "measurement_limit", &measurement_limit);

    std::cout.flush(); // flush before fork so the child does not inherit buffered output
    const pid_t child = fork();
    if (child == -1)
    {
        std::perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0)
    {
        run_child(sensor_buffer.get(), shared_counter);
    }

    std::cout << "[parent] created child " << child
              << ". Press Enter after the child has exited to reap it.\n"
              << std::flush;
    std::cin.get();

    int status = 0;
    waitpid(child, &status, 0);
    std::cout << "[parent] child reaped, exit status " << WEXITSTATUS(status) << '\n'
              << "[parent] measurement_limit = " << measurement_limit
              << " (still 100: the child wrote to its own copy)\n"
              << "[parent] shared_counter    = " << *shared_counter
              << " (7: MAP_SHARED pages are the same physical pages)\n";
    print_frame("[parent]", "measurement_limit", &measurement_limit);

    munmap(shared_page, static_cast<std::size_t>(page_size));
    return EXIT_SUCCESS;
}