#include "renderer/vulkan.h"
#include <SDL_vulkan.h>
#include "std/containers/darray.h"
#include <vulkan/vk_enum_string_helper.h>

static VulkanContext context = {0};

bool query_swapchain_details() {
    SwapchainDetails details = {};

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device.device, context.surface,
                                                       &details.surface_capabilities));

    u32 format_count;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device.device, context.surface, &format_count, NULL));
    details.formats = darray_reserve(VkSurfaceFormatKHR, format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device.device, context.surface, &format_count,
                                                  details.formats));

    u32 mode_count;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device.device, context.surface, &mode_count, NULL));
    details.present_modes = darray_reserve(VkPresentModeKHR, mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device.device, context.surface, &mode_count,
                                                       details.present_modes));

    if (darray_length(details.formats) == 0) {
        LOG_ERROR("No present formats available for swapchain!");
        return false;
    }

    if (darray_length(details.present_modes) == 0) {
        LOG_ERROR("No present modes available for swapchain!");
        return false;
    }

    context.swapchain_details = details;
    return true;
}

VkExtent2D choose_swap_extent(SDL_Window *window) {
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D extent = {
            .width = width,
            .height = height
    };
    return extent;
}

bool init_swapchain(SDL_Window *window) {
    if (!query_swapchain_details()) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    // Select format
    context.swapchain_details.selected_format = UINT32_MAX;
    for (int i = 0; i < darray_length(context.swapchain_details.formats); ++i) {
        VkSurfaceFormatKHR format = context.swapchain_details.formats[i];

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            context.swapchain_details.selected_format = i;
            LOG_INFO("Selecting swapchain surface format: %s - %s", string_VkFormat(format.format),
                     string_VkColorSpaceKHR(format.colorSpace));
            break;
        }
    }
    if (context.swapchain_details.selected_format == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain format!");
        return false;
    }

    // Select mode
    context.swapchain_details.selected_mode = UINT32_MAX;
    for (int i = 0; i < darray_length(context.swapchain_details.present_modes); ++i) {
        VkPresentModeKHR mode = context.swapchain_details.present_modes[i];

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR || mode == VK_PRESENT_MODE_FIFO_KHR) {
            context.swapchain_details.selected_mode = i;
            LOG_INFO("Selecting swapchain presentation format: %s", string_VkPresentModeKHR(mode));
            break;
        }
    }
    if (context.swapchain_details.selected_mode == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain presentation mode!");
        return false;
    }

    u32 min_image_count = context.swapchain_details.surface_capabilities.minImageCount;
    u32 max_image_count = context.swapchain_details.surface_capabilities.maxImageCount;
    VkExtent2D extent = choose_swap_extent(window);

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.surface = context.surface;
    create_info.minImageCount = min_image_count + 1;
    if (max_image_count > 0 && create_info.minImageCount > max_image_count) {
        create_info.minImageCount = max_image_count;
    }
    create_info.imageFormat = context.swapchain_details.formats[context.swapchain_details.selected_format].format;
    create_info.imageColorSpace = context.swapchain_details.formats[context.swapchain_details.selected_format].colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 graphics_index = context.device.queues[QUEUE_FEATURE_GRAPHICS].queue_family->index;
    u32 present_index = context.device.queues[QUEUE_FEATURE_PRESENT].queue_family->index;
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

    create_info.preTransform = context.swapchain_details.surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = context.swapchain_details.present_modes[context.swapchain_details.selected_mode];
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(context.device.device, &create_info, NULL, &context.swapchain_details.swapchain));

    u32 image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context.device.device, context.swapchain_details.swapchain, &image_count, NULL));
    VkImage *images = darray_reserve(VkImage, image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(context.device.device, context.swapchain_details.swapchain, &image_count, images));
    context.swapchain_details.images = images;
    context.extent = extent;

    return true;
}

bool create_image_views() {
    VkImageView *image_views = darray_reserve(VkImageView, darray_length(context.swapchain_details.images));

    for (int i = 0; i < darray_length(context.swapchain_details.images); ++i) {
        VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        create_info.image = context.swapchain_details.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = context.swapchain_details.formats[context.swapchain_details.selected_format].format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context.device.device, &create_info, NULL, &image_views[i]));
    }

    context.swapchain_details.image_views = image_views;

    return true;
}

b8 create_device(VulkanContext *context) {
    device_create(&context->physical_device, &context->surface, &context->device);

    if (!device_queue_available(&context->device, QUEUE_FEATURE_GRAPHICS)) {
        LOG_ERROR("Graphics queue not available!");
        return FALSE;
    }

    if (!device_queue_available(&context->device, QUEUE_FEATURE_PRESENT)){
        LOG_ERROR("Present queue not available!");
        return FALSE;
    }

    return TRUE;
}

bool create_graphics_pipeline() {
    return true;
}

bool initVulkan(SDL_Window *window, const char *app_name) {
    if (!vulkan_instance_create(window, app_name, &context.instance)) {
        LOG_ERROR("Unable to create Vulkan instance!");
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(window, context.instance.vk_instance, &context.surface)) {
        LOG_ERROR("Unable to create Vulkan surface with SDL: %s", SDL_GetError());
        return false;
    }

    if (!physical_device_select_best(context.instance.vk_instance, &context.physical_device)) {
        LOG_ERROR("Couldn't find a suitable GPU!");
        return false;
    }

    if (!create_device(&context)) {
        LOG_ERROR("Couldn't create a logical device!");
        return false;
    }

    if (!init_swapchain(window)) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    if (!create_image_views()) {
        LOG_ERROR("Couldn't create image views for the swapchain!");
        return false;
    }

    if (!create_graphics_pipeline()) {
        LOG_ERROR("Couldn't create graphics pipeline!");
        return false;
    }

    return true;
}

void shutdownVulkan() {
    for (int i = 0; i < darray_length(context.swapchain_details.image_views); ++i) {
        vkDestroyImageView(context.device.device, context.swapchain_details.image_views[i], NULL);
    }

    darray_destroy(context.swapchain_details.image_views);
    darray_destroy(context.swapchain_details.images);
    physical_device_destroy(&context.physical_device);
    vkDestroySwapchainKHR(context.device.device, context.swapchain_details.swapchain, NULL);
    device_destroy(&context.device);
    vkDestroySurfaceKHR(context.instance.vk_instance, context.surface, NULL);
    vulkan_instance_destroy(&context.instance);
}