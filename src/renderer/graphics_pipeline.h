#pragma once

#include <std/defines.h>
#include "vulkan_types.h"
#include "device.h"
#include "shader.h"
#include "swapchain.h"

typedef struct GraphicsPipeline {
    VkRenderPass render_pass;
    VkPipeline vk_pipeline;
    VkPipelineLayout layout;
} GraphicsPipeline;

bool graphics_pipeline_create(Device *device, Swapchain *swapchain, GraphicsPipeline *out);

void graphics_pipeline_destroy(Device *device, GraphicsPipeline *pipeline);