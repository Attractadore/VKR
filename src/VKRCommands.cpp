#include "VKRCommands.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

VkResult VKR_CreateCommandPools() {
    VkResult res = VK_SUCCESS;

    {
        VkCommandPoolCreateInfo create_info = {
            .sType = STYPE(create_info),
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex =
                static_cast<uint32_t>(vkr.queue_families.graphics),
        };
        res = vkCreateCommandPool(vkr.device, &create_info, NULL, &vkr.command_pool);
        if (res < 0) { goto fail; }
    }

    {
        VkCommandPoolCreateInfo create_info = {
            .sType = STYPE(create_info),
            .queueFamilyIndex =
                static_cast<uint32_t>(vkr.queue_families.graphics),
        };
        res = vkCreateCommandPool(vkr.device, &create_info, NULL, &vkr.copy_pool);
        if (res < 0) { goto fail; }
    }

    return VK_SUCCESS;

fail:
    VKR_DestroyCommandPools();

    return res;
}

void VKR_DestroyCommandPools() {
    vkDestroyCommandPool(vkr.device, vkr.command_pool, NULL);
    vkDestroyCommandPool(vkr.device, vkr.copy_pool, NULL);
}

VkResult VKR_AllocateCommandBuffers() {
    VkCommandBufferAllocateInfo allocate_info = {
        .sType = STYPE(allocate_info),
        .commandPool = vkr.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vkr.num_swapchain_images,
    };
    return vkAllocateCommandBuffers(vkr.device, &allocate_info, vkr.command_buffers);
}

void VKR_FreeCommandBuffers() {
    vkFreeCommandBuffers(vkr.device, vkr.command_pool, vkr.num_swapchain_images, vkr.command_buffers);
}
