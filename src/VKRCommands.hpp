#pragma once
#include <vulkan/vulkan.h>

VkResult VKR_CreateCommandPools();
void VKR_DestroyCommandPools();

VkResult VKR_AllocateCommandBuffers();
void VKR_FreeCommandBuffers();
