#include <iostream>


int lol();


int main() {
    std::cout << "\033[2J\033[H";
    std::cout << "BRIDGE starting...\n";
    lol();
    return 0;
}