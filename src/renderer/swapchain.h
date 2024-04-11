#pragma once

#include "defines.h"
#include "vulkan_types.h"
#include "physical_device.h"
#include "device.h"
#include <SDL.h>

typedef struct Swapchain {
    VkSwapchainKHR vk_swapchain;
    VkImage *images;
    VkImageView *image_views;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *present_modes;

    VkExtent2D extent;

    u32 selected_format;
    u32 selected_mode;
} Swapchain;

bool swapchain_init(SDL_Window *window, PhysicalDevice *physicalDevice, Device *device, VkSurfaceKHR *surface, Swapchain *out);

void swapchain_destroy(Device *device, Swapchain *swapchain);