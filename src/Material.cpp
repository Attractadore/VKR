#include "Material.hpp"

namespace VKR {
namespace {
VkPipelineLayout createMaterialPipelineLayout(
    VkDevice device
) {
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(glm::mat4),
    };
    VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };
    VkPipelineLayout layout;
    vkCreatePipelineLayout(device, &create_info, nullptr, &layout);
    return layout;
}

VkShaderModule createShaderModule(
    VkDevice device,
    std::span<const std::byte> shader_binary
) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_binary.size_bytes(),
        .pCode = reinterpret_cast<const uint32_t*>(
            shader_binary.data()
        ),
    };

    VkShaderModule shader_module;
    vkCreateShaderModule(device, &create_info, nullptr, &shader_module);

    return shader_module;
}

VkPipeline createMaterialPipeline(
    VkDevice device,
    std::span<const std::byte> vert_shader_binary,
    std::span<const std::byte> frag_shader_binary,
    VkPipelineLayout layout,
    VkRenderPass render_pass
) {
    auto vert_shader_module = createShaderModule(device, vert_shader_binary);
    auto frag_shader_module = createShaderModule(device, frag_shader_binary);

    auto getShaderStageCreateInfo =
    [](VkShaderStageFlagBits stage, VkShaderModule shader_module) {
        return VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage,
            .module = shader_module,
            .pName = "main",
        };
    };
    
    std::array stages = {
        getShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vert_shader_module),
        getShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_module),
    };

    VkVertexInputBindingDescription binding_desc = {
        .binding = 0,
        .stride = sizeof(glm::vec3),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attribute_desc = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attribute_desc,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,

    };

    VkPipelineViewportStateCreateInfo viewport = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    std::array dynamic_state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_state.size(),
        .pDynamicStates = dynamic_state.data(),
    };

    VkGraphicsPipelineCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = stages.size(),
        .pStages = stages.data(),
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blend,
        .pDynamicState = &dynamic,
        .layout = layout,
        .renderPass = render_pass,
        .subpass = 0,
    };
    
    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, nullptr, 1, &create_info, nullptr, &pipeline);

    vkDestroyShaderModule(device, vert_shader_module, nullptr);
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    
    return pipeline;
}
}

void Material::create(
    VkDevice device,
    std::span<const std::byte> vert_shader_binary,
    std::span<const std::byte> frag_shader_binary,
    VkRenderPass render_pass
) {
    layout = createMaterialPipelineLayout(device);
    pipeline = createMaterialPipeline(
        device,
        vert_shader_binary,
        frag_shader_binary,
        layout,
        render_pass
    );
}
}
