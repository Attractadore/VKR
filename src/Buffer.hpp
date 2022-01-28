#pragma once
#include <glm/vec3.hpp>

#include <vk_mem_alloc.h>

#include <span>

namespace VKR {
struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    void create(
        VmaAllocator allocator,
        size_t size,
        VkBufferUsageFlags buffer_usage,
        VmaAllocationCreateFlags alloc_flags,
        VmaMemoryUsage alloc_usage
    );

    void destroy(VmaAllocator allocator) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
};

inline auto createBuffer(
    VmaAllocator allocator,
    size_t size,
    VkBufferUsageFlags buffer_usage,
    VmaAllocationCreateFlags alloc_flags,
    VmaMemoryUsage alloc_usage
) {
    Buffer b;
    b.create(allocator, size, buffer_usage, alloc_flags, alloc_usage);
    return b;
}

inline auto createStagingBuffer(
    VmaAllocator allocator,
    size_t size
) {
    return createBuffer(
        allocator,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );
}

inline auto createStaticBuffer(
    VmaAllocator allocator,
    size_t size
) {
    return createBuffer(
        allocator,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        0,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
}

inline auto createDynamicBuffer(
    VmaAllocator allocator,
    size_t size
) {
    return createBuffer(
        allocator,
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
}

void copyToDynamicBuffer(
    VmaAllocator allocator,
    std::span<const glm::vec3> vertices,
    VmaAllocation allocation,
    size_t current_frame, size_t frame_count 
);

inline void copyToStagingBuffer(
    VmaAllocator allocator,
    std::span<const glm::vec3> vertices,
    VmaAllocation allocation
) {
    // TODO: this a bit inefficient 
    copyToDynamicBuffer(allocator, vertices, allocation, 0, 1);
}

void copyBuffer(
    VkDevice device,
    VkQueue queue,
    VkBuffer from, VkBuffer to,
    VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset,
    VkCommandPool cmd_pool
);
}
