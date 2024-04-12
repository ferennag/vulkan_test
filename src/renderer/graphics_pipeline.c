#include "graphics_pipeline.h"
#include "vulkan.h"
#include <std/containers/darray.h>

void render_pass_create(Device *device, Swapchain *swapchain, VkRenderPass *render_pass) {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = swapchain->formats[swapchain->selected_format].format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(device->vk_device, &render_pass_create_info, NULL, render_pass));
}

bool graphics_pipeline_create(Device *device, Swapchain *swapchain, GraphicsPipeline *out) {
    render_pass_create(device, swapchain, &out->render_pass);

    Shader shader = {0};
    shader_load(device, "vertex.vert.spv", "fragment.frag.spv", &shader);

    VkPipelineShaderStageCreateInfo vertex_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertex_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_create_info.module = shader.vertex;
    vertex_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragment_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_create_info.module = shader.fragment;
    fragment_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_create_info, fragment_create_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_create_info.pVertexAttributeDescriptions = NULL;
    vertex_input_create_info.vertexBindingDescriptionCount = 0;
    vertex_input_create_info.pVertexBindingDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {
            .x = 0,
            .y = 0,
            .width = (float) swapchain->extent.width,
            .height = (float) swapchain->extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };

    VkRect2D scissor = {
            .offset = {0, 0},
            .extent = swapchain->extent
    };

    VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raterization_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    raterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    raterization_create_info.lineWidth = 1.0f;
    raterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    raterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raterization_create_info.depthBiasEnable = VK_FALSE;
    raterization_create_info.depthBiasConstantFactor = 0.0f;
    raterization_create_info.depthBiasClamp = 0.0f;
    raterization_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_create_info.sampleShadingEnable = VK_FALSE;
    multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_create_info.minSampleShading = 1.0f;
    multisample_create_info.pSampleMask = NULL;
    multisample_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {0};
    color_blend_attachment_state.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_create_info.blendConstants[0] = 0;
    color_blend_create_info.blendConstants[1] = 0;
    color_blend_create_info.blendConstants[2] = 0;
    color_blend_create_info.blendConstants[3] = 0;

    VkPipelineLayoutCreateInfo layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_create_info.setLayoutCount = 0;

    VK_CHECK(vkCreatePipelineLayout(device->vk_device, &layout_create_info, NULL, &out->layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &raterization_create_info;
    pipeline_create_info.pMultisampleState = &multisample_create_info;
    pipeline_create_info.pDepthStencilState = NULL;
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = out->layout;
    pipeline_create_info.renderPass = out->render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = NULL;
    pipeline_create_info.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(device->vk_device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &out->vk_pipeline));

    shader_destroy(device, &shader);
    return true;
}

void graphics_pipeline_destroy(Device *device, GraphicsPipeline *pipeline) {
    vkDestroyRenderPass(device->vk_device, pipeline->render_pass, NULL);
    pipeline->render_pass = NULL;

    vkDestroyPipeline(device->vk_device, pipeline->vk_pipeline, NULL);
    pipeline->vk_pipeline = NULL;

    vkDestroyPipelineLayout(device->vk_device, pipeline->layout, NULL);
    pipeline->layout = NULL;
}

void render_pass_begin(VulkanContext *context, u32 image_index) {
    VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin_info.renderPass = context->graphics_pipeline.render_pass;
    begin_info.framebuffer = context->framebuffers[image_index];
    begin_info.renderArea.offset.x = 0;
    begin_info.renderArea.offset.y = 0;
    begin_info.renderArea.extent = context->swapchain.extent;

    VkClearValue clear_color = {.color = {0.0, 0.0, 0.0, 1.0}};
    begin_info.clearValueCount = 1;
    begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(context->current_renderer->command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void render_pass_end(VulkanContext *context) {
    vkCmdEndRenderPass(context->current_renderer->command_buffer);
}

void bind_pipeline(VulkanContext *context) {
    vkCmdBindPipeline(context->current_renderer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipeline.vk_pipeline);
}