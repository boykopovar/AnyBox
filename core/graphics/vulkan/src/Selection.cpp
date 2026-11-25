#include <anybox/vulkan/Context.hpp>
#include <algorithm>
#include <cstring>

void AnyBox::Vulkan::Context::selectAdapter(const AdapterSelection& selection, std::uint32_t queueCount) {
    findAdapter(selection);
    std::uint32_t count = 0;
    VkPhysicalDeviceExternalMemoryHostPropertiesEXT host{};
    host.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
    VkPhysicalDeviceIDProperties identity{};
    identity.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    subgroups.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    floatControls.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
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
    if (properties.apiVersion < VK_API_VERSION_1_2) throw AdapterIncompatible("Vulkan adapter does not support API version 1.2");
    hostPointerAlignment = host.minImportedHostPointerAlignment;
    instanceFunctions.getMemoryProperties(physicalDevice, &memoryProperties);
    instanceFunctions.getQueueFamilies(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    instanceFunctions.getQueueFamilies(physicalDevice, &count, families.data());
    constexpr auto required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    auto family = std::ranges::find_if(families, [&](const auto& value) { return (value.queueFlags & required) == required && value.queueCount >= queueCount; });
    if (family == families.end()) throw AdapterIncompatible("Vulkan adapter has no universal queue family with the requested queue count");
    queueFamily = static_cast<std::uint32_t>(family - families.begin());
    Check(instanceFunctions.enumerateExtensions(physicalDevice, nullptr, &count, nullptr), "Count Vulkan device extensions");
    std::vector<VkExtensionProperties> extensions(count);
    Check(instanceFunctions.enumerateExtensions(physicalDevice, nullptr, &count, extensions.data()), "Enumerate Vulkan device extensions");
    if (std::ranges::none_of(extensions, [](const auto& value) { return std::strcmp(value.extensionName, VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME) == 0; })) throw AdapterIncompatible("Vulkan host memory imports are unsupported");
    if (hostPointerAlignment == 0) throw std::runtime_error("Vulkan host pointer alignment is zero");
    if (std::ranges::none_of(extensions, [](const auto& value) { return std::strcmp(value.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME) == 0; })) throw AdapterIncompatible("Vulkan Windows semaphore imports are unsupported");
}

void AnyBox::Vulkan::Context::createDevice(std::uint32_t queueCount) {
    VkPhysicalDeviceBufferDeviceAddressFeatures address{};
    address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline{};
    timeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    VkPhysicalDevice8BitStorageFeatures storage{};
    storage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    timeline.pNext = &storage;
    address.pNext = &timeline;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &address;
    instanceFunctions.getFeatures(physicalDevice, &features);
    if (!address.bufferDeviceAddress) throw AdapterIncompatible("Vulkan buffer device addresses are unsupported");
    if (!features.features.shaderInt64) throw AdapterIncompatible("Vulkan shader 64-bit integers are unsupported");
    if (!timeline.timelineSemaphore) throw AdapterIncompatible("Vulkan timeline semaphores are unsupported");
    auto& enabled = enabledFeatures;
    enabled.shaderInt64 = VK_TRUE;
    enabled.textureCompressionBC = features.features.textureCompressionBC;
    enabled.textureCompressionETC2 = features.features.textureCompressionETC2;
    enabled.textureCompressionASTC_LDR = features.features.textureCompressionASTC_LDR;
    enabled.imageCubeArray = features.features.imageCubeArray;
    enabled.shaderClipDistance = features.features.shaderClipDistance;
    enabled.shaderCullDistance = features.features.shaderCullDistance;
    enabled.vertexPipelineStoresAndAtomics = features.features.vertexPipelineStoresAndAtomics;
    enabled.fragmentStoresAndAtomics = features.features.fragmentStoresAndAtomics;
    cubeArrays = enabled.imageCubeArray != VK_FALSE;
    byteStorage = storage.storageBuffer8BitAccess != VK_FALSE;
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
    const char* extensions[]{VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME};
    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext = &address;
    info.pEnabledFeatures = &enabled;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = 2;
    info.ppEnabledExtensionNames = extensions;
    Check(instanceFunctions.createDevice(physicalDevice, &info, nullptr, &device), "Create Vulkan device");
}
