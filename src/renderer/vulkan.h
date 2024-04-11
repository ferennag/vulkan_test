#pragma once

#include <SDL.h>
#include <std/std.h>
#include <std/core/logger.h>

#include "defines.h"
#include "vulkan_types.h"
#include "physical_device.h"
#include "device.h"
#include "vulkan_instance.h"
#include "swapchain.h"


typedef struct VulkanContext {
    VulkanInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physical_device;
    Device device;
    Swapchain swapchain;
    VkExtent2D extent;
} VulkanContext;

bool initVulkan(SDL_Window *window, const char *app_name);

void shutdownVulkan();