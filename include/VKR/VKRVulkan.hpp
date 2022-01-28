#pragma once
#include "VKR.hpp"

#include <vulkan/vulkan.h>

#include <functional>

namespace VKR {
namespace Vulkan {
using LayoutTransitionToTransferDstInserter =
    std::function<void (
        VkCommandBuffer cmd_buffer,
        VkPipelineStageFlagBits src_stage_mask,
        VkPipelineStageFlagBits dst_stage_mask,
        VkImageMemoryBarrier& image_barrier
    )>;
using LayoutTransitionFromTransferDstInserter =
    std::function<void (
        VkCommandBuffer cmd_buffer,
        VkPipelineStageFlagBits src_stage_mask,
        VkImageMemoryBarrier& image_barrier
    )>;

using VkSurfaceKHRCreator =
    std::function<VkResult (
        VkInstance instance,
        const VkAllocationCallbacks* allocator,
        VkSurfaceKHR* surface
    )>;

class Instance: public VKR::Instance {
public:
    WSISurface& createWSISurface(VkSurfaceKHRCreator create_surface);
};

class GraphicsDevice: public VKR::GraphicsDevice {};

class GraphicsDeviceConnection: public VKR::GraphicsDeviceConnection {};

class WSISurface {
protected:
    WSISurface() = default;
    WSISurface(const WSISurface& other) = default;
    WSISurface(WSISurface&& other) = default;
    WSISurface& operator=(const WSISurface& other) = default;
    WSISurface& operator=(WSISurface&& other) = default;

public:
    bool presentModeSupported(
        const VKR::GraphicsDeviceConnection* connection,
        VkPresentModeKHR pmode
    ) const;

    IWSISwapchain& createWSISwapchain(
        VKR::GraphicsDeviceConnection* connection,
        VkExtent2D extent,
        VkPresentModeKHR pmode = VK_PRESENT_MODE_FIFO_KHR
    );
};

class ISwapchain {
public:
    virtual ~ISwapchain() {}

    virtual VkExtent2D getExtent() const = 0;
    virtual void setExtent(VkExtent2D extent) = 0;

    [[nodiscard]]
    virtual std::tuple<uint32_t, VkSemaphore, VkFence> acquireImage() = 0;
    virtual VkImage getImage(uint32_t img_idx) = 0;
    virtual void presentImage(uint32_t img_idx, VkSemaphore wait_sem) = 0;

    virtual LayoutTransitionToTransferDstInserter getLayoutTransitionToTransferDstInserter() = 0;
    virtual LayoutTransitionFromTransferDstInserter getLayoutTransitionFromTransferDstInserter() = 0;
};

class IWSISwapchain: public ISwapchain {
public:
    virtual ~IWSISwapchain() {}

    virtual bool presentModeSupported(VkPresentModeKHR pmode) = 0;
    virtual VkPresentModeKHR getPresentMode() const = 0;
    virtual void setPresentMode(VkPresentModeKHR pmode) = 0;
};
}
}
