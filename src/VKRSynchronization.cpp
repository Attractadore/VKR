#include "VKRSynchronization.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

VkResult VKR_CreateSemaphores(VkSemaphore* semaphores, unsigned num_semaphores) {
    VkResult res = VK_SUCCESS;
    for (unsigned i = 0; i < num_semaphores; i++) {
        VkSemaphoreCreateInfo create_info = {
            .sType = STYPE(create_info),
        };
        res = vkCreateSemaphore(vkr.device, &create_info, NULL, &semaphores[i]);
        if (res < 0) {
            VKR_DestroySemaphores(semaphores, i);
            return res;
        }
    }
    return VK_SUCCESS;
}

VkResult VKR_CreateFences(VkFence* fences, unsigned num_fences, bool signaled) {
    VkResult res = VK_SUCCESS;
    for (unsigned i = 0; i < num_fences; i++) {
        VkFenceCreateInfo create_info = {
            .sType = STYPE(create_info),
            .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlagBits{},
        };
        res = vkCreateFence(vkr.device, &create_info, NULL, &fences[i]);
        if (res < 0) {
            VKR_DestroyFences(fences, i);
            return res;
        }
    }
    return VK_SUCCESS;
}

void VKR_DestroySemaphores(VkSemaphore* semaphores, unsigned num_semaphores) {
    for (unsigned i = 0; i < num_semaphores; i++) {
        vkDestroySemaphore(vkr.device, semaphores[i], NULL);
    }
}

void VKR_DestroyFences(VkFence* fences, unsigned num_fences) {
    for (unsigned i = 0; i < num_fences; i++) {
        vkDestroyFence(vkr.device, fences[i], NULL);
    }
}

VkResult VKR_CreateSynchronizationPrimitives() {
    VkResult res = VK_SUCCESS;

    res = VKR_CreateSemaphores(vkr.acquire_semaphores, vkr.num_swapchain_images);
    if (res < 0) { goto fail; }
    res = VKR_CreateSemaphores(vkr.draw_semaphores, vkr.num_swapchain_images);
    if (res < 0) { goto fail; }
    res = VKR_CreateFences(vkr.frame_fences, vkr.num_swapchain_images);
    if (res < 0) { goto fail; }

    return VK_SUCCESS;
    
fail:
    VKR_DestroySynchronizationPrimitives();

    return res;
}

void VKR_DestroySynchronizationPrimitives() {
    VKR_DestroySemaphores(vkr.acquire_semaphores, vkr.num_swapchain_images);
    VKR_DestroySemaphores(vkr.draw_semaphores, vkr.num_swapchain_images);
    VKR_DestroyFences(vkr.frame_fences, vkr.num_swapchain_images);
}
