#include "VKR.hpp"
#include "VKRInternal.hpp"
#include "VKRUtils.hpp"

VkResult VKR_Init(
    VKR_GetInstanceExtensionsCallbackT extensions_cb,
    VKR_CreateSurfaceCallbackT surface_cb,
    VKR_GetFramebufferSizeCallbackT framebuffer_cb,
    VKR_GetShaderBinaryCallbackT vertex_cb,
    VKR_GetShaderBinaryCallbackT fragment_cb
) {
    VkResult res = VK_SUCCESS;

    res = VKR_CreateInstance(extensions_cb);
    if (res < 0) { goto fail; }
    res = VKR_CreateSurface(surface_cb);
    if (res < 0) { goto fail; }
    res = VKR_CreateDevice();
    if (res < 0) { goto fail; }
    res = VKR_CreateCommandPools();
    if (res < 0) { goto fail; }
    res = VKR_CreateRenderPass();
    if (res < 0) { goto fail; }
    res = VKR_CreateSwapchain(framebuffer_cb);
    if (res < 0) { goto fail; }
    res = VKR_CreatePipeline(vertex_cb, fragment_cb);
    if (res < 0) { goto fail; }
    res = VKR_CreateSynchronizationPrimitives();
    if (res < 0) { goto fail; }
    res = VKR_AllocateCommandBuffers();
    if (res < 0) { goto fail; }

    return VK_SUCCESS;

fail:
    VKR_Quit();

    return res;
}

void VKR_Quit() {
    if (vkr.device) {
        vkDeviceWaitIdle(vkr.device);
    }
    VKR_FreeCommandBuffers();
    vkr.models.clear();
    vkr.static_meshes.clear();
    vkr.streaming_meshes.clear();
    VKR_DestroySynchronizationPrimitives();
    VKR_DestroyPipeline();
    VKR_DestroySwapchain();
    VKR_DestroyRenderPass();
    VKR_DestroyCommandPools();
    VKR_DestroyDevice();
    VKR_DestroySurface();
    VKR_DestroyInstance();
}

VkResult VKR_Draw() {
    VkResult res = VK_SUCCESS;

    VkSemaphore acquire_semaphore = vkr.acquire_semaphores[vkr.frame];
    VkSemaphore draw_semaphore = vkr.draw_semaphores[vkr.frame];
    VkFence frame_fence = vkr.frame_fences[vkr.frame];
    VkCommandBuffer cmd_buffer = vkr.command_buffers[vkr.frame];

    unsigned image;
    res = vkAcquireNextImageKHR(
        vkr.device, vkr.swapchain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, &image
    );
    if (res < 0) { return res; }

    res = vkWaitForFences(vkr.device, 1, &frame_fence, true, UINT64_MAX);
    if (res < 0) { return res; }
    res = vkResetFences(vkr.device, 1, &frame_fence);
    if (res < 0) { return res; }

    {
        VkCommandBufferBeginInfo begin_info = {
            .sType = STYPE(begin_info),
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd_buffer, &begin_info);
    }
    {
        VkClearValue clear_color = {
            .color = {
                .float32 = {0.0f, 0.0f, 0.0f, 1.0f},
            },
        };
        VkClearValue clear_depth = {
            .depthStencil = {
                .depth = 1.0f,
            },
        };
        VkClearValue clear_values[] = {clear_color, clear_depth};
        VkRenderPassBeginInfo begin_info = {
            .sType = STYPE(begin_info),
            .renderPass = vkr.render_pass,
            .framebuffer = vkr.swapchain_framebuffers[image],
            .renderArea = {
                .extent = vkr.swapchain_extent,
            },
            .clearValueCount = ARRAY_SIZE(clear_values),
            .pClearValues = clear_values,
        };
        vkCmdBeginRenderPass(cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.pipeline);
    {
        vkCmdPushConstants(
            cmd_buffer, vkr.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(vkr.proj_view), &vkr.proj_view
        );
    }
    for (const auto& model: vkr.models) {
        VkDeviceSize offset = 0;
        VkBuffer buffer = model->getBuffer();
        unsigned first_vertex = model->getFirstVertex();
        unsigned num_vertices = model->getNumVertices();
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer, &offset);
        vkCmdPushConstants(
            cmd_buffer, vkr.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(vkr.proj_view), sizeof(model->model), &model->model
        );
        vkCmdDraw(cmd_buffer, num_vertices, 1, first_vertex, 0);
    }
    vkCmdEndRenderPass(cmd_buffer);
    res = vkEndCommandBuffer(cmd_buffer);
    if (res < 0) { return res; }

    {
        VkPipelineShaderStageCreateFlags wait_stage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = {
            .sType = STYPE(submit_info),
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &acquire_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &draw_semaphore,
        };
        res = vkQueueSubmit(vkr.queues.graphics, 1, &submit_info, frame_fence);
        if (res < 0) { return res; }
        for (const auto& mesh: vkr.streaming_meshes) {
            mesh->insertFence(frame_fence);
        }
    }
    {
        VkPresentInfoKHR present_info = {
            .sType = STYPE(present_info),
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &draw_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &vkr.swapchain,
            .pImageIndices = &image,
        };
        res = vkQueuePresentKHR(vkr.queues.graphics, &present_info);
        if (res < 0) { return res; }
    }
 
    vkr.frame = (vkr.frame + 1) % vkr.num_swapchain_images;

    return VK_SUCCESS;
}
