#include "VKRSwapchain.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"
#include "VKRMemory.hpp"

#include <iso646.h>

static VkResult selectExtent(
    const VkSurfaceCapabilitiesKHR* capabilities,
    VKR_GetFramebufferSizeCallbackT cb,
    VkExtent2D* extent
) {
    VkResult res = VK_SUCCESS;
    *extent = capabilities->currentExtent;
    if (extent->width == UINT32_MAX and extent->height == UINT32_MAX) {
        unsigned new_width, new_height;
        res = cb(&new_width, &new_height);
        if (res < 0) { return res; }
        extent->width = CLAMP(
            new_width,
            capabilities->minImageExtent.width,
            capabilities->maxImageExtent.width
        );
        extent->height = CLAMP(
            new_height,
            capabilities->minImageExtent.height,
            capabilities->maxImageExtent.height
        );
    }
    return VK_SUCCESS;
}

static unsigned selectImageCount(
    const VkSurfaceCapabilitiesKHR* capabilities
) {
    unsigned image_count = capabilities->minImageCount + 1;
    if (capabilities->maxImageCount) {
        image_count = MIN(image_count, capabilities->maxImageCount);
    }
    return image_count;
}

VkResult createSwapchain(VKR_GetFramebufferSizeCallbackT cb) {
    VkResult res = VK_SUCCESS;

    VkSurfaceCapabilitiesKHR capabilities;
    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vkr.physical_device, vkr.surface, &capabilities
    );
    if (res < 0) { return res; }

    res = selectExtent(&capabilities, cb, &vkr.swapchain_extent);
    if (res < 0) { return res; }
    vkr.num_swapchain_images = selectImageCount(&capabilities);
    
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkr.surface,
        .minImageCount = vkr.num_swapchain_images,
        .imageFormat = vkr.swapchain_format.format,
        .imageColorSpace = vkr.swapchain_format.colorSpace,
        .imageExtent = vkr.swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vkr.swapchain_present_mode,
        .clipped = VK_TRUE,
    };
    res = vkCreateSwapchainKHR(vkr.device, &create_info, NULL, &vkr.swapchain);
    if (res < 0) { return res; }

    res = vkGetSwapchainImagesKHR(
        vkr.device, vkr.swapchain,
        &vkr.num_swapchain_images, NULL 
    );
    if (vkr.num_swapchain_images > VKR_NUM_SWAPCHAIN_IMAGES_MAX) {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    if (res < 0) {
        vkDestroySwapchainKHR(vkr.device, vkr.swapchain, NULL);
        return res;
    }

    return VK_SUCCESS;
}

static void destroyImageViews(VkImageView* image_views, unsigned num_image_views) {
    for (unsigned i = 0; i < num_image_views; i++) {
        vkDestroyImageView(vkr.device, image_views[i], NULL);
    }
}

typedef VkImage (*ImageAccessorT)(const void* data);

static VkResult createImageViews(
    const void* images, unsigned num_images, unsigned image_size,
    VkImageView* image_views,
    VkFormat format, VkImageAspectFlags aspect_mask,
    ImageAccessorT accessor
) {
    VkResult res = VK_SUCCESS;

    for (unsigned i = 0; i < vkr.num_swapchain_images; i++) {
        const char* ptr = ((const char*) images) + image_size * i;
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = accessor(ptr),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        res = vkCreateImageView(
            vkr.device, &create_info, NULL, &image_views[i]
        );
        if (res < 0) {
            destroyImageViews(image_views, i);
            return res;
        }
    }

    return VK_SUCCESS;
}

VkImage swapchainAccessor(const void* data) {
    return *(const VkImage*) data;
}

static VkResult createSwapchainImageViews() {
    VkResult res = VK_SUCCESS;

    VkImage swapchain_images[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
    res = vkGetSwapchainImagesKHR(
        vkr.device, vkr.swapchain, &vkr.num_swapchain_images, swapchain_images
    );
    if (res < 0) { return res; }

    res = createImageViews(
        swapchain_images, vkr.num_swapchain_images, sizeof(*swapchain_images),
        vkr.swapchain_image_views,
        vkr.swapchain_format.format, VK_IMAGE_ASPECT_COLOR_BIT,
        swapchainAccessor
    );
    if (res < 0) { return res; }

    return VK_SUCCESS;
}

static void destroySwapchainImageViews() {
    destroyImageViews(vkr.swapchain_image_views, vkr.num_swapchain_images);
}

static void destroyDepthBuffers() {
    for (unsigned i = 0; i < vkr.num_swapchain_images; i++) {
        VKR_FreeImage(&vkr.depth_buffers[i]);
    }
}

static VkResult createDepthBuffers() {
    VkResult res = VK_SUCCESS;
    for (unsigned i = 0; i < vkr.num_swapchain_images; i++) {
        res = VKR_CreateImage(vkr.swapchain_extent, vkr.depth_buffer_format, &vkr.depth_buffers[i]);
        if (res < 0) {
            destroyDepthBuffers();
            return res;
        }
    }
    return VK_SUCCESS;
}

VkImage depthBufferAccessor(const void* data) {
    return ((const VKRImage*) data)->img;
}

static VkResult createDepthBufferViews() {
    return createImageViews(
        vkr.depth_buffers, vkr.num_swapchain_images, sizeof(*vkr.depth_buffers),
        vkr.depth_buffer_views,
        vkr.depth_buffer_format, VK_IMAGE_ASPECT_DEPTH_BIT,
        depthBufferAccessor
    );
}

static void destroyDepthBufferViews() {
    destroyImageViews(vkr.depth_buffer_views, vkr.num_swapchain_images);
}

static void destroySwapchainFramebuffers();

static VkResult createSwapchainFramebuffers() {
    VkResult res = VK_SUCCESS;

    for (unsigned i = 0; i < vkr.num_swapchain_images; i++) {
        VkImageView attachments[] =
            {vkr.swapchain_image_views[i], vkr.depth_buffer_views[i]};
        VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkr.render_pass,
            .attachmentCount = ARRAY_SIZE(attachments),
            .pAttachments = attachments,
            .width = vkr.swapchain_extent.width,
            .height = vkr.swapchain_extent.height,
            .layers = 1,
        };
        res = vkCreateFramebuffer(
            vkr.device, &create_info, NULL, &vkr.swapchain_framebuffers[i]
        );
        if (res < 0) {
            destroySwapchainFramebuffers();
            return res;
        }
    }

    return VK_SUCCESS;
}

static void destroySwapchainFramebuffers() {
    for (unsigned i = 0; i < vkr.num_swapchain_images; i++) {
        vkDestroyFramebuffer(vkr.device, vkr.swapchain_framebuffers[i], NULL);
    }
}

VkResult VKR_CreateSwapchain(VKR_GetFramebufferSizeCallbackT cb) {
    VkResult res = VK_SUCCESS;

    res = createSwapchain(cb);
    if (res < 0) { goto fail; }
    res = createSwapchainImageViews();
    if (res < 0) { goto fail; }
    res = createDepthBuffers();
    if (res < 0) { goto fail; }
    res = createDepthBufferViews();
    if (res < 0) { goto fail; }
    res = createSwapchainFramebuffers();
    if (res < 0) { goto fail; }

    return VK_SUCCESS;

fail:
    VKR_DestroySwapchain();
    
    return res;
}

void VKR_DestroySwapchain() {
    destroySwapchainFramebuffers();
    destroyDepthBufferViews();
    destroyDepthBuffers();
    destroySwapchainImageViews();
    vkDestroySwapchainKHR(vkr.device, vkr.swapchain, NULL);
}
