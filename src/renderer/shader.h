#pragma once

#include <std/defines.h>
#include "vulkan_types.h"
#include "device.h"

typedef struct Shader {
    VkShaderModule vertex;
    VkShaderModule fragment;
} Shader;

bool shader_load(Device *device, const char *vertex, const char *fragment, Shader *out);

void shader_destroy(Device *device, Shader *shader);