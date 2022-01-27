#include "Buffer.hpp"

#include <algorithm>

namespace VKR {
void Buffer::create(
    VmaAllocator allocator,
    size_t size,
    VkBufferUsageFlags buffer_usage,
    VmaAllocationCreateFlags alloc_flags,
    VmaMemoryUsage alloc_usage
) {
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = buffer_usage,
    };

    VmaAllocationCreateInfo alloc_create_info = {
        .flags = alloc_flags,
        .usage = alloc_usage,
    };

    vmaCreateBuffer(
        allocator,
        &create_info, &alloc_create_info,
        &buffer, &allocation,
        nullptr
    );
}

void copyToDynamicBuffer(
    VmaAllocator allocator,
    std::span<const glm::vec3> vertices,
    VmaAllocation allocation,
    size_t current_frame, size_t frame_count 
) {
    VmaAllocationInfo alloc_info;
    vmaGetAllocationInfo(allocator, allocation, &alloc_info);
    auto offset_bytes = alloc_info.size / frame_count * current_frame;
    auto offset = offset_bytes / sizeof(glm::vec3);
    auto mapped_data = static_cast<glm::vec3*>(
        alloc_info.pMappedData
    ) + offset;
    std::ranges::copy(vertices, mapped_data);
    vmaFlushAllocation(allocator, allocation, offset_bytes, vertices.size_bytes());
}

void copyBuffer(
    VkDevice device,
    VkQueue queue,
    VkBuffer from, VkBuffer to,
    VkCommandPool cmd_pool
) {
    VkCommandBufferAllocateInfo send_cmd_buffer_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer send_cmd_buffer;
    vkAllocateCommandBuffers(device, &send_cmd_buffer_alloc_info, &send_cmd_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(send_cmd_buffer, &begin_info);

    VkBufferCopy copy_info = {
        .size = VK_WHOLE_SIZE,
    };
    vkCmdCopyBuffer(send_cmd_buffer, from, to, 1, &copy_info);

    vkEndCommandBuffer(send_cmd_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &send_cmd_buffer,
    };
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

    // TODO: uploads should be asynchronous
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, cmd_pool, 1, &send_cmd_buffer);
}
}
