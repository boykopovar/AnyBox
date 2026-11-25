#include "List.hpp"
#include "Gpu.hpp"
#include "../pipeline/State.hpp"
#include "../pipeline/Offline.hpp"

using namespace AnyBox::Graphics;

void CommandList::validateDispatch(PipelineState* selected, const std::array<std::uint32_t, 3>& groups) const {
    if (IsClosed()) throw std::logic_error("Compute dispatch requires a recording command list");
    if (gpu) gpu->Check();
    if (guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT && guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Compute dispatch is not implemented for this command list type");
    if (!selected || !compute) throw std::logic_error("Compute dispatch requires a pipeline and root signature");
    if (selected->GetRootSignature() != compute->GetRoot()) throw std::invalid_argument("Compute pipeline and bound root signatures differ");
    if (selected->GetNativeCompute()) throw std::runtime_error("Native compute dispatch resource binding is not implemented");
    if (!selected->GetOffline()) throw std::runtime_error("Compute dispatch requires a prepared Vulkan shader");
    if (selected->GetOffline()->HasVolatileTables()) throw std::runtime_error("Volatile compute descriptor tables are not implemented");
    const auto& limits = device->GetGpuContext().GetProperties().limits;
    for (unsigned index = 0; index < groups.size(); ++index) if (groups[index] > limits.maxComputeWorkGroupCount[index]) throw std::out_of_range("Compute dispatch exceeds the device workgroup count limit");
}

void CommandList::recordDispatch(const std::array<std::uint32_t, 3>& groups) {
    validateDispatch(pipeline, groups);
    if (!gpu) gpu = new GpuRecording(*device, *allocator);
    try { gpu->Dispatch(*pipeline, *compute, groups); }
    catch (...) { gpu->Fail(); throw; }
}

void CommandList::Dispatch(CommandList* self, std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    self->flushGuestCommands();
    self->recordDispatch({x, y, z});
}
