#pragma once

#include <anybox/platform/Windows.hpp>
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

namespace AnyBox::Vulkan {
inline void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) throw std::runtime_error(std::string(operation) + " failed with Vulkan result " + std::to_string(result));
}

struct InstanceFunctions {
    PFN_vkDestroyInstance destroyInstance;
    PFN_vkEnumeratePhysicalDevices enumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties2 getPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties getMemoryProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties getQueueFamilies;
    PFN_vkGetPhysicalDeviceFeatures2 getFeatures;
    PFN_vkEnumerateDeviceExtensionProperties enumerateExtensions;
    PFN_vkCreateDevice createDevice;
    PFN_vkGetDeviceProcAddr getDeviceProcAddr;
    PFN_vkGetPhysicalDeviceExternalBufferProperties getExternalBufferProperties;
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties getExternalSemaphoreProperties;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 getImageFormatProperties;
};

struct DeviceFunctions {
    PFN_vkDestroyDevice destroyDevice;
    PFN_vkGetDeviceQueue getDeviceQueue;
    PFN_vkCreateBuffer createBuffer;
    PFN_vkDestroyBuffer destroyBuffer;
    PFN_vkGetBufferMemoryRequirements2 getBufferRequirements;
    PFN_vkAllocateMemory allocateMemory;
    PFN_vkFreeMemory freeMemory;
    PFN_vkBindBufferMemory bindBufferMemory;
    PFN_vkGetMemoryHostPointerPropertiesEXT getHostPointerProperties;
    PFN_vkGetBufferDeviceAddress getBufferAddress;
    PFN_vkMapMemory mapMemory;
    PFN_vkUnmapMemory unmapMemory;
    PFN_vkCreateCommandPool createCommandPool;
    PFN_vkDestroyCommandPool destroyCommandPool;
    PFN_vkResetCommandPool resetCommandPool;
    PFN_vkAllocateCommandBuffers allocateCommandBuffers;
    PFN_vkBeginCommandBuffer beginCommandBuffer;
    PFN_vkEndCommandBuffer endCommandBuffer;
    PFN_vkCmdPipelineBarrier pipelineBarrier;
    PFN_vkCmdCopyBuffer copyBuffer;
    PFN_vkCmdFillBuffer fillBuffer;
    PFN_vkCreateFence createFence;
    PFN_vkDestroyFence destroyFence;
    PFN_vkQueueSubmit queueSubmit;
    PFN_vkWaitForFences waitForFences;
    PFN_vkQueueWaitIdle queueWaitIdle;
    PFN_vkCreateShaderModule createShaderModule;
    PFN_vkDestroyShaderModule destroyShaderModule;
    PFN_vkCreatePipelineLayout createPipelineLayout;
    PFN_vkDestroyPipelineLayout destroyPipelineLayout;
    PFN_vkCreateComputePipelines createComputePipelines;
    PFN_vkCreateGraphicsPipelines createGraphicsPipelines;
    PFN_vkCreateRenderPass createRenderPass;
    PFN_vkDestroyRenderPass destroyRenderPass;
    PFN_vkCreateFramebuffer createFramebuffer;
    PFN_vkDestroyFramebuffer destroyFramebuffer;
    PFN_vkCmdBeginRenderPass beginRenderPass;
    PFN_vkCmdEndRenderPass endRenderPass;
    PFN_vkCmdDraw draw;
    PFN_vkDestroyPipeline destroyPipeline;
    PFN_vkCmdBindPipeline bindPipeline;
    PFN_vkCmdPushConstants pushConstants;
    PFN_vkCmdDispatch dispatch;
    PFN_vkCreateSemaphore createSemaphore;
    PFN_vkDestroySemaphore destroySemaphore;
    PFN_vkImportSemaphoreWin32HandleKHR importSemaphore;
    PFN_vkGetSemaphoreCounterValue getSemaphoreValue;
    PFN_vkWaitSemaphores waitSemaphores;
    PFN_vkCreateImage createImage;
    PFN_vkDestroyImage destroyImage;
    PFN_vkGetImageMemoryRequirements2 getImageRequirements;
    PFN_vkBindImageMemory bindImageMemory;
    PFN_vkCmdClearColorImage clearColorImage;
    PFN_vkCmdCopyImageToBuffer copyImageToBuffer;
    PFN_vkCreateImageView createImageView;
    PFN_vkDestroyImageView destroyImageView;
    PFN_vkCreateDescriptorSetLayout createDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout destroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool createDescriptorPool;
    PFN_vkDestroyDescriptorPool destroyDescriptorPool;
    PFN_vkAllocateDescriptorSets allocateDescriptorSets;
    PFN_vkUpdateDescriptorSets updateDescriptorSets;
    PFN_vkCmdBindDescriptorSets bindDescriptorSets;
};

template<class TFunction>
TFunction RequireFunction(PFN_vkVoidFunction function, const char* name) {
    if (!function) throw std::runtime_error(std::string("Required Vulkan function is unavailable: ") + name);
    return reinterpret_cast<TFunction>(function);
}
}
