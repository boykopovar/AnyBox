#include <anybox/vulkan/Context.hpp>

void AnyBox::Vulkan::Context::loadInstanceFunctions() {
    instanceFunctions.destroyInstance = RequireFunction<PFN_vkDestroyInstance>(getInstanceProcAddr(instance, "vkDestroyInstance"), "vkDestroyInstance");
    instanceFunctions.enumeratePhysicalDevices = RequireFunction<PFN_vkEnumeratePhysicalDevices>(getInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"), "vkEnumeratePhysicalDevices");
    instanceFunctions.getPhysicalDeviceProperties = RequireFunction<PFN_vkGetPhysicalDeviceProperties2>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2"), "vkGetPhysicalDeviceProperties2");
    instanceFunctions.getMemoryProperties = RequireFunction<PFN_vkGetPhysicalDeviceMemoryProperties>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"), "vkGetPhysicalDeviceMemoryProperties");
    instanceFunctions.getQueueFamilies = RequireFunction<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"), "vkGetPhysicalDeviceQueueFamilyProperties");
    instanceFunctions.getFeatures = RequireFunction<PFN_vkGetPhysicalDeviceFeatures2>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2"), "vkGetPhysicalDeviceFeatures2");
    instanceFunctions.enumerateExtensions = RequireFunction<PFN_vkEnumerateDeviceExtensionProperties>(getInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"), "vkEnumerateDeviceExtensionProperties");
    instanceFunctions.createDevice = RequireFunction<PFN_vkCreateDevice>(getInstanceProcAddr(instance, "vkCreateDevice"), "vkCreateDevice");
    instanceFunctions.getDeviceProcAddr = RequireFunction<PFN_vkGetDeviceProcAddr>(getInstanceProcAddr(instance, "vkGetDeviceProcAddr"), "vkGetDeviceProcAddr");
    instanceFunctions.getExternalBufferProperties = RequireFunction<PFN_vkGetPhysicalDeviceExternalBufferProperties>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalBufferProperties"), "vkGetPhysicalDeviceExternalBufferProperties");
    instanceFunctions.getExternalSemaphoreProperties = RequireFunction<PFN_vkGetPhysicalDeviceExternalSemaphoreProperties>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalSemaphoreProperties"), "vkGetPhysicalDeviceExternalSemaphoreProperties");
    instanceFunctions.getImageFormatProperties = RequireFunction<PFN_vkGetPhysicalDeviceImageFormatProperties2>(getInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2"), "vkGetPhysicalDeviceImageFormatProperties2");
}

void AnyBox::Vulkan::Context::loadDeviceFunctions() {
#define ANYBOX_VULKAN_FUNCTION(field, name) deviceFunctions.field = RequireFunction<PFN_##name>(instanceFunctions.getDeviceProcAddr(device, #name), #name)
    ANYBOX_VULKAN_FUNCTION(destroyDevice, vkDestroyDevice);
    ANYBOX_VULKAN_FUNCTION(getDeviceQueue, vkGetDeviceQueue);
    ANYBOX_VULKAN_FUNCTION(createBuffer, vkCreateBuffer);
    ANYBOX_VULKAN_FUNCTION(destroyBuffer, vkDestroyBuffer);
    ANYBOX_VULKAN_FUNCTION(getBufferRequirements, vkGetBufferMemoryRequirements2);
    ANYBOX_VULKAN_FUNCTION(allocateMemory, vkAllocateMemory);
    ANYBOX_VULKAN_FUNCTION(freeMemory, vkFreeMemory);
    ANYBOX_VULKAN_FUNCTION(bindBufferMemory, vkBindBufferMemory);
    ANYBOX_VULKAN_FUNCTION(getHostPointerProperties, vkGetMemoryHostPointerPropertiesEXT);
    ANYBOX_VULKAN_FUNCTION(getBufferAddress, vkGetBufferDeviceAddress);
    ANYBOX_VULKAN_FUNCTION(mapMemory, vkMapMemory);
    ANYBOX_VULKAN_FUNCTION(unmapMemory, vkUnmapMemory);
    ANYBOX_VULKAN_FUNCTION(createCommandPool, vkCreateCommandPool);
    ANYBOX_VULKAN_FUNCTION(destroyCommandPool, vkDestroyCommandPool);
    ANYBOX_VULKAN_FUNCTION(resetCommandPool, vkResetCommandPool);
    ANYBOX_VULKAN_FUNCTION(allocateCommandBuffers, vkAllocateCommandBuffers);
    ANYBOX_VULKAN_FUNCTION(beginCommandBuffer, vkBeginCommandBuffer);
    ANYBOX_VULKAN_FUNCTION(endCommandBuffer, vkEndCommandBuffer);
    ANYBOX_VULKAN_FUNCTION(pipelineBarrier, vkCmdPipelineBarrier);
    ANYBOX_VULKAN_FUNCTION(copyBuffer, vkCmdCopyBuffer);
    ANYBOX_VULKAN_FUNCTION(fillBuffer, vkCmdFillBuffer);
    ANYBOX_VULKAN_FUNCTION(createFence, vkCreateFence);
    ANYBOX_VULKAN_FUNCTION(destroyFence, vkDestroyFence);
    ANYBOX_VULKAN_FUNCTION(queueSubmit, vkQueueSubmit);
    ANYBOX_VULKAN_FUNCTION(waitForFences, vkWaitForFences);
    ANYBOX_VULKAN_FUNCTION(queueWaitIdle, vkQueueWaitIdle);
    ANYBOX_VULKAN_FUNCTION(createShaderModule, vkCreateShaderModule);
    ANYBOX_VULKAN_FUNCTION(destroyShaderModule, vkDestroyShaderModule);
    ANYBOX_VULKAN_FUNCTION(createPipelineLayout, vkCreatePipelineLayout);
    ANYBOX_VULKAN_FUNCTION(destroyPipelineLayout, vkDestroyPipelineLayout);
    ANYBOX_VULKAN_FUNCTION(createComputePipelines, vkCreateComputePipelines);
    ANYBOX_VULKAN_FUNCTION(createGraphicsPipelines, vkCreateGraphicsPipelines);
    ANYBOX_VULKAN_FUNCTION(createRenderPass, vkCreateRenderPass);
    ANYBOX_VULKAN_FUNCTION(destroyRenderPass, vkDestroyRenderPass);
    ANYBOX_VULKAN_FUNCTION(createFramebuffer, vkCreateFramebuffer);
    ANYBOX_VULKAN_FUNCTION(destroyFramebuffer, vkDestroyFramebuffer);
    ANYBOX_VULKAN_FUNCTION(beginRenderPass, vkCmdBeginRenderPass);
    ANYBOX_VULKAN_FUNCTION(endRenderPass, vkCmdEndRenderPass);
    ANYBOX_VULKAN_FUNCTION(draw, vkCmdDraw);
    ANYBOX_VULKAN_FUNCTION(destroyPipeline, vkDestroyPipeline);
    ANYBOX_VULKAN_FUNCTION(bindPipeline, vkCmdBindPipeline);
    ANYBOX_VULKAN_FUNCTION(pushConstants, vkCmdPushConstants);
    ANYBOX_VULKAN_FUNCTION(dispatch, vkCmdDispatch);
    ANYBOX_VULKAN_FUNCTION(createSemaphore, vkCreateSemaphore);
    ANYBOX_VULKAN_FUNCTION(destroySemaphore, vkDestroySemaphore);
    ANYBOX_VULKAN_FUNCTION(importSemaphore, vkImportSemaphoreWin32HandleKHR);
    ANYBOX_VULKAN_FUNCTION(getSemaphoreValue, vkGetSemaphoreCounterValue);
    ANYBOX_VULKAN_FUNCTION(waitSemaphores, vkWaitSemaphores);
    ANYBOX_VULKAN_FUNCTION(createImage, vkCreateImage);
    ANYBOX_VULKAN_FUNCTION(destroyImage, vkDestroyImage);
    ANYBOX_VULKAN_FUNCTION(getImageRequirements, vkGetImageMemoryRequirements2);
    ANYBOX_VULKAN_FUNCTION(bindImageMemory, vkBindImageMemory);
    ANYBOX_VULKAN_FUNCTION(clearColorImage, vkCmdClearColorImage);
    ANYBOX_VULKAN_FUNCTION(copyImageToBuffer, vkCmdCopyImageToBuffer);
    ANYBOX_VULKAN_FUNCTION(createImageView, vkCreateImageView);
    ANYBOX_VULKAN_FUNCTION(destroyImageView, vkDestroyImageView);
    ANYBOX_VULKAN_FUNCTION(createDescriptorSetLayout, vkCreateDescriptorSetLayout);
    ANYBOX_VULKAN_FUNCTION(destroyDescriptorSetLayout, vkDestroyDescriptorSetLayout);
    ANYBOX_VULKAN_FUNCTION(createDescriptorPool, vkCreateDescriptorPool);
    ANYBOX_VULKAN_FUNCTION(destroyDescriptorPool, vkDestroyDescriptorPool);
    ANYBOX_VULKAN_FUNCTION(allocateDescriptorSets, vkAllocateDescriptorSets);
    ANYBOX_VULKAN_FUNCTION(updateDescriptorSets, vkUpdateDescriptorSets);
    ANYBOX_VULKAN_FUNCTION(bindDescriptorSets, vkCmdBindDescriptorSets);
#undef ANYBOX_VULKAN_FUNCTION
}
