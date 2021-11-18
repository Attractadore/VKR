#pragma once
#include "VKRTypes.hpp"

VkResult VKR_CreatePipeline(
    VKR_GetShaderBinaryCallbackT vertex_cb,
    VKR_GetShaderBinaryCallbackT fragment_cb
);

void VKR_DestroyPipeline();
