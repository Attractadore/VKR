#include "VKRState.hpp"
#include "VKRUtils.hpp"

#include <string.h>

bool VKR_FeatureSupported(
    const void* supported,
    unsigned num_supported,
    unsigned supported_size,
    const char* feature,
    AccessFunctionT accessor
) {
    auto supported_char = static_cast<const char*>(supported);
    for (unsigned i = 0; i < num_supported; i++) {
        const char* supported_feature =
            accessor(supported_char + i * supported_size);
        if (strcmp(supported_feature, feature) == 0) {
            return true;
        }
    }
    return false;
}

bool VKR_FeaturesSupported(
    const void* supported,
    unsigned num_supported,
    unsigned supported_size,
    const char** features,
    unsigned num_features,
    AccessFunctionT accessor
) {
    bool all = true;
    for (unsigned i = 0; i < num_features; i++) {
        if (!VKR_FeatureSupported(supported, num_supported, supported_size, features[i], accessor)) {
            return false;
        }
    }
    return true;
}
