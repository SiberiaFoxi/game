#pragma once
// Простой лог-файл (STALKER_MOD/stalker_mod.log)
#include <cstdio>
#include <cstdarg>

namespace stalker {

class Log {
public:
    static void Open(const char* path);
    static void Close();
    static void Write(const char* fmt, ...);
    static void Error(const char* fmt, ...);

private:
    static FILE* s_file;
};

} // namespace stalker
