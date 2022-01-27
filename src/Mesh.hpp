#pragma once
#include "Buffer.hpp"

namespace VKR {
struct StaticMesh {
    Buffer buffer;
    uint32_t vertex_count = 0;

    void create(
        VkDevice device, VmaAllocator allocator,
        VkQueue graphics_queue, VkCommandPool cmd_pool,
        std::span<const glm::vec3> vertices
    );

    void destroy(VmaAllocator allocator) {
        buffer.destroy(allocator);
    }

    void bind(VkCommandBuffer cmd_buffer) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer.buffer, &offset);
    }

    void draw(VkCommandBuffer cmd_buffer) {
        vkCmdDraw(cmd_buffer, vertex_count, 1, 0, 0);
    }
};

struct DynamicMesh {
    Buffer buffer;
    static constexpr uint32_t frame_count = 2;
    std::array<VkFence, frame_count> fences = {
        VK_NULL_HANDLE, VK_NULL_HANDLE,
    };
    uint32_t vertex_reserved_count = 0;
    uint32_t vertex_count = 0;
    uint8_t current_frame = 0;

    void create(
        VkDevice device, VmaAllocator allocator,
        VkQueue graphics_queue,
        size_t vertex_count
    );

    void destroy(VmaAllocator allocator) {
        buffer.destroy(allocator);
    }

    void bind(VkCommandBuffer cmd_buffer) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer.buffer, &offset);
    }

    void draw(VkCommandBuffer cmd_buffer) {
        vkCmdDraw(cmd_buffer, vertex_count, 1, vertex_reserved_count * current_frame, 0);
    }

    void setVertexData(
        VkDevice device, VmaAllocator allocator,
        std::span<const glm::vec3> vertices
    );

    void advanceFrame() {
        current_frame = (current_frame + 1) % frame_count;
    }

    void waitForFrameFence(VkDevice device) {
        auto& current_fence = fences[current_frame];
        if (current_fence) {
            vkWaitForFences(device, 1, &current_fence, true, UINT64_MAX);
            current_fence = VK_NULL_HANDLE;
        }
    }

    void updateFrameFrence(VkFence new_fence) {
        fences[current_frame] = new_fence;
    }
};
};
