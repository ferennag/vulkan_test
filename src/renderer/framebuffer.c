#include "framebuffer.h"
#include <std/containers/darray.h>


bool framebuffer_create(VulkanContext *context) {
    context->framebuffers = darray_create(VkFramebuffer);

    for (int i = 0; i < darray_length(context->swapchain.image_views); ++i) {
        VkImageView *image_view = &context->swapchain.image_views[i];

        VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        create_info.pAttachments = image_view;
        create_info.attachmentCount = 1;
        create_info.renderPass = context->graphics_pipeline.render_pass;
        create_info.width = context->swapchain.extent.width;
        create_info.height = context->swapchain.extent.height;
        create_info.layers = 1;

        VkFramebuffer framebuffer;
        VK_CHECK(vkCreateFramebuffer(context->device.vk_device, &create_info, NULL, &framebuffer));
        darray_push(context->framebuffers, framebuffer);
    }

    return true;
}

void framebuffer_destroy(VulkanContext *context) {
    for (int i = 0; i < darray_length(context->framebuffers); ++i) {
        vkDestroyFramebuffer(context->device.vk_device, context->framebuffers[i], NULL);
    }

    darray_destroy(context->framebuffers)
    context->framebuffers = NULL;
}