// hello_world.cpp
#include <iostream>
#include <thread>
#include <unistd.h>

int main()
{
    std::cout << "Hello World\n";
    std::cout << "My PID: " << getpid() << "\n";

    std::thread my_thread(
        []()
        {
            std::cout << "Hello from thread\n";
            std::cout << "Thread PID: " << getpid() << "\n";
            sleep(60); // Keep the thread alive for inspection
        });

    my_thread.join();

    return 0;
}