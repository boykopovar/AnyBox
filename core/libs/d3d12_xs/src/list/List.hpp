#pragma once

#include "../allocator/Allocator.hpp"
#include "../object/Lifetime.hpp"
#include "Header.hpp"
#include <memory>
#include <span>

namespace AnyBox::Graphics {
class ComputeState;
struct ComputeRecording;
class PreparedDispatch;
inline constexpr Guid CommandListId{0x7116D91C, 0xE7E4, 0x47CE, {0xB8, 0xC6, 0xEC, 0x81, 0x68, 0xF4, 0x37, 0xE5}};
inline constexpr Guid GraphicsListId{0x5B160D0F, 0xAC1B, 0x4185, {0x8B, 0xA8, 0xB3, 0xAE, 0x42, 0xA5, 0xA4, 0x55}};
inline constexpr Guid XboxGraphicsListId{0xC3827890, 0x4024, 0x474C, {0x98, 0xCF, 0x64, 0x5B, 0x51, 0xDC, 0x48, 0x74}};
inline constexpr Guid DmaListId{0x30BC77B9, 0x8379, 0x4B25, {0x95, 0xDE, 0xB8, 0x58, 0x8C, 0x42, 0x27, 0x5D}};
const ListTable& GetListTable();

struct XboxListDescription {
    D3D12_COMMAND_LIST_TYPE type;
    std::uint32_t flags;
    std::uint32_t nodeMask;
    std::uint32_t barrierBatchSize;
};

class CommandList : public ObjectLifetime<CommandList> {
public:
    CommandList(ID3D12GraphicsCommandList* host, Device* device, Allocator* allocator, D3D12_COMMAND_LIST_TYPE type, std::uint32_t flags = 0);
    ~CommandList();
    ID3D12GraphicsCommandList* GetHost() const noexcept { return host; }
    Device* GetParent() const noexcept { return device; }
    D3D12_COMMAND_LIST_TYPE GetNativeType() const noexcept { return static_cast<std::uint32_t>(guest.type) == 0x10 ? D3D12_COMMAND_LIST_TYPE_COPY : guest.type; }
    bool IsClosed() const noexcept { return (state & closedBit) != 0; }
    std::uint32_t GetCreationFlags() const noexcept { return state & ~closedBit; }
    static constexpr std::size_t GetVtableOffset() { return offsetof(CommandList, guest) + offsetof(GuestListHeader, vtable); }
    static CommandList* From(void* object);
    bool Supports(const Guid& interfaceId) const noexcept;
    static bool SupportsCreation(D3D12_COMMAND_LIST_TYPE type, const Guid& interfaceId) noexcept;
    static D3D12_COMMAND_LIST_TYPE GetType(CommandList* self);
    static HRESULT Close(CommandList* self);
    static HRESULT Reset(CommandList* self, void* allocator, void* initialState);
    static void SetComputeRootSignature(CommandList* self, void* signature);
    static void SetComputeRootDescriptorTable(CommandList* self, std::uint32_t parameter, std::uint64_t address);
    static void SetComputeRoot32BitConstant(CommandList* self, std::uint32_t parameter, std::uint32_t value, std::uint32_t offset);
    static void SetComputeRoot32BitConstants(CommandList* self, std::uint32_t parameter, std::uint32_t count, const std::uint32_t* values, std::uint32_t offset);
    static void SetComputeRootConstantBufferView(CommandList* self, std::uint32_t parameter, std::uint64_t address);
    static void SetComputeRootShaderResourceView(CommandList* self, std::uint32_t parameter, std::uint64_t address);
    static void SetComputeRootUnorderedAccessView(CommandList* self, std::uint32_t parameter, std::uint64_t address);
    const ComputeState* GetComputeState() const noexcept { return compute; }
    std::span<const std::shared_ptr<const PreparedDispatch>> GetPreparedDispatches() const noexcept;
    static void SyncTopOfPipeX(CommandList* self, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size);
    static void SyncBottomOfPipeX(CommandList* self, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size);

private:
    friend class ObjectLifetime<CommandList>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return guest.references; }
    void validateGuestCommands() const;
    void flushGuestCommands();
    void writeComputeArgument(std::uint32_t parameter, std::uint32_t offset, std::span<const std::uint32_t> words);
    void writeComputeDescriptor(std::uint32_t parameter, std::uint64_t address, std::uint32_t kind);
    void updateRootHeaders() noexcept;
    void resetGuestCommands() noexcept;
    ID3D12GraphicsCommandList7* getSynchronizationHost();
    void synchronize(bool top, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size);
    GuestListHeader guest;
    static constexpr std::size_t commandSlack = 64;
    std::array<std::uint32_t, 256> inlineCommands{};
    ID3D12GraphicsCommandList* host;
    Device* device;
    Allocator* allocator;
    static constexpr std::uint32_t closedBit = 0x80000000;
    std::uint32_t state;
    ID3D12GraphicsCommandList7* synchronizationHost = nullptr;
    ComputeState* compute = nullptr;
    ComputeRecording* recording = nullptr;
    bool nativeCommandsRecorded = false;
};

static_assert(std::is_standard_layout_v<CommandList>);
static_assert(CommandList::GetVtableOffset() == 0);
static_assert(sizeof(XboxListDescription) == 16);
}
