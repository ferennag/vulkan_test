#pragma once

#include "defines.h"

#include <SDL.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <std/std.h>
#include <std/core/logger.h>

#define VK_CHECK(expr) {                                            \
    VkResult result = expr;                                         \
    if (result != VK_SUCCESS) {                                     \
        LOG_ERROR("Vulkan error: %s", string_VkResult(result));     \
        exit(-1);                                                   \
    }                                                               \
}

typedef struct VulkanQueues {
    u32 graphicsIndex;
    VkQueue graphicsQueue;

    u32 presentIndex;
    VkQueue presentQueue;
} VulkanQueues;

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
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    SwapchainDetails swapchain_details;
    VkExtent2D extent;

    const char **available_device_extensions;

    VulkanQueues queues;
} VulkanContext;

bool initVulkan(SDL_Window *window, const char *app_name);

void initSwapChain();

void shutdownVulkan();