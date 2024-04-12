#pragma once

#include "vulkan_types.h"
#include "vulkan.h"

typedef struct RendererInstance RendererInstance;

void command_buffer_create(VulkanContext *context, RendererInstance *out);

void command_buffer_destroy(RendererInstance *instance);

void command_buffer_begin(RendererInstance *instance);

void command_buffer_end(RendererInstance *instance);