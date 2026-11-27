#include <chad.hpp>
#include <xg_xs.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

constexpr wchar_t traceVariable[] = L"ANYBOX_TRACE_FILE";
constexpr auto dspExport = "chad.dll!XDspConnectWithMaximumStreamLimit";
constexpr auto textureExport = "xg_xs.dll!XGCreateTextureComputer";
constexpr std::size_t threadCount = 8;
constexpr unsigned attempts = 12;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TraceDirectory {
public:
    TraceDirectory()
        : directory(std::filesystem::current_path() /
                    (L"AnyBox trace \u03A9 " + std::to_wstring(GetCurrentProcessId()) +
                     L"-" + std::to_wstring(GetTickCount64()))),
          log(directory / L"calls.tsv") {
        Require(std::filesystem::create_directory(directory), "Could not create trace test directory");
    }

    ~TraceDirectory() {
        std::error_code error;
        std::filesystem::remove(log, error);
        std::filesystem::remove(directory, error);
    }

    const std::filesystem::path directory;
    const std::filesystem::path log;
};

template<class Call>
void ExpectUnimplemented(std::string_view exportName, Call call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        Require(error.what() == std::string(exportName) + " is not implemented", "Trace changed the exception message");
        return;
    }
    throw std::runtime_error("Unimplemented export did not throw");
}

void CallUnsupportedDspMode() {
    ExpectUnimplemented(dspExport, [] {
        std::array<std::byte, 0x10000> memory{};
        XDspConnection* connection = nullptr;
        XDspConnectWithMaximumStreamLimit(memory.data(), static_cast<std::uint32_t>(memory.size()), 0, 1, 0, &connection);
    });
}

void CallTextureStub() {
    ExpectUnimplemented(textureExport, [] { XGCreateTextureComputer(nullptr, nullptr); });
}

void CheckTrace(const std::filesystem::path& path, const std::array<DWORD, threadCount>& callers) {
    std::ifstream trace(path);
    Require(trace.is_open(), "The trace file was not created");

    const auto expectedProcess = std::to_string(GetCurrentProcessId());
    std::map<std::string, unsigned> callCounts;
    unsigned dspCalls = 0;
    unsigned textureCalls = 0;
    std::string line;
    while (std::getline(trace, line)) {
        std::istringstream record(line);
        std::string timestamp;
        std::string process;
        std::string thread;
        std::string exportName;
        Require(static_cast<bool>(std::getline(record, timestamp, '\t') &&
                                  std::getline(record, process, '\t') &&
                                  std::getline(record, thread, '\t') &&
                                  std::getline(record, exportName)),
                "Trace record is incomplete");
        Require(timestamp.size() == 24 && timestamp[10] == 'T' && timestamp.back() == 'Z', "Trace timestamp is malformed");
        Require(process == expectedProcess, "Trace recorded the wrong process");
        Require(exportName == dspExport || exportName == textureExport, "Trace recorded the wrong export");
        ++callCounts[thread];
        if (exportName == dspExport) ++dspCalls;
        if (exportName == textureExport) ++textureCalls;
    }
    Require(trace.eof(), "Could not finish reading trace file");
    Require(callCounts.size() == threadCount + 1, "Trace recorded the wrong number of threads");
    Require(callCounts[std::to_string(GetCurrentThreadId())] == 2, "Main thread trace lost or appended records");
    for (const auto caller : callers) {
        Require(callCounts[std::to_string(caller)] == attempts, "Worker thread trace lost or duplicated records");
    }
    Require(dspCalls == threadCount * attempts / 2 + 1, "DSP trace count was wrong");
    Require(textureCalls == threadCount * attempts / 2 + 1, "Texture trace count was wrong");
}

}

int main() {
    try {
        const TraceDirectory paths;

        Require(SetEnvironmentVariableW(traceVariable, paths.log.c_str()) != 0, "Could not enable tracing");
        CallUnsupportedDspMode();
        CallTextureStub();

        std::array<DWORD, threadCount> callers{};
        std::array<std::exception_ptr, threadCount> errors{};
        std::array<std::jthread, threadCount> threads;
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads[i] = std::jthread([&, i] {
                callers[i] = GetCurrentThreadId();
                try {
                    for (unsigned attempt = 0; attempt < attempts; ++attempt) {
                        if (attempt % 2 == 0) CallUnsupportedDspMode();
                        else CallTextureStub();
                    }
                } catch (...) {
                    errors[i] = std::current_exception();
                }
            });
        }
        for (auto& thread : threads) thread.join();
        for (const auto& error : errors) if (error) std::rethrow_exception(error);

        Require(SetEnvironmentVariableW(traceVariable, paths.directory.c_str()) != 0, "Could not set an unwritable trace location");
        CallUnsupportedDspMode();
        Require(SetEnvironmentVariableW(traceVariable, nullptr) != 0, "Could not disable tracing");
        CallTextureStub();

        CheckTrace(paths.log, callers);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
