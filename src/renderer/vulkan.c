#include "renderer/vulkan.h"
#include <SDL_vulkan.h>
#include "std/containers/darray.h"
#include <vulkan/vk_enum_string_helper.h>

static VulkanContext context = {0};

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

bool vulkan_init(SDL_Window *window, const char *app_name) {
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

    if (!swapchain_init(window, &context.physical_device, &context.device, &context.surface, &context.swapchain)) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    if (!create_graphics_pipeline()) {
        LOG_ERROR("Couldn't create graphics pipeline!");
        return false;
    }

    return true;
}

void vulkan_shutdown() {
    physical_device_destroy(&context.physical_device);
    swapchain_destroy(&context.device, &context.swapchain);
    device_destroy(&context.device);
    vkDestroySurfaceKHR(context.instance.vk_instance, context.surface, NULL);
    vulkan_instance_destroy(&context.instance);
}