#include "command_pool.h"

void command_pool_create(VulkanContext *context) {
    VkCommandPoolCreateInfo create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = context->device.queues[QUEUE_FEATURE_GRAPHICS].queue_family->index;

    VK_CHECK(vkCreateCommandPool(context->device.vk_device, &create_info, NULL, &context->command_pool));
}

void command_pool_destroy(VulkanContext *context) {
    vkDestroyCommandPool(context->device.vk_device, context->command_pool, NULL);
    context->command_pool = NULL;
}