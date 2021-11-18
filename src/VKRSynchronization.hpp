#pragma once
#include <vulkan/vulkan.h>

VkResult VKR_CreateSemaphores(VkSemaphore* semaphores, unsigned num_semaphores);
void VKR_DestroySemaphores(VkSemaphore* semaphores, unsigned num_semaphores);

VkResult VKR_CreateFences(VkFence* fences, unsigned num_fences, bool signaled = true);
void VKR_DestroyFences(VkFence* fences, unsigned num_fences);

VkResult VKR_CreateSynchronizationPrimitives();
void VKR_DestroySynchronizationPrimitives();
