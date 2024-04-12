#pragma once

#include <std/defines.h>
#include "vulkan_types.h"
#include <SDL.h>

typedef struct VulkanInstance {
    VkInstance vk_instance;
} VulkanInstance;

bool vulkan_instance_create(SDL_Window *window, const char *app_name, VulkanInstance *out);

void vulkan_instance_destroy(VulkanInstance *instance);