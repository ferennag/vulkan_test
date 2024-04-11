#pragma once

#include <stdlib.h>
#include <std/core/logger.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(expr) {                                            \
    VkResult result = expr;                                         \
    if (result != VK_SUCCESS) {                                     \
        LOG_ERROR("Vulkan error: %s", string_VkResult(result));     \
        exit(-1);                                                   \
    }                                                               \
}
