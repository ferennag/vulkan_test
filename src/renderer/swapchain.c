#include "swapchain.h"

#include <SDL_vulkan.h>
#include <std/containers/darray.h>

bool query_swapchain_details(PhysicalDevice *physical_device, VkSurfaceKHR *surface, Swapchain *out) {
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->device, *surface,
                                                       &out->surface_capabilities));

    u32 format_count;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->device, *surface, &format_count, NULL));
    out->formats = darray_reserve(VkSurfaceFormatKHR, format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->device, *surface, &format_count,
                                                  out->formats));

    u32 mode_count;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->device, *surface, &mode_count, NULL));
    out->present_modes = darray_reserve(VkPresentModeKHR, mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->device, *surface, &mode_count,
                                                       out->present_modes));

    if (darray_length(out->formats) == 0) {
        LOG_ERROR("No present formats available for swapchain!");
        return false;
    }

    if (darray_length(out->present_modes) == 0) {
        LOG_ERROR("No present modes available for swapchain!");
        return false;
    }

    return true;
}

VkExtent2D swapchain_choose_swap_extent(SDL_Window *window) {
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D extent = {
            .width = width,
            .height = height
    };
    return extent;
}

void create_image_views(Device *device, Swapchain *swapchain) {
    VkImageView *image_views = darray_reserve(VkImageView, darray_length(swapchain->images));

    for (int i = 0; i < darray_length(swapchain->images); ++i) {
        VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        create_info.image = swapchain->images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain->formats[swapchain->selected_format].format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(device->device, &create_info, NULL, &image_views[i]));
    }

    swapchain->image_views = image_views;
}

bool swapchain_init(SDL_Window *window, PhysicalDevice *physicalDevice, Device *device, VkSurfaceKHR *surface,
                    Swapchain *out) {
    if (!query_swapchain_details(physicalDevice, surface, out)) {
        LOG_ERROR("Couldn't query swapchain details!");
        return false;
    }

    // Select format
    out->selected_format = UINT32_MAX;
    for (int i = 0; i < darray_length(out->formats); ++i) {
        VkSurfaceFormatKHR format = out->formats[i];

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out->selected_format = i;
            LOG_INFO("Selecting swapchain surface format: %s - %s", string_VkFormat(format.format),
                     string_VkColorSpaceKHR(format.colorSpace));
            break;
        }
    }
    if (out->selected_format == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain format!");
        return false;
    }

    // Select mode
    out->selected_mode = UINT32_MAX;
    for (int i = 0; i < darray_length(out->present_modes); ++i) {
        VkPresentModeKHR mode = out->present_modes[i];

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR || mode == VK_PRESENT_MODE_FIFO_KHR) {
            out->selected_mode = i;
            LOG_INFO("Selecting swapchain presentation format: %s", string_VkPresentModeKHR(mode));
            break;
        }
    }
    if (out->selected_mode == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain presentation mode!");
        return false;
    }

    u32 min_image_count = out->surface_capabilities.minImageCount;
    u32 max_image_count = out->surface_capabilities.maxImageCount;
    VkExtent2D extent = swapchain_choose_swap_extent(window);

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.surface = *surface;
    create_info.minImageCount = min_image_count + 1;
    if (max_image_count > 0 && create_info.minImageCount > max_image_count) {
        create_info.minImageCount = max_image_count;
    }
    create_info.imageFormat = out->formats[out->selected_format].format;
    create_info.imageColorSpace = out->formats[out->selected_format].colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 graphics_index = device->queues[QUEUE_FEATURE_GRAPHICS].queue_family->index;
    u32 present_index = device->queues[QUEUE_FEATURE_PRESENT].queue_family->index;
    u32 queue_family_indices[] = {graphics_index, present_index};

    if (graphics_index != present_index) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    create_info.preTransform = out->surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = out->present_modes[out->selected_mode];
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(device->device, &create_info, NULL, &out->vk_swapchain));

    u32 image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device->device, out->vk_swapchain, &image_count, NULL));
    VkImage *images = darray_reserve(VkImage, image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(device->device, out->vk_swapchain, &image_count, images));
    out->images = images;
    out->extent = extent;

    create_image_views(device, out);

    return true;
}

void swapchain_destroy(Device *device, Swapchain *swapchain) {
    for (int i = 0; i < darray_length(swapchain->image_views); ++i) {
        vkDestroyImageView(device->device, swapchain->image_views[i], NULL);
    }

    darray_destroy(swapchain->image_views);
    swapchain->image_views = NULL;
    darray_destroy(swapchain->images);
    swapchain->images = NULL;

    vkDestroySwapchainKHR(device->device, swapchain->vk_swapchain, NULL);
    swapchain->vk_swapchain = NULL;
}