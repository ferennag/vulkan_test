#include "command_buffer.h"
#include "renderer_instance.h"

void command_buffer_create(VulkanContext *context, RendererInstance *out) {
    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = context->command_pool;
    allocate_info.commandBufferCount = 1;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(context->device.vk_device, &allocate_info, &out->command_buffer));
}

void command_buffer_destroy(RendererInstance *instance) {
    instance->command_buffer = NULL;
}

void command_buffer_begin(RendererInstance *instance) {
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(instance->command_buffer, &begin_info));
}

void command_buffer_end(RendererInstance *instance) {
    VK_CHECK(vkEndCommandBuffer(instance->command_buffer));
}