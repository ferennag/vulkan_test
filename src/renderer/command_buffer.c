#include "command_buffer.h"

void command_buffer_create(VulkanContext *context) {
    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = context->command_pool;
    allocate_info.commandBufferCount = 1;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(context->device.vk_device, &allocate_info, &context->command_buffer));
}

void command_buffer_destroy(VulkanContext *context) {
    context->command_buffer = NULL;
}

void command_buffer_begin(VulkanContext *context) {
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(context->command_buffer, &begin_info));
}

void command_buffer_end(VulkanContext *context) {
    VK_CHECK(vkEndCommandBuffer(context->command_buffer));
}