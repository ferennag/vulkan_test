#pragma once

#include <SDL.h>
#include <std/std.h>
#include <std/core/logger.h>

#include <std/defines.h>
#include "vulkan_types.h"
#include "physical_device.h"
#include "device.h"
#include "vulkan_instance.h"
#include "swapchain.h"
#include "graphics_pipeline.h"
#include "renderer_instance.h"

typedef struct VulkanContext {
    VulkanInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physical_device;
    Device device;
    Swapchain swapchain;
    GraphicsPipeline graphics_pipeline;
    VkFramebuffer *framebuffers;

    VkCommandPool command_pool;

    RendererInstance *renderer_instances;
    RendererInstance *current_renderer;
    u32 current_renderer_index;
} VulkanContext;

bool vulkan_init(SDL_Window *window, const char *app_name);

void vulkan_shutdown();

void render();