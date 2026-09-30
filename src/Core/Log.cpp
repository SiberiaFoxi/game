#include "Log.h"
#include <ctime>

namespace stalker {

FILE* Log::s_file = nullptr;

void Log::Open(const char* path) {
    if (s_file) return;
    s_file = fopen(path, "w");
    if (!s_file) return;
    std::time_t t = std::time(nullptr);
    fprintf(s_file, "=== S.T.A.L.K.E.R. mod for GTA SA — log ===\n");
    fprintf(s_file, "Started: %s\n", std::ctime(&t));
    fflush(s_file);
}

void Log::Close() {
    if (s_file) {
        fclose(s_file);
        s_file = nullptr;
    }
}

void Log::Write(const char* fmt, ...) {
    if (!s_file) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(s_file, fmt, args);
    va_end(args);
    fputc('\n', s_file);
    fflush(s_file);
}

void Log::Error(const char* fmt, ...) {
    if (!s_file) return;
    fputs("[ERROR] ", s_file);
    va_list args;
    va_start(args, fmt);
    vfprintf(s_file, fmt, args);
    va_end(args);
    fputc('\n', s_file);
    fflush(s_file);
}

} // namespace stalker
