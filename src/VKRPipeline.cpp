#include "VKRPipeline.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

static VkResult createShaderModule(VKR_GetShaderBinaryCallbackT cb, VkShaderModule* module) {
    unsigned binary_size;
    const char* binary = cb(&binary_size);
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = binary_size,
        .pCode = (const uint32_t*) binary,
    };
    return vkCreateShaderModule(vkr.device, &create_info, NULL, module);
}

static VkResult createPipelineLayout() {
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(glm::mat4[2]),
    };
    VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    };
    return vkCreatePipelineLayout(vkr.device, &create_info, NULL, &vkr.pipeline_layout);
};

VkResult VKR_CreatePipeline(
    VKR_GetShaderBinaryCallbackT vertex_cb,
    VKR_GetShaderBinaryCallbackT fragment_cb
) {
    VkResult res = VK_SUCCESS;

    VkShaderModule shader_modules[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    auto fail = [&]() {
        VKR_DestroyPipeline();
        for (unsigned i = 0; i < ARRAY_SIZE(shader_modules); i++) {
            vkDestroyShaderModule(vkr.device, shader_modules[i], NULL);
        }
    
        return res;
    };

    res = createShaderModule(vertex_cb, &shader_modules[0]);
    if (res < 0) { return fail(); }
    res = createShaderModule(fragment_cb, &shader_modules[1]);
    if (res < 0) { return fail(); }

    VkShaderStageFlagBits shader_stages[] =
        {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkPipelineShaderStageCreateInfo shader_create_infos[2];
    for (unsigned i = 0; i < 2; i++) {
        shader_create_infos[i] = (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shader_stages[i],
            .module = shader_modules[i],
            .pName = "main",
        };
    }

    VkVertexInputBindingDescription vertex_binding_description = {
        .binding = 0,
        .stride = sizeof(VKRVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VKRVertex, color),
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = ARRAY_SIZE(vertex_attribute_descriptions),
        .pVertexAttributeDescriptions= vertex_attribute_descriptions,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkViewport vp = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(vkr.swapchain_extent.width),
        .height = static_cast<float>(vkr.swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D sc = {
        .offset = {.x = 0, .y = 0},
        .extent = vkr.swapchain_extent,
    };

    VkPipelineViewportStateCreateInfo viewport = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &vp,
        .scissorCount = 1,
        .pScissors = &sc
    };

    VkPipelineRasterizationStateCreateInfo rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = STYPE(depth_stencil),
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    res = createPipelineLayout();
    if (res < 0) { return fail(); }

    VkGraphicsPipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = ARRAY_SIZE(shader_create_infos),
        .pStages = shader_create_infos,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blend,
        .layout = vkr.pipeline_layout,
        .renderPass = vkr.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    res = vkCreateGraphicsPipelines(vkr.device, VK_NULL_HANDLE, 1, &create_info, NULL, &vkr.pipeline);
    if (res < 0) { return fail(); }

    for (unsigned i = 0; i < ARRAY_SIZE(shader_modules); i++) {
        vkDestroyShaderModule(vkr.device, shader_modules[i], NULL);
    }

    return VK_SUCCESS;
}

void VKR_DestroyPipeline() {
    vkDestroyPipelineLayout(vkr.device, vkr.pipeline_layout, NULL);
    vkDestroyPipeline(vkr.device, vkr.pipeline, NULL);
}
