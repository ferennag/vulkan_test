#include "renderer/vulkan.h"
#include "shader.h"
#include "framebuffer.h"
#include "command_pool.h"
#include "command_buffer.h"
#include <std/containers/darray.h>
#include <SDL_vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

static VulkanContext context = {0};

bool create_device(VulkanContext *context) {
    device_create(&context->physical_device, &context->surface, &context->device);

    if (!device_queue_available(&context->device, QUEUE_FEATURE_GRAPHICS)) {
        LOG_ERROR("Graphics queue not available!");
        return false;
    }

    if (!device_queue_available(&context->device, QUEUE_FEATURE_PRESENT)) {
        LOG_ERROR("Present queue not available!");
        return false;
    }

    return true;
}

bool recreate_swap_chain(SDL_Window *window) {
    if (context.swapchain.vk_swapchain != NULL) {
        framebuffer_destroy(&context);
        swapchain_destroy(&context.device, &context.swapchain);
    }

    if (!swapchain_init(window, &context.physical_device, &context.device, &context.surface, &context.swapchain)) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    if (context.graphics_pipeline.vk_pipeline == NULL) {
        if (!graphics_pipeline_create(&context.device, &context.swapchain, &context.graphics_pipeline)) {
            LOG_ERROR("Couldn't create graphics vk_pipeline!");
            return false;
        }
    }

    if (!framebuffer_create(&context)) {
        LOG_ERROR("Couldn't create graphics vk_pipeline!");
        return false;
    }

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

    if (!recreate_swap_chain(window)) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    const int max_renderers = 3;
    command_pool_create(&context);
    renderer_instance_create(&context, max_renderers);

    return true;
}

void vulkan_shutdown() {
    renderer_instance_destroy(&context);
    command_pool_destroy(&context);
    framebuffer_destroy(&context);
    graphics_pipeline_destroy(&context.device, &context.graphics_pipeline);
    physical_device_destroy(&context.physical_device);
    swapchain_destroy(&context.device, &context.swapchain);
    device_destroy(&context.device);
    vkDestroySurfaceKHR(context.instance.vk_instance, context.surface, NULL);
    vulkan_instance_destroy(&context.instance);
}

void begin_frame(u32 image_index) {
    command_buffer_begin(context.current_renderer);
    render_pass_begin(&context, image_index);
    bind_pipeline(&context);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = context.swapchain.extent.width;
    viewport.height = context.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(context.current_renderer->command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = context.swapchain.extent;
    vkCmdSetScissor(context.current_renderer->command_buffer, 0, 1, &scissor);
}

void end_frame(u32 image_index) {
    render_pass_end(&context);
    command_buffer_end(context.current_renderer);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    VkSemaphore semaphores[] = {context.current_renderer->image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = sizeof(semaphores) / sizeof(VkSemaphore);
    submit_info.pWaitSemaphores = semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context.current_renderer->command_buffer;

    VkSemaphore signal_semaphores[] = {context.current_renderer->render_finished_semaphore};
    submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(VkSemaphore);
    submit_info.pSignalSemaphores = signal_semaphores;

    VkQueue graphics_queue = context.device.queues[QUEUE_FEATURE_GRAPHICS].vk_queue;
    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, context.current_renderer->in_flight_fence));

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context.swapchain.vk_swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = NULL;
    VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));
}

void vulkan_render() {
    context.current_renderer = &context.renderer_instances[context.current_renderer_index];

    // Wait for the previous frame to finish
    VK_CHECK(vkWaitForFences(context.device.vk_device, 1, &context.current_renderer->in_flight_fence, VK_TRUE,
                             UINT64_MAX));
    VK_CHECK(vkResetFences(context.device.vk_device, 1, &context.current_renderer->in_flight_fence));

    u32 image_index = 0;
    VK_CHECK(vkAcquireNextImageKHR(context.device.vk_device, context.swapchain.vk_swapchain, UINT64_MAX,
                                   context.current_renderer->image_available_semaphore, VK_NULL_HANDLE, &image_index));
    vkResetCommandBuffer(context.current_renderer->command_buffer, 0);

    begin_frame(image_index);
    vkCmdDraw(context.current_renderer->command_buffer, 3, 1, 0, 0);
    end_frame(image_index);
    context.current_renderer_index =
            (context.current_renderer_index + 1) % darray_length(context.renderer_instances);
}

void vulkan_window_resized(SDL_Window *window) {
    vkDeviceWaitIdle(context.device.vk_device);
    recreate_swap_chain(window);
}