#include "VKRDebugMessenger.hpp"
#include "VKRUtils.hpp"

#include <stdio.h>

VkResult VKR_vkCreateDebugMessenger(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger
) {
    PFN_vkCreateDebugUtilsMessengerEXT f = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, STRING(vkCreateDebugUtilsMessengerEXT));
    if (!f) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return f(instance, pCreateInfo, pAllocator, pMessenger);
}

void VKR_vkDestroyDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator
) {
    PFN_vkDestroyDebugUtilsMessengerEXT f = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, STRING(vkDestroyDebugUtilsMessengerEXT));
    if (f) {
        f(instance, messenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VKR_DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* p)
{
    (void)type;
    (void)p;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "VULKAN ERROR: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}
