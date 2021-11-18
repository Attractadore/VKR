#include "VKRDevice.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

#include <assert.h>
#include <iso646.h>

typedef struct {
    VkSurfaceFormatKHR formats[VKR_NUM_SURFACE_FORMATS_MAX];
    VkPresentModeKHR present_modes[VKR_NUM_SURFACE_PRESENT_MODES_MAX];
    unsigned num_formats;
    unsigned num_present_modes;
} SurfaceInfo;

typedef struct {
    unsigned supported_bits;
} DepthInfo;

static const char** getRequiredExtensions(unsigned* num_extensions) {
    static const char* extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    *num_extensions = ARRAY_SIZE(extensions);
    return extensions;
}

static const char* extensionAccessor(const void* props) {
    return ((const VkExtensionProperties*) props)->extensionName;
}

static bool deviceExtensionsSupported(
    VkPhysicalDevice device,
    const char** extensions,
    unsigned num_extensions
) {
    VkExtensionProperties supported[VKR_NUM_DEVICE_EXTENSIONS_MAX];
    unsigned num_supported = ARRAY_SIZE(supported);
    if (vkEnumerateDeviceExtensionProperties(device, NULL, &num_supported, supported) < 0) {
        return false;
    }
    return VKR_FeaturesSupported(
        supported, num_supported, sizeof(*supported),
        extensions, num_extensions,
        extensionAccessor
    );
}

static bool deviceSurfaceSupported(const SurfaceInfo* info) {
    return info->num_formats and info->num_present_modes;
}

static int deviceScore(
    VkPhysicalDevice device,
    QueueFamilies queue_families,
    const SurfaceInfo* surface_info,
    unsigned format_info,
    const char** extensions,
    unsigned num_extensions
) {
    if (!deviceExtensionsSupported(device, extensions, num_extensions)) {
        return -1;
    }
    if (!deviceSurfaceSupported(surface_info)) {
        return -1;
    }
    if (!format_info) {
        return -1;
    }
    if (queue_families.graphics < 0) {
        return -1;
    }
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static int findBestDevice(
    const VkPhysicalDevice* devices,
    const QueueFamilies* queue_families,
    const SurfaceInfo* surface_infos,
    const unsigned* format_infos,
    unsigned num_devices
) {
    int best_i = -1;
    int best_score = -1;
    unsigned num_extensions;
    const char** extensions = getRequiredExtensions(&num_extensions);
    for (int i = 0; i < num_devices; i++) {
        int score = deviceScore(
            devices[i], queue_families[i], &surface_infos[i], format_infos[i],
            extensions, num_extensions
        );
        if (score > best_score) {
            best_score = score;
            best_i = i;
        }
    }
    return (best_score >= 0) ? best_i : -1;
}

static int findGraphicsQueueFamily(
    VkPhysicalDevice dev,
    const VkQueueFamilyProperties* properties,
    unsigned num_queue_families
) {
    for (unsigned i = 0; i < num_queue_families; i++) {
        VkBool32 has_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vkr.surface, &has_present);
        if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT and has_present) {
            return i;
        }
    }
    return -1;   
}

static VkResult getPhysicalDevices(VkPhysicalDevice* devices, unsigned* num_devices) {
    return vkEnumeratePhysicalDevices(vkr.instance, num_devices, devices);
}

static void findQueueFamilies(VkPhysicalDevice device, QueueFamilies* queue_families) {
    enum { NUM_QUEUE_FAMILIES_MAX = 8 };
    VkQueueFamilyProperties properties[NUM_QUEUE_FAMILIES_MAX];
    unsigned num_queue_families = NUM_QUEUE_FAMILIES_MAX;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, properties);
    queue_families->graphics = findGraphicsQueueFamily(device, properties, num_queue_families);
}

static VkResult getSurfaceInfo(VkPhysicalDevice dev, SurfaceInfo* info) {
    VkResult res = VK_SUCCESS;
    info->num_formats = VKR_NUM_SURFACE_FORMATS_MAX;
    info->num_present_modes = VKR_NUM_SURFACE_PRESENT_MODES_MAX;

    res = vkGetPhysicalDeviceSurfaceFormatsKHR(
        dev, vkr.surface, &info->num_formats, info->formats
    );
    if (res < 0) {
        return res;
    }
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        dev, vkr.surface, &info->num_present_modes, info->present_modes
    );
    if (res < 0) {
        return res;
    }

    return VK_SUCCESS;
}

