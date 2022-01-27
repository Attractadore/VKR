#pragma once
#include <vk_mem_alloc.h>

namespace VKR {
struct Image {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    void create(
        VmaAllocator allocator,
        VkFormat format,
        VkImageCreateFlags flags,
        uint32_t width, uint32_t height,
        VkImageUsageFlags usage
    );

    void destroy(VmaAllocator allocator) {
        vmaDestroyImage(allocator, image, allocation);
    }
};

[[nodiscard]]
inline auto createImage(
    VmaAllocator allocator,
    VkFormat format,
    VkImageCreateFlags flags,
    uint32_t width, uint32_t height,
    VkImageUsageFlags usage
) {
    Image img;
    img.create(allocator, format, flags, width, height, usage);
    return img;
}

[[nodiscard]]
inline auto createColorImage(
    VmaAllocator allocator,
    VkFormat format,
    uint32_t width, uint32_t height
) {
    return createImage(
        allocator, format, 0, width, height,
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    );
}

[[nodiscard]]
inline auto createDepthImage(
    VmaAllocator allocator,
    VkFormat format,
    uint32_t width, uint32_t height
) {
    return createImage(
        allocator, format, 0, width, height,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

[[nodiscard]]
VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspect
);

[[nodiscard]]
inline auto createColorImageView(
    VkDevice device,
    VkImage image,
    VkFormat format
) {
    return createImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

[[nodiscard]]
inline auto createDepthImageView(
    VkDevice device,
    VkImage image,
    VkFormat format
) {
    return createImageView(device, image, format, VK_IMAGE_ASPECT_DEPTH_BIT);
}
}
