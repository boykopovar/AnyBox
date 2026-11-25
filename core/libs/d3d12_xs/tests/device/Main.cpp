#include "Tests.hpp"
#include "Validation.hpp"
#include <d3d12_xs.hpp>
#include <xmem.hpp>
#include <optional>

void CheckGraphicsCache();
void CheckExtendedTextureShaderViews(AnyBox::Graphics::Device* device);
void CheckGraphicsPipelineStream();
void CheckPreparedGraphics(AnyBox::Graphics::Device* device);

int main(int argc, char** argv) {
    return AnyBox::Tests::Run([&] {
        using namespace AnyBox::Graphics;
        using namespace AnyBox::Graphics::Tests;
        if (argc > 2 || (argc == 2 && std::strcmp(argv[1], "--validate") != 0)) throw std::invalid_argument("Unknown graphics test argument");
        std::optional<NativeValidation> validation;
        if (argc == 2) validation.emplace();
        D3D12XboxCreateDeviceParameters parameters{};
        parameters.version = 0x20004;
        void* output = nullptr;
        Reject<std::invalid_argument>([&] { D3D12XboxCreateDevice(nullptr, nullptr, XboxDeviceId, &output); });
        Reject<std::invalid_argument>([&] { D3D12XboxCreateDevice(nullptr, &parameters, XboxDeviceId, nullptr); });
        auto invalid = parameters;
        invalid.version = 0;
        Reject<std::runtime_error>([&] { D3D12XboxCreateDevice(nullptr, &invalid, XboxDeviceId, &output); });
        invalid = parameters;
        invalid.processDebugFlags = 1;
        Reject<std::runtime_error>([&] { D3D12XboxCreateDevice(nullptr, &invalid, XboxDeviceId, &output); });
        AnyBox::Guid unsupported{};
        Reject<std::invalid_argument>([&] { D3D12XboxCreateDevice(nullptr, &parameters, unsupported, &output); });
        Require(!output, "Rejected graphics device creation changed its output");
        Require(D3D12XboxCreateDevice(nullptr, &parameters, XboxDeviceId, &output) == 0 && output, "Xbox graphics device creation failed");
        XMemWorkingSetStatistics statistics{};
        XMemGetWorkingSetStatistics(0x20000000, &statistics);
        Require(statistics.gpuPageTableSize == 0x1400000, "Device creation did not initialize its default graphics memory budget");
        auto* device = static_cast<Device*>(output);
        auto query = Method<decltype(&Device::QueryInterface)>(device, 0);
        auto addRef = Method<decltype(&Device::AddRef)>(device, 1);
        auto release = Method<decltype(&Device::Release)>(device, 2);
        void* repeated = nullptr;
        Require(D3D12XboxCreateDevice(nullptr, &parameters, XboxDeviceId, &repeated) == 0 && repeated == device, "Repeated graphics creation changed the device identity");
        Require(release(static_cast<Device*>(repeated)) == 1, "Repeated graphics creation did not own a reference");
        Require(addRef(device) == 2 && release(device) == 1, "Graphics device reference counting changed");
        void* identity = nullptr;
        Require(query(device, UnknownId, &identity) == 0 && identity == device, "Graphics device IUnknown identity changed");
        Require(release(static_cast<Device*>(identity)) == 1, "Graphics device interface did not own a reference");
        Require(query(device, XboxDeviceId, &identity) == 0 && identity == device, "Xbox graphics device identity changed");
        Require(release(static_cast<Device*>(identity)) == 1, "Xbox device interface did not own a reference");
        for (const auto& id : DeviceInterfaces) {
            Require(query(device, id, &identity) == 0 && identity == device, "Base device interface changed its COM identity");
            Require(release(static_cast<Device*>(identity)) == 1, "Base device interface did not own a reference");
            Require(D3D12XboxCreateDevice(nullptr, &parameters, id, &identity) == 0 && identity == device, "Base device creation returned a foreign identity");
            Require(release(static_cast<Device*>(identity)) == 1, "Base device creation did not own a reference");
        }
        Reject<std::invalid_argument>([&] { query(device, unsupported, &identity); });
        Require(!identity, "Rejected graphics interface retained a stale pointer");
        try {
            CheckProperties(device);
            CheckAllocation(device);
            CheckCommittedTextures(device);
            CheckCommittedBuffers(device);
            CheckCommittedBufferGpu(device);
            CheckCommittedBufferInvalid(device);
            CheckCommittedTextureLegacy(device);
            CheckQueues(device);
            CheckFences(device);
            CheckQueryHeaps(device);
            CheckCommandSignatures(device);
            CheckAllocators(device);
            CheckLists(device);
            CheckRoots(device);
            CheckComputeRoots(device);
            CheckComputeInline(device);
            CheckPipelines(device);
            CheckPreparedPipeline(device);
            CheckPreparedGraphics(device);
            CheckGraphicsCache();
            CheckGraphicsPipelineStream();
            CheckPreparedDispatch(device);
            CheckSynchronization(device);
            CheckPhysicalHeap(device);
            CheckPagePools(device);
            CheckPlacedBuffer(device);
            CheckDescriptorHeaps(device);
            CheckSamplers(device);
            CheckDescriptorCopies(device);
            CheckNullShaderViews(device);
            CheckNullUnorderedViews(device);
            CheckConstantViews(device);
            CheckRawShaderViews(device);
            CheckExtendedTextureShaderViews(device);
        CheckTypedShaderViews(device);
            CheckVulkanTimeline(device);
            CheckGpuQueue(device);
            CheckGpuQueueSlots(device);
            CheckCommandArena(device);
            CheckCommandRetirement(device);
            CheckInlineCommands(device);
            CheckDxgi(device);
            CheckFrameConfiguration(device);
            CheckFrameTimeline();
            CheckFrameEvents(device);
            CheckFrameAncillary(device);
            CheckUnsupportedDeviceMethods(device);
        } catch (...) {
            if (validation) CheckValidation(device);
            throw;
        }
        if (validation) CheckValidation(device);
        Require(release(device) == 0, "Graphics device final reference was not released");
    });
}
