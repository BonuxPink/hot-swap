#include <fmt/format.h>
#include <fmt/chrono.h>

#include <chrono>

using namespace std::chrono_literals;

// has to be extern, to disable mangling
extern "C" int fooFunc2()
{
    auto now = std::chrono::high_resolution_clock::now();
    fmt::print("fooFunc2: {}\n", now);
    return 42;
}
