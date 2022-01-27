#include "Internal.hpp"
#include "Sync.hpp"
#include "WSISwapchain.hpp"

#include <algorithm>
#include <cassert>

namespace VKR {
namespace {
uint32_t selectSwapchainImageCount(
    const VkSurfaceCapabilitiesKHR& surf_caps,
    uint32_t pref_img_cnt
) {
    uint32_t img_cnt = std::max(pref_img_cnt, surf_caps.minImageCount);
    if (surf_caps.maxImageCount) {
        img_cnt = std::min(img_cnt, surf_caps.maxImageCount);
    }
    return img_cnt;
}

VkExtent2D selectSwapchainExtent(
    const VkSurfaceCapabilitiesKHR& surf_caps,
    VkExtent2D pref_extent
) {
    auto ext = surf_caps.currentExtent;
    constexpr auto c_spec_val = UINT32_MAX;
    if (ext.width == c_spec_val and ext.height == c_spec_val) {
        ext = pref_extent;
    }
    ext.width = std::clamp(
        ext.width, 
        surf_caps.minImageExtent.width,
        surf_caps.maxImageExtent.width
    );
    ext.height = std::clamp(
        ext.height, 
        surf_caps.minImageExtent.height,
        surf_caps.maxImageExtent.height
    );
    return ext;
}

VkFormat selectSwapchainFormat(
    std::span<const VkSurfaceFormatKHR> surf_fmts
) {

    auto fmt_it = std::ranges::find_first_of(
        srgb_fmts, surf_fmts,
        {},
        {}, 
        [](VkSurfaceFormatKHR surf_fmt) {
            return surf_fmt.format;
        }
    );
    assert(fmt_it != srgb_fmts.end());
    return *fmt_it;
}

VkSwapchainKHR createSwapchain(
    VkDevice device, VkSurfaceKHR surface,
    uint32_t img_cnt, VkExtent2D ext,
    VkFormat fmt,
    VkSurfaceTransformFlagBitsKHR trans,
    VkPresentModeKHR pres_mode,
    VkSwapchainKHR old_swapchain
) {
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = img_cnt,
        .imageFormat = fmt,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = ext,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = trans,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = pres_mode,
        .clipped = true,
        .oldSwapchain = old_swapchain,
    };

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);

    return swapchain;
}
}

WSISwapchain::WSISwapchain(
    VkPhysicalDevice physical_device, uint32_t queue_family,
    VkDevice device, VkQueue queue,
    VkSurfaceKHR surf
): m_physical_device(physical_device),
   m_queue_family(queue_family),
   m_device(device),
   m_queue(queue),
   m_surface(surf) {}

WSISwapchain::~WSISwapchain() {
    vkDeviceWaitIdle(m_device);
    destroy();
}

void WSISwapchain::create(
    VkExtent2D extent, VkPresentModeKHR pmode
) {
    VkSurfaceCapabilitiesKHR surf_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_physical_device, m_surface, &surf_caps
    );

    uint32_t surf_fmt_cnt;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &surf_fmt_cnt, nullptr);
    std::vector<VkSurfaceFormatKHR> surf_fmts(surf_fmt_cnt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &surf_fmt_cnt, surf_fmts.data());

    auto img_cnt = selectSwapchainImageCount(surf_caps, c_pref_img_cnt);
    m_extent = selectSwapchainExtent(surf_caps, extent);
    auto fmt = selectSwapchainFormat(surf_fmts);
    assert(presentModeSupported(pmode));
    m_pmode = pmode;

    auto new_swapchain = createSwapchain(
        m_device, m_surface,
        img_cnt, m_extent,
        fmt, surf_caps.currentTransform,
        m_pmode,
        m_swapchain
    );

    // TODO: this should happen asynchronously
    vkDeviceWaitIdle(m_device);
    destroy();

    m_swapchain = new_swapchain;

    uint32_t created_img_cnt;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &created_img_cnt, nullptr);
    m_images.resize(created_img_cnt);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &created_img_cnt, m_images.data());

    m_sems.resize(created_img_cnt);
    for (auto& sem: m_sems) {
        sem = createSemaphore(m_device);
    }
    m_fences.resize(created_img_cnt);
    for (auto& fence: m_fences) {
        fence = createUnsignaledFence(m_device);
    }
}

void WSISwapchain::destroy() {
    for (auto& fence: m_fences) {
        vkDestroyFence(m_device, fence, nullptr);
    }
    for (auto& sem: m_sems) {
        vkDestroySemaphore(m_device, sem, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

VkExtent2D WSISwapchain::getExtent() const {
    return m_extent;
}

void WSISwapchain::setExtent(VkExtent2D extent) {
    create(extent, m_pmode);
}

[[nodiscard]]
std::tuple<uint32_t, VkSemaphore, VkFence> WSISwapchain::acquireImage() {
    // TODO: handle VK_ERROR_OUT_OF_DATE_KHR
    uint32_t current_image;
    auto& sem = m_sems[m_current_sync];
    auto& fence = m_fences[m_current_sync];
    vkAcquireNextImageKHR(
        m_device, m_swapchain,
        UINT64_MAX, sem, fence,
        &current_image
    );
    m_current_sync = (m_current_sync + 1) % m_sems.size();
    return {current_image, sem, fence};
}

VkImage WSISwapchain::getImage(uint32_t i) {
    return m_images[i];
}

void WSISwapchain::presentImage(uint32_t img_idx, VkSemaphore wait_sem) {
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &wait_sem,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &img_idx
    };
    vkQueuePresentKHR(m_queue, &present_info);
}

LayoutTransitionToTransferDstInserter WSISwapchain::getLayoutTransitionToTransferDstInserter() {
    auto queue_family = m_queue_family;
    return
    [=](VkCommandBuffer cmd_buffer,
        VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
        VkImageMemoryBarrier& bar
    ) {
        bar.srcAccessMask = 0;
        bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bar.srcQueueFamilyIndex = queue_family;
        vkCmdPipelineBarrier(
            cmd_buffer,
            src_stage_mask, dst_stage_mask, 0,
            0, nullptr, 0, nullptr, 1, &bar
        );
    };
}

LayoutTransitionFromTransferDstInserter WSISwapchain::getLayoutTransitionFromTransferDstInserter() {
    auto queue_family = m_queue_family;
    return
    [=](
        VkCommandBuffer cmd_buffer,
        VkPipelineStageFlags src_stage_mask,
        VkImageMemoryBarrier& bar
    ) {
        bar.dstAccessMask = 0;
        bar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        bar.dstQueueFamilyIndex = queue_family;
        vkCmdPipelineBarrier(
            cmd_buffer,
            src_stage_mask, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
            0, nullptr, 0, nullptr, 1, &bar
        );
    };
}

bool WSISwapchain::presentModeSupported(VkPresentModeKHR pmode) {
    return WSIPresentModeSupported(m_physical_device, m_surface, pmode);
}

VkPresentModeKHR WSISwapchain::getPresentMode() const {
    return m_pmode;
}

void WSISwapchain::setPresentMode(VkPresentModeKHR pmode) {
    create(m_extent, pmode);
}

bool WSIPresentModeSupported(
    VkPhysicalDevice pdev, VkSurfaceKHR surf, VkPresentModeKHR pmode
) {
    uint32_t pmode_cnt;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        pdev, surf, &pmode_cnt, nullptr
    );
    std::vector<VkPresentModeKHR> pmodes(pmode_cnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        pdev, surf, &pmode_cnt, pmodes.data() 
    );

    auto it = std::ranges::find(pmodes, pmode);
    return it != pmodes.end();
}
}
