// thread_shared_memory.cpp
#include <functional>
#include <iostream>
#include <thread>
#include <unistd.h>

void update_measurement(int& measurement)
{
    std::cout << "Worker thread: process PID = " << getpid()
              << "\nWorker thread: initial measurement = " << measurement
              << "\nWorker thread: address = " << &measurement << '\n';

    measurement = 250;
}

int main()
{
    int measurement = 100;

    std::cout << "Main thread: process PID = " << getpid()
              << "\nMain thread: initial measurement = " << measurement
              << "\nMain thread: address = " << &measurement << '\n';

    std::thread worker(update_measurement, std::ref(measurement));

    // Wait for the worker to finish before reading measurement.
    worker.join();

    std::cout << "Main thread: measurement after join = " << measurement << '\n';

    return 0;
}