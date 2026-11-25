#include "Resource.hpp"
#include "../pipeline/Offline.hpp"
#include "../pipeline/State.hpp"
#include <anybox/Unimplemented.hpp>
#include <BdaAbi.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace {
using namespace AnyBox;
using namespace AnyBox::Graphics;
namespace Abi = ShaderRecompiler::BdaAbi;

constexpr VkMemoryPropertyFlags HostStorage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

class DispatchResources : public Shader::IRootResources {
public:
    explicit DispatchResources(Device* device) : device(device) {}

    Shader::RootBufferValue ResolveDescriptor(const Shader::BufferBinding& binding, std::uint64_t address) override {
        if (binding.resourceClass != Shader::ResourceClass::UnorderedAccess || binding.kind != Shader::BufferKind::Raw) throw std::invalid_argument("Prepared compute requires raw root UAV descriptors");
        if (address % alignof(std::uint32_t) != 0) throw std::invalid_argument("Prepared compute root UAV address is misaligned");
        auto resource = Resource::AcquirePlacedBuffer(device, address);
        const auto& metadata = resource->GetMetadata();
        if ((metadata.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0) throw std::invalid_argument("Prepared compute root UAV buffer does not allow unordered access");
        auto offset = address - Resource::GetGPUVirtualAddress(resource.get());
        if (std::ranges::find(buffers, resource) == buffers.end()) buffers.push_back(std::move(resource));
        return {address, metadata.Width - offset};
    }

    Shader::RootBufferValue ResolveTable(const Shader::BufferBinding&, std::uint32_t, std::uint32_t) override {
        throw std::invalid_argument("Prepared compute descriptor tables are not implemented");
    }

    std::vector<std::shared_ptr<Resource>> buffers;

private:
    Device* device;
};

struct DispatchDescriptors {
    explicit DispatchDescriptors(Vulkan::Context& context) : context(context), faults(Vulkan::Buffer::Allocate(context, Abi::FaultBufferBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HostStorage)) {
        std::ranges::fill(faults.Map(), std::byte{});
    }

    ~DispatchDescriptors() {
        if (pool) context.GetFunctions().destroyDescriptorPool(context.GetDevice(), pool, nullptr);
    }

