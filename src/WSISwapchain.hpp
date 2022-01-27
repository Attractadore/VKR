#pragma once
#include "VKRVulkan.hpp"

#include <vector>

namespace VKR {
class WSISwapchain: public Vulkan::IWSISwapchain {
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
    WSISwapchain(
        VkPhysicalDevice physical_device, uint32_t queue_family,
        VkDevice device, VkQueue queue,
        VkSurfaceKHR surf
    );
    ~WSISwapchain();

    void create(
        VkExtent2D extent, VkPresentModeKHR pres_mode
    );

    void destroy();

    VkExtent2D getExtent() const override;
    void setExtent(VkExtent2D extent) override;

    [[nodiscard]]
    std::tuple<uint32_t, VkSemaphore, VkFence> acquireImage() override;
    VkImage getImage(uint32_t img_idx) override;
    void presentImage(uint32_t img_idx, VkSemaphore wait_sem) override;

    LayoutTransitionToTransferDstInserter getLayoutTransitionToTransferDstInserter() override;
    LayoutTransitionFromTransferDstInserter getLayoutTransitionFromTransferDstInserter() override;

    bool presentModeSupported(VkPresentModeKHR pmode) override;
    VkPresentModeKHR getPresentMode() const override;
    void setPresentMode(VkPresentModeKHR pmode) override;
};

bool WSIPresentModeSupported(
    VkPhysicalDevice pdev, VkSurfaceKHR surf, VkPresentModeKHR pmode
);
}
