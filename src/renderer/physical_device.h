#pragma once

#include <std/defines.h>
#include <vulkan/vulkan.h>

typedef struct PhysicalDevice {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;

    const char **available_extensions;
} PhysicalDevice;

bool physical_device_select_best(VkInstance instance, PhysicalDevice *out);

bool physical_device_is_extension_available(PhysicalDevice *physical_device, const char *name);

void physical_device_destroy(PhysicalDevice *physical_device);