static void getFormatInfos(
    VkPhysicalDevice dev, VkFormatFeatureFlags flags,
    unsigned* format_info,
    const VkFormat* formats, unsigned num_formats
) {
    for (unsigned i = 0; i < num_formats; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(dev, formats[i], &props);
        if ((props.optimalTilingFeatures & flags) == flags) {
            SET_BIT(*format_info, i);
        }
    }
}

static VkSurfaceFormatKHR selectSwapchainFormat(
    const VkSurfaceFormatKHR* formats, unsigned num_formats
) {
    for (unsigned i = 0; i < num_formats; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB and
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    return formats[0];
}

static VkFormat selectDepthBufferFormat(
    unsigned format_info,
    const VkFormat* formats, unsigned num_formats
) {
    for (unsigned i = 0; i < num_formats; i++) {
        if (BIT_SET(format_info, i)) {
            return formats[i];
        }
    }
    assert(!"No formats found");
    return formats[0];
}

static VkPresentModeKHR selectSwapchainPresentMode(
    const VkPresentModeKHR* present_modes, unsigned num_present_modes
) {
    for (unsigned i = 0; i < num_present_modes; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkResult VKR_SelectPhysicalDevice() {
    VkResult res = VK_SUCCESS;

    enum { NUM_PHYSICAL_DEVICES_MAX = 8 };
    VkPhysicalDevice devices[NUM_PHYSICAL_DEVICES_MAX];
    QueueFamilies queue_families[NUM_PHYSICAL_DEVICES_MAX];
    SurfaceInfo surface_infos[NUM_PHYSICAL_DEVICES_MAX];
    unsigned format_infos[NUM_PHYSICAL_DEVICES_MAX] = {0};

    unsigned num_devices = NUM_PHYSICAL_DEVICES_MAX;
    res = getPhysicalDevices(devices, &num_devices);
    if (res < 0) { return res; }

    for (unsigned i = 0; i < num_devices; i++) {
        findQueueFamilies(devices[i], &queue_families[i]);
    }

    for (unsigned i = 0; i < num_devices; i++) {
        res = getSurfaceInfo(devices[i], &surface_infos[i]);
        if (res < 0) { return res; }
    }

    VkFormat depth_formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D16_UNORM_S8_UINT,
    };

    for (unsigned i = 0; i < num_devices; i++) {
        getFormatInfos(
            devices[i], VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, &format_infos[i],
            depth_formats, ARRAY_SIZE(depth_formats)
        );
    }
    
    int best_idx = findBestDevice(devices, queue_families, surface_infos, format_infos, num_devices);
    if (best_idx < 0) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    vkr.physical_device = devices[best_idx];
    vkr.queue_families = queue_families[best_idx];
    vkr.swapchain_format = selectSwapchainFormat(
        surface_infos[best_idx].formats, surface_infos[best_idx].num_formats
    );
    vkr.swapchain_present_mode = selectSwapchainPresentMode(
        surface_infos[best_idx].present_modes, surface_infos[best_idx].num_present_modes
    );
    vkr.depth_buffer_format = selectDepthBufferFormat(
        format_infos[best_idx], depth_formats, ARRAY_SIZE(depth_formats)
    );

    return VK_SUCCESS;
}

static VkResult VKR_CreateLogicalDevice() {
    VkResult res = VK_SUCCESS;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vkr.queue_families.graphics,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };

    unsigned num_extensions;
    const char** extensions = getRequiredExtensions(&num_extensions);

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions,
    };

    res = vkCreateDevice(vkr.physical_device, &device_create_info, NULL, &vkr.device);
    if (res < 0) { return res; }
    vkGetDeviceQueue(vkr.device, vkr.queue_families.graphics, 0, &vkr.queues.graphics);
    
    return VK_SUCCESS;
}

static void VKR_DestroyLogicalDevice() {
    vkDestroyDevice(vkr.device, NULL);
}

VkResult VKR_CreateDevice() {
    VkResult res = VK_SUCCESS;

    res = VKR_SelectPhysicalDevice();
    if (res < 0) { return res; }
    res = VKR_CreateLogicalDevice();
    if (res < 0) { return res; }

    return VK_SUCCESS;
}

void VKR_DestroyDevice() {
    VKR_DestroyLogicalDevice();
}
