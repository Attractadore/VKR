#include "VKRState.hpp"
#include "VKRSurface.hpp"

VkResult VKR_CreateSurface(VKR_CreateSurfaceCallbackT cb) {
    return cb(vkr.instance, NULL, &vkr.surface);
}

void VKR_DestroySurface() {
    vkDestroySurfaceKHR(vkr.instance, vkr.surface, NULL);
}
