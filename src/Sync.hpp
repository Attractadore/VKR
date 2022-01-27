#pragma once
#include <vulkan/vulkan.h>

inline VkSemaphore createSemaphore(VkDevice dev) {
    VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore sem;
    vkCreateSemaphore(dev, &create_info, nullptr, &sem);
    return sem;
}

inline VkFence createFence(VkDevice dev, VkFenceCreateFlags flags) {
    VkFenceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    VkFence fence;
    vkCreateFence(dev, &create_info, nullptr, &fence);
    return fence;
}

inline VkFence createSignaledFence(VkDevice dev) {
    return createFence(dev, VK_FENCE_CREATE_SIGNALED_BIT);
}

inline VkFence createUnsignaledFence(VkDevice dev) {
    return createFence(dev, 0);
}
