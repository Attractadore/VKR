#pragma once
#include "VKR.hpp"

#include <vulkan/vulkan.h>

#include <functional>

namespace VKR {
using LayoutTransitionToTransferDstInserter =
    std::function<void (VkCommandBuffer, VkPipelineStageFlagBits, VkPipelineStageFlagBits, VkImageMemoryBarrier&)>;
using LayoutTransitionFromTransferDstInserter =
    std::function<void (VkCommandBuffer, VkPipelineStageFlagBits, VkImageMemoryBarrier&)>;

namespace Vulkan {
class Instance: public VKR::Instance {
public:
};

class GraphicsDevice: public VKR::GraphicsDevice {
public:
};

class GraphicsDeviceConnection: public VKR::GraphicsDeviceConnection {
public:
    bool WSISwapchainPresentModeSupported(
        VkSurfaceKHR surf,
        VkPresentModeKHR pmode
    ) const;

    IWSISwapchain& createWSISwapchain(
        VkSurfaceKHR surf,
        VkExtent2D extent,
        VkPresentModeKHR pmode = VK_PRESENT_MODE_FIFO_KHR
    );
};

class ISwapchain {
public:
    virtual ~ISwapchain() = 0;

    virtual VkExtent2D getExtent() const;
    virtual void setExtent(VkExtent2D extent);

    [[nodiscard]]
    virtual std::tuple<uint32_t, VkSemaphore, VkFence> acquireImage();
    virtual VkImage getImage(uint32_t img_idx);
    virtual void presentImage(uint32_t img_idx, VkSemaphore wait_sem);

    virtual LayoutTransitionToTransferDstInserter getLayoutTransitionToTransferDstInserter();
    virtual LayoutTransitionFromTransferDstInserter getLayoutTransitionFromTransferDstInserter();
};
inline ISwapchain::~ISwapchain() {}

class IWSISwapchain: public ISwapchain {
public:
    virtual ~IWSISwapchain() = 0;

    virtual bool presentModeSupported(VkPresentModeKHR pmode);
    virtual VkPresentModeKHR getPresentMode() const;
    virtual void setPresentMode(VkPresentModeKHR pmode);
};
inline IWSISwapchain::~IWSISwapchain() {}

class Scene: public VKR::Scene {
public:
};
}
}
