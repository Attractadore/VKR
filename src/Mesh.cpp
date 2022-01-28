#include "Mesh.hpp"

namespace VKR {
void StaticMesh::create(
    VkDevice device, VmaAllocator allocator,
    VkQueue graphics_queue, VkCommandPool cmd_pool,
    std::span<const glm::vec3> vertices
) {
    auto staging_buffer = createStagingBuffer(allocator, vertices.size_bytes());
    copyToStagingBuffer(allocator, vertices, staging_buffer.allocation);
    buffer = createStaticBuffer(allocator, vertices.size_bytes());
    copyBuffer(
        device, graphics_queue,
        staging_buffer.buffer, buffer.buffer,
        vertices.size_bytes(), 0, 0,
        cmd_pool
    );
    vertex_count = vertices.size();
    staging_buffer.destroy(allocator);
}

void DynamicMesh::create(
    VkDevice device, VmaAllocator allocator,
    VkQueue graphics_queue,
    size_t vertex_count
) {
    auto buffer_size = sizeof(glm::vec3[DynamicMesh::frame_count]) * vertex_count;
    buffer = createDynamicBuffer(allocator, buffer_size);
    vertex_reserved_count = vertex_count;
}

void DynamicMesh::setVertexData(
    VkDevice device, VmaAllocator allocator,
    std::span<const glm::vec3> vertices
) {
    advanceFrame();
    waitForFrameFence(device);
    copyToDynamicBuffer(
        allocator,
        vertices, buffer.allocation,
        current_frame, frame_count
    );
    vertex_count = vertices.size();
}
};
