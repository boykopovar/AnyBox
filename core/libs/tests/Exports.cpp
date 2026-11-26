#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <initializer_list>

namespace {

struct DllExports {
    const char* dll;
    std::initializer_list<const char*> names;
};

const DllExports expected[] = {
    {"AcpHal.dll", {"AcpHalAllocateShapeContexts", "AcpHalCreate", "AcpHalReleaseShapeContexts",
                    "ApuAlloc", "ApuCreateHeap", "ApuFree", "ApuIsVirtualAddressValid",
                    "ApuMapApuAddress", "ApuMapVirtualAddress"}},
    {"chad.dll", {"XDspActivate", "XDspConnectWithMaximumStreamLimit", "XDspDeactivate",
                  "XDspDisconnect", "XDspSubmitCommand"}},
    {"d3d12_xs.dll", {"D3D12ConfigureVirtualMemoryX", "D3D12CreateRootSignatureDeserializer",
                     "D3D12SerializeVersionedRootSignature", "D3D12XboxCreateDevice"}},
    {"xapu.dll", {"XApuConnect", "XApuDequeueResult", "XApuDisconnect",
                  "XApuEnqueueCommand", "XApuGetResultCount"}},
    {"xg_xs.dll", {"XGCreateTextureComputer", "XGSuggestSwizzleMode"}},
    {"xmem.dll", {"XMemAlloc", "XMemAllocDefault", "XMemAllocatePhysicalPages", "XMemFree",
                  "XMemFreeDefault", "XMemFreePhysicalPages", "XMemGetWorkingSetStatistics",
                  "XMemIsToolsMemAvailable", "XMemRegisterGraphicsMemoryConfiguration",
                  "XMemMapPhysicalPages", "XMemMapPhysicalPagesScatter", "XMemTransferMemory",
                  "XMemVirtualAlloc", "XMemVirtualQuery", "AnyBoxAcquirePhysicalPages",
                  "AnyBoxAcquireMappedPhysicalPages"}},
    {"XCRDAPI.dll", {"DStorageGetFactory"}},
};

}

int main() {
    std::array<wchar_t, 32768> executablePath{};
    const auto length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (length == 0 || length == executablePath.size()) {
        std::fprintf(stderr, "Could not locate the test executable (Win32 error %lu)\n", GetLastError());
        return 1;
    }
    const auto directory = std::filesystem::path(executablePath.data()).parent_path();
    bool allFound = true;
    for (const auto& library : expected) {
        const auto path = directory / library.dll;
        const auto module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            std::fprintf(stderr, "%s failed to load (Win32 error %lu)\n", library.dll, GetLastError());
            allFound = false;
            continue;
        }
        for (const char* name : library.names) {
            if (!GetProcAddress(module, name)) {
                std::fprintf(stderr, "%s is missing export %s\n", library.dll, name);
                allFound = false;
            }
        }
        FreeLibrary(module);
    }
    return allFound ? 0 : 1;
}
