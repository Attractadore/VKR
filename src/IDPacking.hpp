#pragma once
#include "VKR.hpp"

#include <bit>

namespace VKR {
inline constexpr auto getStorageWidth() {
        return std::bit_width(
            static_cast<Detail::storageid>(MeshStorageFormat::Last)
        );
    }

inline constexpr auto getMeshIDWidth() {
    return std::bit_width(
        std::numeric_limits<Detail::meshid>::max()
    );
}

inline constexpr auto getMeshIDIndexWidth() {
    return getMeshIDWidth() - getStorageWidth();
}

inline constexpr auto getModelIDWidth() {
    return std::bit_width(
        std::numeric_limits<Detail::modelid>::max()
    );
}

inline constexpr auto getModelIDIndexWidth() {
    return getModelIDWidth() - getStorageWidth();
}

struct MeshIDImpl {
    Detail::meshid index: getMeshIDIndexWidth();
    MeshStorageFormat storage_format: getStorageWidth();
};
static_assert(sizeof(MeshIDImpl) == sizeof(MeshID));

struct MaterialIDImpl {
    Detail::materialid index;
};
static_assert(sizeof(MaterialIDImpl) == sizeof(MaterialID));

struct ModelIDImpl {
    Detail::modelid index: getModelIDIndexWidth();
    MeshStorageFormat storage_format: getStorageWidth();
};
static_assert(sizeof(ModelIDImpl) == sizeof(ModelID));

inline MeshStorageFormat getMeshStorageFormat(MeshID mesh) {
    return std::bit_cast<MeshIDImpl>(mesh).storage_format;
}

inline size_t getMeshIndex(MeshID mesh) {
    return std::bit_cast<MeshIDImpl>(mesh).index;
}

inline MeshID makeMeshID(
    Detail::meshid index, MeshStorageFormat storage_format
) {
    MeshIDImpl id = {
        .index = index,
        .storage_format = storage_format,
    };
    return std::bit_cast<MeshID>(id);
}

inline MeshStorageFormat getModelMeshStorageFormat(ModelID model) {
    return std::bit_cast<ModelIDImpl>(model).storage_format;
}

inline size_t getModelIndex(ModelID mesh) {
    return std::bit_cast<ModelIDImpl>(mesh).index;
}

inline ModelID makeModelID(
    Detail::modelid index, MeshStorageFormat storage_format
) {
    ModelIDImpl id = {
        .index = index,
        .storage_format = MeshStorageFormat::Static,
    };
    return std::bit_cast<ModelID>(id);
}
}
