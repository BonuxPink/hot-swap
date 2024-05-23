#include <iostream>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <sys/select.h>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <dlfcn.h>
#include <termios.h>
#include <fcntl.h>

using namespace std::chrono_literals;

using fooFunc_t = int(*)();
fooFunc_t fooFunc{};

void* handle{};

// The function that 'updates'
void reload()
{
    // Close old library
    int ret = dlclose(handle);
    if (ret != 0)
    {
        fmt::print("dlclose failed with: {}\n", dlerror());
        std::exit(-1);
    }

    handle = nullptr;

    // Open new one
    handle = dlopen("./libFoo2.so", RTLD_NOW);
    if (!handle)
    {
        fmt::print("dlopen failed with: {}\n", dlerror());
        std::exit(-1);
    }

    fooFunc = std::bit_cast<fooFunc_t>(dlsym(handle, "fooFunc2"));
    if (fooFunc == nullptr)
    {
        fmt::print("Failed to get fooFunc2\n");
        std::exit(-1);
    }
}

char getCharInput()
{
    char input{};
    std::cin >> input;
    return input;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(1);
    }
}

void set_terminal_mode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

int main()

{    // Set stdin to non-blocking mode
    set_nonblocking(STDIN_FILENO);

    // Set terminal to non-canonical mode
    set_terminal_mode(true);

    bool exit = false;

    handle = dlopen("./libFoo1.so", RTLD_NOW);
    if (!handle)
    {
        fmt::print("Failed opening libFoo.so\n");
        return -1;
    }

    // Get a function out of libFoo
    fooFunc = std::bit_cast<fooFunc_t>(dlsym(handle, "fooFunc1"));

    std::ofstream logFile("log.txt", std::ios_base::app);
    if (!logFile)
    {
        fmt::print("Creating log.txt failed\n");
        return -1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1)
    {
        fmt::print("fcntl1 failed\n");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(STDERR_FILENO, F_SETFL, flags) == -1)
    {
        fmt::print("fcntl2 failed\n");
        return -1;
    }

    fmt::print("press q - Quit, r - Update\n");
    do
    {
        /*
         * Async io takes a lot of space
         */
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 1;

        char buffer[64];
        int result = select(STDERR_FILENO + 1, &read_fds, nullptr, nullptr, &tv);
        if (result > 0)
        {
            if (FD_ISSET(STDIN_FILENO, &read_fds))
            {
                auto bread = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

                if (bread > 0)
                {
                    buffer[bread] = 0;
                }
                else if (bread == 0)
                {
                    exit = true;
                    break;
                }
            }
        }

        if (buffer[0] == 'q')
        {
            exit = true;
        }

        if (buffer[0] == 'r')
        {
            reload();
        }

        memset(&buffer, '\0', sizeof buffer);

        const int ret = fooFunc();
        const auto now = std::chrono::high_resolution_clock::now();
        logFile << fmt::format("[{}] fooFunc returned: {}\n", now, ret);

        std::this_thread::sleep_for(100ms);
    } while(!exit);


    // Restore terminal settings
    set_terminal_mode(false);

    fmt::print("Exiting\n");
}
