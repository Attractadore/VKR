#include "VKRMemory.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

#include <string.h>

static int getMemoryTypeIndex(
    uint32_t supported_memory_types, VkMemoryPropertyFlags memory_flags
) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vkr.physical_device, &memory_properties);
    for (unsigned i = 0; i < memory_properties.memoryTypeCount; i++) {
        if (BIT_SET(supported_memory_types, i)) {
            if ((memory_properties.memoryTypes[i].propertyFlags & memory_flags) == memory_flags) {
                return i;
            }
        }
    }
    return -1;
}

static VkResult allocateMemory(
    const VkMemoryRequirements* requirements,
    VkMemoryPropertyFlags flags,
    VkDeviceMemory* memory
) {
    int memory_type_idx =
        getMemoryTypeIndex(requirements->memoryTypeBits, flags);

    VkMemoryAllocateInfo memory_allocate_info = {
        .sType = STYPE(memory_allocate_info),
        .allocationSize = requirements->size,
        .memoryTypeIndex = memory_type_idx,
    };
    return vkAllocateMemory(vkr.device, &memory_allocate_info, NULL, memory);
}

static VkResult createBuffer(
    unsigned size, VkBufferUsageFlags buffer_flags,
    VkMemoryPropertyFlags memory_flags,
    VKRBuffer* buffer
) {
    size = std::max(size, 1u);
    VkResult res = VK_SUCCESS;
    *buffer = (VKRBuffer) {
        .buf = VK_NULL_HANDLE,
        .mem = VK_NULL_HANDLE,
    };

    VkBufferCreateInfo buffer_create_info = {
        .sType = STYPE(buffer_create_info),
        .size = size,
        .usage = buffer_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    res = vkCreateBuffer(vkr.device, &buffer_create_info, NULL, &buffer->buf);
    if (res < 0) { goto fail; }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vkr.device, buffer->buf, &memory_requirements);

    res = allocateMemory(&memory_requirements, memory_flags, &buffer->mem);
    if (res < 0) { goto fail; }

    res = vkBindBufferMemory(vkr.device, buffer->buf, buffer->mem, 0);
    if (res < 0) { goto fail; }

    buffer->size = size;

    return VK_SUCCESS;

fail:
    VKR_FreeBuffer(buffer);

    return res;
}

VkResult VKR_CreateHostBuffer(
    unsigned size, VKRBuffer* buffer
) {
    return createBuffer(
        size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer 
    );
}

VkResult VKR_CreateDeviceBuffer(
    const void* data, unsigned size, VKRBuffer* buffer, VkFence fence
) {
    VkResult res = VK_SUCCESS;

    VKRBuffer staging_buffer;
    res = createBuffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer
    );
    if (res < 0) { return res; }

    res = createBuffer(
        size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer
    );
    if (res < 0) { goto fail; }

    void* mapped_data;
    res = vkMapMemory(vkr.device, staging_buffer.mem, 0, VK_WHOLE_SIZE, 0, &mapped_data);
    if (res < 0) { goto fail; }
    memcpy(mapped_data, data, size);
    vkUnmapMemory(vkr.device, staging_buffer.mem);

    res = VKR_CopyBuffer(staging_buffer.buf, buffer->buf, buffer->size, 0, 0, fence);

    VKR_FreeBuffer(&staging_buffer);

    return VK_SUCCESS;

fail:
    VKR_FreeBuffer(buffer);
    VKR_FreeBuffer(&staging_buffer);

    return res;
}

void VKR_FreeBuffer(VKRBuffer* buffer) {
    vkDestroyBuffer(vkr.device, buffer->buf, NULL);
    vkFreeMemory(vkr.device, buffer->mem, NULL);
}

VkResult VKR_CopyBuffer(VkBuffer src, VkBuffer dst, unsigned size, unsigned src_offset, unsigned dst_offset, VkFence fence) {
    VkResult res = VK_SUCCESS;

    VkCommandBuffer copy_cmd;
    {
        VkCommandBufferAllocateInfo allocate_info = {
            .sType = STYPE(allocate_info),
            .commandPool = vkr.copy_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        res = vkAllocateCommandBuffers(vkr.device, &allocate_info, &copy_cmd);
        if (res < 0) { goto fail; }
    }

    {
        VkCommandBufferBeginInfo begin_info = {
            .sType = STYPE(begin_info),
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        res = vkBeginCommandBuffer(copy_cmd, &begin_info);
        if (res < 0) { goto fail; }
    }

    {
        VkBufferCopy copy = {
            .srcOffset = src_offset,
            .dstOffset = dst_offset,
            .size = size,
        };
        vkCmdCopyBuffer(copy_cmd, src, dst, 1, &copy);
    }

    res = vkEndCommandBuffer(copy_cmd);
    if (res < 0) { goto fail; }

    {
        VkSubmitInfo submit_info = {
            .sType = STYPE(submit_info),
            .commandBufferCount = 1,
            .pCommandBuffers = &copy_cmd,
        };
        res = vkQueueSubmit(vkr.queues.graphics, 1, &submit_info, fence);
        if (res < 0) { goto fail; }
        if (!fence) {
            res = vkQueueWaitIdle(vkr.queues.graphics);
            if (res < 0) { goto fail; }
        }
    }

    vkFreeCommandBuffers(vkr.device, vkr.copy_pool, 1, &copy_cmd);

    return VK_SUCCESS;

fail:
    vkFreeCommandBuffers(vkr.device, vkr.copy_pool, 1, &copy_cmd);

    return res;
}

VkResult VKR_CreateImage(VkExtent2D extent, VkFormat format, VKRImage* image) {
    VkResult res = VK_SUCCESS;
    *image = (VKRImage) {
        .img = VK_NULL_HANDLE,
        .mem = VK_NULL_HANDLE,
    };

    VkImageCreateInfo create_info = {
        .sType = STYPE(create_info),
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    res = vkCreateImage(vkr.device, &create_info, NULL, &image->img);
    if (res < 0) { goto fail; }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vkr.device, image->img, &memory_requirements);
    
    res = allocateMemory(
        &memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &image->mem
    );
    if (res < 0) { goto fail; }

    res = vkBindImageMemory(vkr.device, image->img, image->mem, 0);
    if (res < 0) { goto fail; }

    return VK_SUCCESS;

fail:
    VKR_FreeImage(image);

    return res;
}

void VKR_FreeImage(VKRImage* image) {
    vkDestroyImage(vkr.device, image->img, NULL);
    vkFreeMemory(vkr.device, image->mem, NULL);
}
