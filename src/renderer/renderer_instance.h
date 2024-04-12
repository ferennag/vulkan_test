#pragma once

#include <std/defines.h>
#include "vulkan_types.h"

typedef struct VulkanContext VulkanContext;

typedef struct RendererInstance {
    VkCommandBuffer command_buffer;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
} RendererInstance;

void renderer_instance_create(VulkanContext *context, u32 count);

void renderer_instance_destroy(VulkanContext *context);
