#pragma once

#include <SDL.h>
#include <std/std.h>
#include <std/core/logger.h>

#include "defines.h"
#include "vulkan_types.h"
#include "physical_device.h"
#include "device.h"
#include "vulkan_instance.h"

typedef struct SwapchainDetails {
    VkSwapchainKHR swapchain;
    VkImage *images;
    VkImageView *image_views;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *present_modes;

    u32 selected_format;
    u32 selected_mode;
} SwapchainDetails;

typedef struct VulkanContext {
    VulkanInstance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physical_device;
    Device device;
    SwapchainDetails swapchain_details;
    VkExtent2D extent;
} VulkanContext;

bool initVulkan(SDL_Window *window, const char *app_name);

void shutdownVulkan();