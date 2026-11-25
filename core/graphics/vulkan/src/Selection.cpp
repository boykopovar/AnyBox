#include <anybox/vulkan/Context.hpp>
#include <algorithm>
#include <bit>
#include <cstring>

void AnyBox::Vulkan::Context::selectAdapter(const AdapterSelection& selection, std::uint32_t queueCount) {
    findAdapter(selection);
    std::uint32_t count = 0;
    Check(instanceFunctions.enumerateExtensions(physicalDevice, nullptr, &count, nullptr), "Count Vulkan device extensions");
    std::vector<VkExtensionProperties> extensions(count);
    Check(instanceFunctions.enumerateExtensions(physicalDevice, nullptr, &count, extensions.data()), "Enumerate Vulkan device extensions");
    auto hasExtension = [&](const char* name) {
        return std::ranges::any_of(extensions, [name](const auto& value) { return std::strcmp(value.extensionName, name) == 0; });
    };
    subgroupSizeExtension = hasExtension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
    VkPhysicalDeviceExternalMemoryHostPropertiesEXT host{};
    host.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
    VkPhysicalDeviceIDProperties identity{};
    identity.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    subgroups.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    floatControls.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
    if (subgroupSizeExtension) {
        subgroupSizeLimits.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
        floatControls.pNext = &subgroupSizeLimits;
    }
    identity.pNext = &subgroups;
    subgroups.pNext = &floatControls;
    host.pNext = &identity;
    VkPhysicalDeviceProperties2 physical{};
    physical.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physical.pNext = &host;
    instanceFunctions.getPhysicalDeviceProperties(physicalDevice, &physical);
    properties = physical.properties;
    if (!identity.deviceLUIDValid) throw std::runtime_error("Vulkan adapter has no Windows LUID");
    std::memcpy(adapterLuid.data(), identity.deviceLUID, adapterLuid.size());
    if (properties.apiVersion < VK_API_VERSION_1_2) throw std::runtime_error("Vulkan adapter does not support API version 1.2");
    hostPointerAlignment = host.minImportedHostPointerAlignment;
    instanceFunctions.getMemoryProperties(physicalDevice, &memoryProperties);
    instanceFunctions.getQueueFamilies(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    instanceFunctions.getQueueFamilies(physicalDevice, &count, families.data());
    constexpr auto required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    auto family = std::ranges::find_if(families, [&](const auto& value) { return (value.queueFlags & required) == required && value.queueCount >= queueCount; });
    if (family == families.end()) throw std::runtime_error("Vulkan adapter has no universal queue family with the requested queue count");
    queueFamily = static_cast<std::uint32_t>(family - families.begin());
    if (!hasExtension(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME)) throw std::runtime_error("Vulkan host memory imports are unsupported");
    if (hostPointerAlignment == 0) throw std::runtime_error("Vulkan host pointer alignment is zero");
    if (!hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)) throw std::runtime_error("Vulkan Windows semaphore imports are unsupported");
}

void AnyBox::Vulkan::Context::createDevice(std::uint32_t queueCount) {
    VkPhysicalDeviceBufferDeviceAddressFeatures address{};
    address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline{};
    timeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    VkPhysicalDevice8BitStorageFeatures storage{};
    storage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    VkPhysicalDeviceSubgroupSizeControlFeatures subgroupFeatures{};
    subgroupFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
    if (subgroupSizeExtension) storage.pNext = &subgroupFeatures;
    timeline.pNext = &storage;
    address.pNext = &timeline;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &address;
    instanceFunctions.getFeatures(physicalDevice, &features);
    if (!address.bufferDeviceAddress) throw std::runtime_error("Vulkan buffer device addresses are unsupported");
    if (!features.features.shaderInt64) throw std::runtime_error("Vulkan shader 64-bit integers are unsupported");
    if (!timeline.timelineSemaphore) throw std::runtime_error("Vulkan timeline semaphores are unsupported");
    VkPhysicalDeviceFeatures enabled{};
    enabled.shaderInt64 = VK_TRUE;
    enabled.textureCompressionBC = features.features.textureCompressionBC;
    enabled.textureCompressionETC2 = features.features.textureCompressionETC2;
    enabled.textureCompressionASTC_LDR = features.features.textureCompressionASTC_LDR;
    enabled.imageCubeArray = features.features.imageCubeArray;
    cubeArrays = enabled.imageCubeArray != VK_FALSE;
    byteStorage = storage.storageBuffer8BitAccess != VK_FALSE;
    subgroupSizeControl = subgroupFeatures.subgroupSizeControl != VK_FALSE;
    computeFullSubgroups = subgroupFeatures.computeFullSubgroups != VK_FALSE;
    auto enableSubgroupExtension = subgroupSizeControl || computeFullSubgroups;
    if (!enableSubgroupExtension) storage.pNext = nullptr;
    storage.uniformAndStorageBuffer8BitAccess = VK_FALSE;
    storage.storagePushConstant8 = VK_FALSE;
    address.bufferDeviceAddressCaptureReplay = VK_FALSE;
    address.bufferDeviceAddressMultiDevice = VK_FALSE;
    std::vector<float> priorities(queueCount, 1.0f);
    VkDeviceQueueCreateInfo queue{};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueFamilyIndex = queueFamily;
    queue.queueCount = queueCount;
    queue.pQueuePriorities = priorities.data();
    const char* extensions[]{VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME};
    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext = &address;
    info.pEnabledFeatures = &enabled;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = enableSubgroupExtension ? 3 : 2;
    info.ppEnabledExtensionNames = extensions;
    Check(instanceFunctions.createDevice(physicalDevice, &info, nullptr, &device), "Create Vulkan device");
}

bool AnyBox::Vulkan::Context::CanRequireComputeSubgroupSize(std::uint32_t size, std::uint64_t workgroupInvocations) const noexcept {
    return subgroupSizeControl && (subgroupSizeLimits.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT) != 0 &&
        std::has_single_bit(size) && size >= subgroupSizeLimits.minSubgroupSize && size <= subgroupSizeLimits.maxSubgroupSize &&
        workgroupInvocations <= static_cast<std::uint64_t>(size) * subgroupSizeLimits.maxComputeWorkgroupSubgroups;
}
