#pragma once
#include "VKRMatrix.hpp"
#include "VKRMesh.hpp"

#ifdef __cplusplus
extern "C" {
#endif

VkResult VKR_Init(
    VKR_GetInstanceExtensionsCallbackT extensions_cb,
    VKR_CreateSurfaceCallbackT surface_cb,
    VKR_GetFramebufferSizeCallbackT framebuffer_cb,
    VKR_GetShaderBinaryCallbackT vertex_cb,
    VKR_GetShaderBinaryCallbackT fragment_cb
);
void VKR_Quit();

VkResult VKR_Draw();

#ifdef __cplusplus
}
#endif
