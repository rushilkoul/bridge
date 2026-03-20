#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

int lol()
{
    struct winsize w;
    int columns, rows;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        rows = w.ws_row;
        columns = w.ws_col;
    }
    std::cout << "\033[47;30m";

    for (int i = 0; i < columns/2 - 7; i++) {
        std::cout << " ";
    }

    std::cout << "BRIDGE v0.01a";

    for (int i = 1; i < columns/2 - 4; i++) {
        std::cout << " ";
    }
    std::cout << "\033[0m" << std::endl;

    // print a vertical divider
    int t = -1;
    for(int i = 0; i < rows - 5; i++) {
        for(int i = 0; i < columns*3/10; i++) {
        std::cout << " ";
        }
        std::cout << "│ ";
        if (t == -1) {
            std::cout << "\033[32m" << "You are now connected to 192.168.29.65: " << "\033[42m\033[37m" << "RUSHIL'S PC " << "\033[0m\n";
            t = 0;
            continue;
        }

        if(t == 0) {
            std::cout << "\033[36m" << "You: " << "\033[0m" << "hello test message!\n";
            t = 1;
            continue;
        }
        if(t == 1) {
            std::cout << "\n";
            t = 2;
            continue;
        }
        if(t == 2) {
            std::cout << "\033[33m" << "Rushil: " << "\033[0m" << "hi!\n";
            t = 3;
            continue;
        }
        if(t == 3) {
            std::cout << "\n";
            t = 0;
            continue;
        }
    }
    return 0;
}

// cool lines
// │  vertical
// ─  horizontal
// ┌ ┐ └ ┘  corners
// ├ ┤ ┬ ┴ ┼  junctions