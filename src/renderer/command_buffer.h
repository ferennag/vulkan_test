#pragma once

#include "vulkan_types.h"
#include "vulkan.h"

void command_buffer_create(VulkanContext *context);

void command_buffer_destroy(VulkanContext *context);

void command_buffer_begin(VulkanContext *context);

void command_buffer_end(VulkanContext *context);