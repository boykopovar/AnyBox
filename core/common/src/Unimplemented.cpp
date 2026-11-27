#include <anybox/Unimplemented.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

std::wstring TracePath() {
    constexpr wchar_t variable[] = L"ANYBOX_TRACE_FILE";
    const auto required = GetEnvironmentVariableW(variable, nullptr, 0);
    if (required == 0) return {};

    std::wstring path(required, L'\0');
    const auto length = GetEnvironmentVariableW(variable, path.data(), required);
    if (length == 0 || length >= required) return {};
    path.resize(length);
    return path;
}

void TraceUnimplemented(const char* libraryName, const char* functionName) {
    const auto path = TracePath();
    if (path.empty()) return;

    SYSTEMTIME time{};
    GetSystemTime(&time);
    char line[512];
    const auto length = std::snprintf(line, sizeof(line),
                                      "%04hu-%02hu-%02huT%02hu:%02hu:%02hu.%03huZ\t%lu\t%lu\t%s!%s\n",
                                      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
                                      time.wSecond, time.wMilliseconds, GetCurrentProcessId(),
                                      GetCurrentThreadId(), libraryName, functionName);
    if (length <= 0 || static_cast<std::size_t>(length) >= sizeof(line)) return;

    const auto file = CreateFileW(path.c_str(), FILE_APPEND_DATA,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    if (GetFileType(file) == FILE_TYPE_DISK) {
        // Write an entire record through one append operation, including across DLLs.
        DWORD written = 0;
        WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr);
    }
    CloseHandle(file);
}

}

[[noreturn]] void AnyBox::NotImplemented(const char* libraryName, const char* functionName) {
    const auto lastError = GetLastError();
    const int savedErrno = errno;
    try {
        TraceUnimplemented(libraryName, functionName);
    } catch (...) {
        // Diagnostics must never replace the existing exception.
    }
    errno = savedErrno;
    SetLastError(lastError);
    throw std::runtime_error(std::string(libraryName) + "!" + functionName + " is not implemented");
}
