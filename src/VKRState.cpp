#include "VKRMatrix.hpp"
#include "VKRState.hpp"

VKRState vkr;

void VKR_SetProjViewMatrix(const glm::mat4& proj_view) {
    vkr.proj_view = proj_view;
}
