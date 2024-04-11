#include "device.h"

#include <std/containers/darray.h>
#include <std/core/logger.h>

int queue_family_compare(const void *a, const void *b) {
    return (int) ((QueueFamily *) a)->index - (int) ((QueueFamily *) b)->index;
}

QueueFamily *device_get_queue_families(PhysicalDevice *physical_device, VkSurfaceKHR *surface) {
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device->device, &queue_family_count, 0);
    VkQueueFamilyProperties *queue_families = darray_reserve(VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device->device, &queue_family_count, queue_families);

    QueueFamily *result = darray_create(QueueFamily);
    for (u32 i = 0; i < darray_length(queue_families); ++i) {
        QueueFamily family = {.index = i};

        family.features[QUEUE_FEATURE_GRAPHICS] = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        family.features[QUEUE_FEATURE_COMPUTE] = queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        family.features[QUEUE_FEATURE_SPARSE_BINDING] = queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
        family.features[QUEUE_FEATURE_TRANSFER] = queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
        family.features[QUEUE_FEATURE_VIDEO_ENCODE] = queue_families[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
        family.features[QUEUE_FEATURE_VIDEO_DECODE] = queue_families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;

        VkBool32 supported;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device->device, i, *surface, &supported))
        family.features[QUEUE_FEATURE_PRESENT] = supported;
        darray_push(result, family)
    }

    darray_destroy(queue_families);
    qsort(result, darray_length(result), sizeof(QueueFamily), queue_family_compare);
    return result;
}

QueueFamily *device_find_queue_family(Device *device, QueueFeature feature) {
    for (int i = 0; i < darray_length(device->queue_families); ++i) {
        QueueFamily *family = &device->queue_families[i];

        if (family->features[feature]) {
            return family;
        }
    }

    return NULL;
}

VkDeviceQueueCreateInfo *device_queue_create_infos(Device *device) {
    // TODO a hashmap would be nice here

    // Collect unique queue indexes
    u32 *unique_indexes = darray_create(u32);
    for (int i = 0; i < QUEUE_FEATURE_MAX; ++i) {
        Queue queue = device->queues[i];
        if (queue.queue_family == NULL) {
            continue;
        }

        if (darray_length(unique_indexes) == 0) {
            darray_push(unique_indexes, queue.queue_family->index);
        } else if (unique_indexes[darray_length(unique_indexes) - 1] != queue.queue_family->index) {
            darray_push(unique_indexes, queue.queue_family->index);
        }
    }

    VkDeviceQueueCreateInfo *queue_create_infos = darray_create(VkDeviceQueueCreateInfo);
    float priority = 1.0f;
    for (int i = 0; i < darray_length(unique_indexes); ++i) {
        VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = unique_indexes[i];
        darray_push(queue_create_infos, queueCreateInfo);
    }

    darray_destroy(unique_indexes);
    return queue_create_infos;
}

bool device_create(PhysicalDevice *physical_device, VkSurfaceKHR *surface, Device *out) {
    Device result = {0};
    QueueFamily *queue_families = device_get_queue_families(physical_device, surface);
    result.queue_families = queue_families;

    for (int i = 0; i < QUEUE_FEATURE_MAX; ++i) {
        QueueFeature feature = (QueueFeature) i;
        QueueFamily *family = device_find_queue_family(&result, feature);
        result.queues[i].queue_family = family;
    }

    VkDeviceQueueCreateInfo *queue_create_infos = device_queue_create_infos(&result);

    const char **extensions = darray_create(const char *);
    if (!physical_device_is_extension_available(physical_device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        LOG_ERROR("Vulkan Swapchain extension unavailable: %s", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        return FALSE;
    }
    darray_push(extensions, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (physical_device_is_extension_available(physical_device, "VK_KHR_portability_subset")) {
        darray_push(extensions, &"VK_KHR_portability_subset");
    }

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = darray_length(queue_create_infos);
    createInfo.pQueueCreateInfos = queue_create_infos;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = darray_length(extensions);
    createInfo.pEnabledFeatures = &features;

    VkDevice device;
    VK_CHECK(vkCreateDevice(physical_device->device, &createInfo, 0, &device))
    result.device = device;

    for (int i = 0; i < QUEUE_FEATURE_MAX; ++i) {
        if (result.queues[i].queue_family != NULL) {
            VkQueue vk_queue;
            vkGetDeviceQueue(result.device, result.queues[i].queue_family->index, 0, &vk_queue);
            result.queues[i].vk_queue = vk_queue;
        }
    }

    darray_destroy(queue_create_infos);
    darray_destroy(extensions);

    *out = result;
    LOG_INFO("Successfully initialized Vulkan device.");
    return TRUE;
}

bool device_queue_available(Device *device, QueueFeature feature) {
    return device->queues[feature].vk_queue != NULL;
}

void device_destroy(Device *device) {
    darray_destroy(device->queue_families);
    device->queue_families = 0;

    vkDestroyDevice(device->device, NULL);
    device->device = 0;
}