#include <string.h>
#include <std/containers/darray.h>
#include <std/core/logger.h>
#include <std/core/memory.h>
#include "vulkan_types.h"
#include "physical_device.h"

u16 priority(PhysicalDevice *device) {
    switch (device->properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return 0;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 4;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 5;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 2;
        default:
            return 0;
    }
}

PhysicalDevice *query_physical_devices(VkInstance instance) {
    u32 deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, 0))
    VkPhysicalDevice *devices = darray_reserve(VkPhysicalDevice, deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices))

    PhysicalDevice *all = darray_reserve(PhysicalDevice, deviceCount);
    for (u32 i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        all[i].device = devices[i];
        all[i].properties = properties;

        LOG_INFO("Found physical device %s:  %d - %s", string_VkPhysicalDeviceType(properties.deviceType),
                 properties.vendorID, properties.deviceName);
    }

    darray_destroy(devices)
    return all;
}

const char **query_available_device_extensions(PhysicalDevice *physical_device) {
    u32 count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device->device, NULL, &count, NULL));
    VkExtensionProperties *properties = darray_reserve(VkExtensionProperties, count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device->device, NULL, &count, properties));

    const char **names = darray_reserve(const char *, count);
    for (int i = 0; i < count; ++i) {
        LOG_INFO("Found supported device extension: %s", properties[i].extensionName);
        names[i] = properties[i].extensionName;
    }

    darray_destroy(properties);
    return names;
}

bool physical_device_select_best(VkInstance instance, PhysicalDevice *out) {
    LOG_INFO("Querying physical devices...");
    PhysicalDevice *all = query_physical_devices(instance);
    PhysicalDevice *selected_device = NULL;
    for (int i = 0; i < darray_length(all); ++i) {
        if (selected_device == NULL || priority(&all[i]) > priority(selected_device)) {
            selected_device = &all[i];
        }
    }

    if (selected_device == NULL) {
        LOG_ERROR("Couldn't find a suitable physical device.");
        return false;
    }

    memory_copy(out, selected_device, sizeof(PhysicalDevice));
    out->available_extensions = query_available_device_extensions(out);

    LOG_INFO("Selected physical device %s:  %d - %s",
             string_VkPhysicalDeviceType(selected_device->properties.deviceType),
             selected_device->properties.vendorID, selected_device->properties.deviceName);

    darray_destroy(all)
    return true;
}

bool physical_device_is_extension_available(PhysicalDevice *physical_device, const char *name) {
    for (int i = 0; i < darray_length(physical_device->available_extensions); ++i) {
        if (strcmp(name, physical_device->available_extensions[i]) == 0) {
            return true;
        }
    }
    return false;
}

void physical_device_destroy(PhysicalDevice *physical_device) {
    if (physical_device->available_extensions) {
        darray_destroy(physical_device->available_extensions);
        physical_device->available_extensions = 0;
    }
}