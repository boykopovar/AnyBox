#include "List.hpp"
#include "Compute.hpp"
#include "../pipeline/Offline.hpp"
#include "../pipeline/State.hpp"
#include <anybox/graphics/commands/Inline.hpp>
#include <iterator>
#include <optional>
#include <stdexcept>

void AnyBox::Graphics::CommandList::resetGuestCommands() noexcept {
    guest.commandWrite = inlineCommands.data();
    guest.dispatchEnd = inlineCommands.data() + inlineCommands.size() - commandSlack;
    guest.commandEnd = guest.dispatchEnd;
    std::uint32_t clean = 0;
    std::memcpy(guest.driverState.data() + 320 - 56, &clean, sizeof(clean));
    updateRootHeaders();
}

void AnyBox::Graphics::CommandList::validateGuestCommands() const {
    auto begin = reinterpret_cast<std::uintptr_t>(inlineCommands.data());
    auto end = begin + sizeof(inlineCommands);
    auto visibleEnd = end - commandSlack * sizeof(std::uint32_t);
    auto write = reinterpret_cast<std::uintptr_t>(guest.commandWrite);
    if (write < begin || write > end || (write - begin) % sizeof(std::uint32_t) != 0 || reinterpret_cast<std::uintptr_t>(guest.dispatchEnd) != visibleEnd || reinterpret_cast<std::uintptr_t>(guest.commandEnd) != visibleEnd) throw std::logic_error("Xbox command stream bounds are corrupted");
}

void AnyBox::Graphics::CommandList::updateRootHeaders() noexcept {
    auto count = compute ? compute->GetRoot()->GetLayout().parameters.size() : 0;
    for (std::uint32_t index = 0; index < 64; ++index) {
        auto header = index < count ? Commands::RootHeader(index) : UINT32_MAX;
        std::memcpy(guest.driverState.data() + 0x344 - 56 + index * 4, &header, 4);
    }
}

void AnyBox::Graphics::CommandList::flushGuestCommands() {
    validateGuestCommands();
    if (guest.commandWrite == inlineCommands.data()) return;
    auto words = std::span(inlineCommands.data(), static_cast<std::size_t>(guest.commandWrite - inlineCommands.data()));
    std::optional<Commands::RootState> arguments;
    if (compute) arguments.emplace(compute->GetArguments());
    ComputeRecording pending;
    if (recording) pending.pipeline = recording->pipeline;
    Commands::InlineReader reader(words);
    while (!reader.Empty()) {
        auto command = reader.Next(arguments ? &*arguments : nullptr);
        if (auto* update = std::get_if<Commands::RootWrite>(&command)) {
            arguments->Write(update->parameter, update->offset, update->words);
            continue;
        }
        if (guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Prepared inline dispatch requires a compute command list");
        if (auto* bind = std::get_if<Commands::PipelineBind>(&command)) {
            pending.pipeline = PipelineState::Acquire(bind->address, device);
            continue;
        }
        const auto& dispatch = std::get<Commands::Dispatch>(command);
        if (dispatch.header != Commands::DispatchHeader) throw std::runtime_error("Xbox inline compute dispatch mode is not implemented");
        if (nativeCommandsRecorded) throw std::runtime_error("Mixed native and prepared compute commands are not implemented");
        if (!pending.pipeline || !compute) throw std::runtime_error("Prepared inline dispatch requires a pipeline and compute root signature");
        if (compute->GetRoot()->GetLayout() != pending.pipeline->GetRootSignature()->GetLayout()) throw std::invalid_argument("Inline dispatch root signature does not match the prepared compute pipeline");
        pending.dispatches.push_back(PrepareComputeDispatch(pending.pipeline, *arguments, dispatch.groups));
    }
    if (recording) {
        recording->dispatches.insert(recording->dispatches.end(), std::make_move_iterator(pending.dispatches.begin()), std::make_move_iterator(pending.dispatches.end()));
        recording->pipeline = std::move(pending.pipeline);
    } else if (pending.pipeline) {
        recording = new ComputeRecording(std::move(pending));
    }
    if (arguments) compute->GetArguments() = std::move(*arguments);
    guest.commandWrite = inlineCommands.data();
}

std::span<const std::shared_ptr<const AnyBox::Graphics::PreparedDispatch>> AnyBox::Graphics::CommandList::GetPreparedDispatches() const noexcept {
    if (recording) return recording->dispatches;
    return {};
}
