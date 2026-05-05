#include "encryption.hpp"
static const char KEY = 'K'; 

std::string encrypt(const std::string& msg) {
    std::string out = msg;
    for (char &c : out) {
        c ^= KEY;
    }
    return out;
}

std::string decrypt(const std::string& msg) {
    return encrypt(msg);
}