    Vulkan::Context& context;
    Vulkan::Buffer faults;
    VkDescriptorPool pool = VK_NULL_HANDLE;
};

VkDescriptorSet CreateDispatchDescriptors(DispatchDescriptors& descriptors, VkDescriptorSetLayout layout, const Vulkan::Buffer& table) {
    const auto& functions = descriptors.context.GetFunctions();
    auto device = descriptors.context.GetDevice();
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &size;
    Vulkan::Check(functions.createDescriptorPool(device, &poolInfo, nullptr, &descriptors.pool), "Create prepared compute descriptor pool");

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = descriptors.pool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &layout;
    Vulkan::Check(functions.allocateDescriptorSets(device, &allocation, &set), "Allocate prepared compute descriptor set");

    constexpr std::array bindings{59u, 60u};
    const std::array infos{VkDescriptorBufferInfo{table.GetHandle(), 0, table.GetSize()}, VkDescriptorBufferInfo{descriptors.faults.GetHandle(), 0, descriptors.faults.GetSize()}};
    std::array<VkWriteDescriptorSet, bindings.size()> writes{};
    for (std::size_t index = 0; index < writes.size(); ++index) {
        writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[index].dstSet = set;
        writes[index].dstBinding = bindings[index];
        writes[index].descriptorCount = 1;
        writes[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[index].pBufferInfo = &infos[index];
    }
    functions.updateDescriptorSets(device, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return set;
}
}

AnyBox::Graphics::BufferBacking AnyBox::Graphics::CreateBufferBacking(Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description) {
    static_assert(sizeof(D3D12_RESOURCE_DESC1) == 64);
    if (description.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
    if (address == 0 || address % PhysicalPageSize != 0 || description.Width == 0 || description.Width > std::numeric_limits<std::size_t>::max() - (PhysicalPageSize - 1)) throw std::invalid_argument("Placed buffer address or width is invalid");
    if (description.Height != 1 || description.DepthOrArraySize != 1 || description.MipLevels != 1 || description.Format != DXGI_FORMAT_UNKNOWN || description.SampleDesc.Count != 1 || description.SampleDesc.Quality != 0 || description.Layout != D3D12_TEXTURE_LAYOUT_ROW_MAJOR) throw std::invalid_argument("Placed buffer description is invalid");
    if (description.Alignment != 0 && description.Alignment != PhysicalPageSize) ANYBOX_NOT_IMPLEMENTED();
    if (description.SamplerFeedbackMipRegion.Width != 0 || description.SamplerFeedbackMipRegion.Height != 0 || description.SamplerFeedbackMipRegion.Depth != 0) throw std::invalid_argument("Placed buffer cannot have a sampler feedback mip region");
    constexpr auto allowedFlags = static_cast<std::uint32_t>(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) | 0x20000u;
    if ((static_cast<std::uint32_t>(description.Flags) & ~allowedFlags) != 0) ANYBOX_NOT_IMPLEMENTED();

    auto count = static_cast<std::size_t>((description.Width + PhysicalPageSize - 1) / PhysicalPageSize);
    BufferBacking backing;
    backing.pages.reset(AnyBoxAcquireMappedPhysicalPages(reinterpret_cast<void*>(address), count));
    std::span memory(reinterpret_cast<std::byte*>(address), count * PhysicalPageSize);
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    auto buffer = Vulkan::Buffer::Import(device->GetGpuContext(), memory, description.Width, usage);
    backing.resource = std::make_unique<Vulkan::Buffer>(std::move(buffer));
    return backing;
}

AnyBox::Graphics::PreparedDispatch::PreparedDispatch(std::shared_ptr<PipelineState> pipeline, std::vector<std::shared_ptr<Resource>> buffers, Vulkan::Buffer table, std::vector<std::uint32_t> constants, const std::array<std::uint32_t, 3>& groups) : pipeline(std::move(pipeline)), buffers(std::move(buffers)), table(std::move(table)), constants(std::move(constants)), groups(groups) {}

std::shared_ptr<const void> AnyBox::Graphics::PreparedDispatch::Record(VkCommandBuffer commands) const {
    auto& context = pipeline->GetParent()->GetGpuContext();
    const auto& prepared = pipeline->GetOffline()->GetPipeline();
    auto descriptors = std::make_shared<DispatchDescriptors>(context);
    std::array sets{CreateDispatchDescriptors(*descriptors, prepared.GetDescriptorLayout(), table)};
    prepared.GetPipeline().Dispatch(commands, std::as_bytes(std::span(constants)), groups, sets);
    return descriptors;
}

std::shared_ptr<const AnyBox::Graphics::PreparedDispatch> AnyBox::Graphics::PrepareComputeDispatch(std::shared_ptr<PipelineState> pipeline, const Commands::RootState& arguments, const std::array<std::uint32_t, 3>& groups) {
    if (!pipeline || !pipeline->GetOffline() || !pipeline->GetOffline()->SupportsPlacedUavs()) throw std::invalid_argument("Prepared compute pipeline requires the BDA table and fault bindings");
    auto* device = pipeline->GetParent();
    auto& context = device->GetGpuContext();
    const auto& limits = context.GetProperties().limits;
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (groups[index] > limits.maxComputeWorkGroupCount[index]) throw std::out_of_range("Prepared compute dispatch exceeds the workgroup count limit");
    }

    const auto& offline = *pipeline->GetOffline();
    const auto& writer = offline.GetArgumentWriter();
    std::vector<std::uint32_t> constants(offline.GetPushConstantSize() / sizeof(std::uint32_t));
    auto start = offline.GetPushDataStart();
    auto words = writer.GetWordCount();
    if (start > constants.size() || words > constants.size() - start) throw std::invalid_argument("Prepared compute push constant arguments exceed the pipeline range");
    DispatchResources resources(device);
    writer.Write(arguments, resources, std::span(constants).subspan(start, words));
    if (resources.buffers.empty()) throw std::invalid_argument("Prepared compute dispatch has no placed UAV buffer");

    std::vector<Abi::Range> ranges;
    for (const auto& resource : resources.buffers) {
        auto begin = Resource::GetGPUVirtualAddress(resource.get());
        auto width = resource->GetMetadata().Width;
        auto deviceAddress = resource->GetGpuBuffer().GetAddress();
        constexpr auto maxAddress = std::numeric_limits<std::uint64_t>::max();
        if (width > maxAddress - begin || width > maxAddress - deviceAddress) throw std::invalid_argument("Prepared compute placed UAV address range overflows");
        ranges.push_back({begin, begin + width, deviceAddress, Abi::Read | Abi::Write, 0});
    }
    std::ranges::sort(ranges, {}, &Abi::Range::begin);
    for (std::size_t index = 1; index < ranges.size(); ++index) {
        if (ranges[index].begin < ranges[index - 1].end) throw std::invalid_argument("Prepared compute UAV ranges overlap");
    }
    auto tableSize = sizeof(Abi::Header) + ranges.size() * sizeof(Abi::Range);
    if (tableSize > limits.maxStorageBufferRange) throw std::length_error("Prepared compute BDA table exceeds the device storage buffer limit");
    auto table = Vulkan::Buffer::Allocate(context, tableSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HostStorage);
    auto mapped = table.Map();
    const Abi::Header header{Abi::Version, static_cast<std::uint32_t>(ranges.size()), sizeof(Abi::Range), 0};
    std::memcpy(mapped.data(), &header, sizeof(header));
    std::memcpy(mapped.data() + sizeof(header), ranges.data(), ranges.size() * sizeof(Abi::Range));
    return std::shared_ptr<const PreparedDispatch>(new PreparedDispatch(std::move(pipeline), std::move(resources.buffers), std::move(table), std::move(constants), groups));
}
