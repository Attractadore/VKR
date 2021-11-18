#include "VKRDebugMessenger.hpp"
#include "VKRInstance.hpp"
#include "VKRState.hpp"
#include "VKRUtils.hpp"

#include <string.h>

static const char** getRequiredLayers(unsigned* num_layers) {
#ifndef NDEBUG
    static const char* debug_layers[] = {"VK_LAYER_KHRONOS_validation"};
    *num_layers = ARRAY_SIZE(debug_layers);
    return debug_layers;
#endif
    *num_layers = 0;
    return NULL;
};

static const char** getRequiredExtensions(unsigned* num_extensions) {
#ifndef NDEBUG
    static const char* debug_extensions[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    *num_extensions = ARRAY_SIZE(debug_extensions);
    return debug_extensions;
#endif
    *num_extensions = 0;
    return NULL;
}

static const char* layerAccessor(const void* layer_properties) {
    return ((const VkLayerProperties*)layer_properties)->layerName;
}

static bool instanceLayersSupported(const char** layers, unsigned num_layers) {
    VkLayerProperties supported[VKR_NUM_INSTANCE_LAYERS_MAX];
    unsigned num_supported = ARRAY_SIZE(supported);
    if (vkEnumerateInstanceLayerProperties(&num_supported, supported) < 0) {
        return false;
    }
    return VKR_FeaturesSupported(
        supported, num_supported, sizeof(*supported),
        layers, num_layers,
        layerAccessor
    );
}

static const char* extensionAccessor(const void* extensions_properties) {
    return ((const VkExtensionProperties*)extensions_properties)->extensionName;
}

static bool instanceExtensionsSupported(const char** extensions, unsigned num_extensions) {
    VkExtensionProperties supported[VKR_NUM_INSTANCE_EXTENSIONS_MAX];
    unsigned num_supported = ARRAY_SIZE(supported);
    if (vkEnumerateInstanceExtensionProperties(NULL, &num_supported, supported) < 0) {
        return false;
    }
    return VKR_FeaturesSupported(
        supported, num_supported, sizeof(*supported),
        extensions, num_extensions,
        extensionAccessor
    );
}

VkResult VKR_CreateInstance(VKR_GetInstanceExtensionsCallbackT cb) {
    VkResult res = VK_SUCCESS;
    vkr.instance = VK_NULL_HANDLE;
#ifndef NDEBUG
    vkr.debug_messenger = VK_NULL_HANDLE;
#endif

    unsigned num_layers;
    const char** layers = getRequiredLayers(&num_layers);

    const char* extensions[VKR_NUM_INSTANCE_EXTENSIONS_MAX];
    unsigned num_extensions = ARRAY_SIZE(extensions);
    res = cb(&num_extensions, extensions);
    if (res == VK_INCOMPLETE) { return VK_ERROR_OUT_OF_HOST_MEMORY; }
    if (res < 0) { return res; }
    {
        unsigned num_required_extensions;
        const char** required_extensions = getRequiredExtensions(&num_required_extensions);
        if (num_required_extensions + num_extensions > ARRAY_SIZE(extensions)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        memcpy(extensions + num_extensions, required_extensions, num_required_extensions * sizeof(*extensions));
        num_extensions += num_required_extensions;
    }

    if (!instanceLayersSupported(layers, num_layers)) {
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (!instanceExtensionsSupported(extensions, num_extensions)) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VKR_DebugCallback,
    };
#endif

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
        .pNext = &debug_messenger_create_info,
#endif
        .enabledLayerCount = num_layers,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions,
    };

    res = vkCreateInstance(&instance_create_info, NULL, &vkr.instance);
    if (res < 0) { return res; }
#ifndef NDEBUG
    res = VKR_vkCreateDebugMessenger(vkr.instance, &debug_messenger_create_info, NULL, &vkr.debug_messenger);
    if (res < 0) {
        vkDestroyInstance(vkr.instance, NULL);
        return res;
    }
#endif

    return VK_SUCCESS;
}

void VKR_DestroyInstance() {
#ifndef NDEBUG
    VKR_vkDestroyDebugMessenger(vkr.instance, vkr.debug_messenger, NULL);
#endif
    vkDestroyInstance(vkr.instance, NULL);
}
