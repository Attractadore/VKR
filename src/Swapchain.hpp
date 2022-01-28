#pragma once
#include "VKRVulkan.hpp"

#include <vector>

namespace VKR {
bool presentModeSupported(
    VkPhysicalDevice pdev, VkSurfaceKHR surf, VkPresentModeKHR pmode
);

class Swapchain: public Vulkan::IWSISwapchain {
    static constexpr uint32_t c_pref_img_cnt = 3;

    VkPhysicalDevice m_physical_device;
    uint32_t m_queue_family;
    VkDevice m_device;
    VkQueue m_queue;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    VkExtent2D m_extent;
    VkPresentModeKHR m_pmode;

    std::vector<VkImage> m_images;

    uint32_t m_current_sync = 0;
    std::vector<VkSemaphore> m_sems;
    std::vector<VkFence> m_fences;

public:
    Swapchain(
        VkPhysicalDevice physical_device, uint32_t queue_family,
        VkDevice device, VkQueue queue,
        VkSurfaceKHR surf
    ): m_physical_device(physical_device),
       m_queue_family(queue_family),
       m_device(device),
       m_queue(queue),
       m_surface(surf) {}
    Swapchain(const Swapchain& other) = delete;
    Swapchain(Swapchain&& other);
    Swapchain& operator=(const Swapchain& other) = delete;
    Swapchain& operator=(Swapchain&& other);

    ~Swapchain() {
        vkDeviceWaitIdle(m_device);
        destroy();
    }

    void create(
        VkExtent2D extent, VkPresentModeKHR pres_mode
    );
    void destroy();

    VkExtent2D getExtent() const override {
        return m_extent;
    }

    void setExtent(VkExtent2D extent) override;

    [[nodiscard]]
    std::tuple<uint32_t, VkSemaphore, VkFence> acquireImage() override;
    VkImage getImage(uint32_t img_idx) override {
        return m_images[img_idx];
    }
    void presentImage(uint32_t img_idx, VkSemaphore wait_sem) override;

    Vulkan::LayoutTransitionToTransferDstInserter getLayoutTransitionToTransferDstInserter() override;
    Vulkan::LayoutTransitionFromTransferDstInserter getLayoutTransitionFromTransferDstInserter() override;

    bool presentModeSupported(VkPresentModeKHR pmode) override {
        return ::VKR::presentModeSupported(m_physical_device, m_surface, pmode);
    }

    VkPresentModeKHR getPresentMode() const override {
        return m_pmode;
    }

    void setPresentMode(VkPresentModeKHR pmode) override;
};
}
