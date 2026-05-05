#include "encryption.hpp"

std::string encrypt(const std::string& msg, const std::string& key) {
    if (key.empty()) return msg;
    
    std::string out = msg;
    for (size_t i = 0; i < out.length(); i++) {
        out[i] ^= key[i % key.length()];
    }
    return out;
}

std::string decrypt(const std::string& msg, const std::string& key) {
    return encrypt(msg, key);
}