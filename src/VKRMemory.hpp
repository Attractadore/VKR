#pragma once
#include "VKRTypesInternal.hpp"

VkResult VKR_CreateDeviceBuffer(const void* data, unsigned size, VKRBuffer* buffer, VkFence fence);
VkResult VKR_CreateHostBuffer(unsigned size, VKRBuffer* buffer);
void VKR_FreeBuffer(VKRBuffer* buffer);

VkResult VKR_CopyBuffer(VkBuffer src, VkBuffer dst, unsigned size, unsigned src_offset, unsigned dst_offset, VkFence fence);

VkResult VKR_CreateImage(VkExtent2D extent, VkFormat format, VKRImage* image);
void VKR_FreeImage(VKRImage* image);
