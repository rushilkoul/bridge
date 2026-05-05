#pragma once
#include <string>

std::string encrypt(const std::string& msg, const std::string& key);
std::string decrypt(const std::string& msg, const std::string& key);