#include "renderer_instance.h"
#include "vulkan.h"
#include "std/containers/darray.h"
#include "command_buffer.h"

void create_sync_objects(VulkanContext *context, RendererInstance *instance) {
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateSemaphore(context->device.vk_device, &semaphore_create_info, NULL,
                               &instance->image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(context->device.vk_device, &semaphore_create_info, NULL,
                               &instance->render_finished_semaphore));
    VK_CHECK(vkCreateFence(context->device.vk_device, &fence_create_info, NULL, &instance->in_flight_fence));
}

void renderer_instance_create(VulkanContext *context, u32 count) {
    context->renderer_instances = darray_create(RendererInstance);

    for (int i = 0; i < count; ++i) {
        RendererInstance instance = {0};

        create_sync_objects(context, &instance);
        command_buffer_create(context, &instance);

        darray_push(context->renderer_instances, instance);
    }
}

void renderer_instance_destroy(VulkanContext *context) {
    for (int i = 0; i < darray_length(context->renderer_instances); ++i) {
        RendererInstance *instance = &context->renderer_instances[i];
        VK_CHECK(vkWaitForFences(context->device.vk_device, 1, &instance->in_flight_fence, VK_TRUE, UINT64_MAX));

        vkDestroySemaphore(context->device.vk_device, instance->render_finished_semaphore, NULL);
        vkDestroySemaphore(context->device.vk_device, instance->image_available_semaphore, NULL);
        vkDestroyFence(context->device.vk_device, instance->in_flight_fence, NULL);
    }

    darray_destroy(context->renderer_instances);
    context->renderer_instances = NULL;
}