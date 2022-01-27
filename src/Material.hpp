#pragma once
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#include <span>

namespace VKR {
struct Material {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    void create(
        VkDevice device,
        std::span<const std::byte> vert_shader_binary,
        std::span<const std::byte> frag_shader_binary,
        VkRenderPass render_pass
    );

    void destroy(VkDevice device) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
    }

    void bind(VkCommandBuffer cmd_buffer) {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void setMVP(VkCommandBuffer cmd_buffer, const glm::mat4& mvp) {
        vkCmdPushConstants(
            cmd_buffer, layout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(glm::mat4), &mvp
        );
    }
};
}
