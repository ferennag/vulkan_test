#pragma once

#include "defines.h"
#include "vulkan_types.h"
#include "physical_device.h"

typedef enum QueueFeature {
    QUEUE_FEATURE_GRAPHICS,
    QUEUE_FEATURE_PRESENT,
    QUEUE_FEATURE_COMPUTE,
    QUEUE_FEATURE_SPARSE_BINDING,
    QUEUE_FEATURE_TRANSFER,
    QUEUE_FEATURE_VIDEO_ENCODE,
    QUEUE_FEATURE_VIDEO_DECODE,
    QUEUE_FEATURE_MAX
} QueueFeature;

typedef struct QueueFamily {
    u32 index;
    bool features[QUEUE_FEATURE_MAX];
} QueueFamily;

typedef struct Queue {
    QueueFamily *queue_family;
    VkQueue vk_queue;
} Queue;

typedef struct Device {
    VkDevice device;
    QueueFamily *queue_families;
    Queue queues[QUEUE_FEATURE_MAX];
} Device;

bool device_create(PhysicalDevice *physical_device, VkSurfaceKHR *surface, Device *out);

bool device_queue_available(Device *device, QueueFeature feature);

void device_destroy(Device *device);
