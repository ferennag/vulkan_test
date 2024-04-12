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

typedef struct VulkanContext {
    VulkanInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physical_device;
    Device device;
    Swapchain swapchain;
    GraphicsPipeline graphics_pipeline;
    VkFramebuffer *framebuffers;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
} VulkanContext;

bool vulkan_init(SDL_Window *window, const char *app_name);

void vulkan_shutdown();

void render();