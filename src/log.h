#pragma once

#include <iostream>

enum LogType {
    DEBUG = 32,
    WARN  = 33,
    ERROR = 31
};

static void log(LogType type, std::string message)
{
    std::cout << "[\x1b[1;" << type << "m";
    std::cout << (type == DEBUG ? "DEBUG" : type == WARN ? "WARN" : "ERROR");
    std::cout << "\x1b[;39m] " << message << "\n";
    if (type == ERROR) std::abort(); // Exit with a backtrace
}
