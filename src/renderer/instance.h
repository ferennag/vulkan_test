#pragma once

#include "defines.h"
#include "vulkan_types.h"
#include <SDL.h>

typedef struct Instance {
    VkInstance vk_instance;
} Instance;

bool instance_create(SDL_Window *window, const char *app_name, Instance *out);

void instance_destroy(Instance *instance);