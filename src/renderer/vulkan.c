#include "renderer/vulkan.h"
#include <SDL_vulkan.h>
#include "std/containers/darray.h"

static VulkanContext context = {0};

void query_available_device_extensions() {
    u32 count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(context.physicalDevice, NULL, &count, NULL));
    VkExtensionProperties *properties = darray_reserve(VkExtensionProperties, count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(context.physicalDevice, NULL, &count, properties));

    const char **names = darray_reserve(const char *, count);
    for (int i = 0; i < count; ++i) {
        LOG_INFO("Found supported device extension: %s", properties[i].extensionName);
        names[i] = properties[i].extensionName;
    }

    darray_destroy(properties);
    context.available_device_extensions = names;
}

bool is_device_extension_available(const char *name) {
    for (int i = 0; i < darray_length(context.available_device_extensions); ++i) {
        if (strcmp(name, context.available_device_extensions[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool check_validation_layers(const char **requested) {
    u32 layer_count;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, NULL));
    VkLayerProperties *layer_props = darray_reserve(VkLayerProperties, layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layer_props));

    u32 not_found = 0;

    for (u32 j = 0; j < darray_length(requested); ++j) {
        bool found = false;
        for (u32 i = 0; i < darray_length(layer_props); ++i) {
            if (strcmp(requested[j], layer_props[i].layerName) == 0) {
                LOG_INFO("Requested layer found: %s", requested[j]);
                found = true;
                break;
            }
        }

        if (!found) {
            LOG_ERROR("Requested layer not found: %s", requested[j]);
            ++not_found;
        }
    }

    darray_destroy(layer_props);
    return not_found == 0;
}

bool query_swapchain_details() {
    SwapchainDetails details = {};

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface,
                                                       &details.surface_capabilities));

    u32 format_count;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &format_count, NULL));
    details.formats = darray_reserve(VkSurfaceFormatKHR, format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &format_count,
                                                  details.formats));

    u32 mode_count;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &mode_count, NULL));
    details.present_modes = darray_reserve(VkPresentModeKHR, mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &mode_count,
                                                       details.present_modes));

    if (darray_length(details.formats) == 0) {
        LOG_ERROR("No present formats available for swapchain!");
        return false;
    }

    if (darray_length(details.present_modes) == 0) {
        LOG_ERROR("No present modes available for swapchain!");
        return false;
    }

    context.swapchain_details = details;
    return true;
}

VkExtent2D choose_swap_extent(SDL_Window *window) {
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D extent = {
            .width = width,
            .height = height
    };
    return extent;
}

bool init_swapchain(SDL_Window *window) {
    if (!query_swapchain_details()) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    // Select format
    context.swapchain_details.selected_format = UINT32_MAX;
    for (int i = 0; i < darray_length(context.swapchain_details.formats); ++i) {
        VkSurfaceFormatKHR format = context.swapchain_details.formats[i];

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            context.swapchain_details.selected_format = i;
            LOG_INFO("Selecting swapchain surface format: %s - %s", string_VkFormat(format.format), string_VkColorSpaceKHR(format.colorSpace));
            break;
        }
    }
    if (context.swapchain_details.selected_format == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain format!");
        return false;
    }

    // Select mode
    context.swapchain_details.selected_mode = UINT32_MAX;
    for (int i = 0; i < darray_length(context.swapchain_details.present_modes); ++i) {
        VkPresentModeKHR mode = context.swapchain_details.present_modes[i];

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR || mode == VK_PRESENT_MODE_FIFO_KHR) {
            context.swapchain_details.selected_mode = i;
            LOG_INFO("Selecting swapchain presentation format: %s", string_VkPresentModeKHR(mode));
            break;
        }
    }
    if (context.swapchain_details.selected_mode == UINT32_MAX) {
        LOG_ERROR("Couldn't find a suitable swapchain presentation mode!");
        return false;
    }

    u32 min_image_count = context.swapchain_details.surface_capabilities.minImageCount;
    u32 max_image_count = context.swapchain_details.surface_capabilities.maxImageCount;
    VkExtent2D extent = choose_swap_extent(window);

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.surface = context.surface;
    create_info.minImageCount = min_image_count + 1;
    if (max_image_count > 0 && create_info.minImageCount > max_image_count) {
        create_info.minImageCount = max_image_count;
    }
    create_info.imageFormat = context.swapchain_details.formats[context.swapchain_details.selected_format].format;
    create_info.imageColorSpace = context.swapchain_details.formats[context.swapchain_details.selected_format].colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 queue_family_indices[] = {context.queues.graphicsIndex, context.queues.presentIndex};
    if (context.queues.graphicsIndex != context.queues.presentIndex) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    create_info.preTransform = context.swapchain_details.surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = context.swapchain_details.present_modes[context.swapchain_details.selected_mode];
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(context.device, &create_info, NULL, &context.swapchain_details.swapchain));

    u32 image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain_details.swapchain, &image_count, NULL));
    VkImage *images = darray_reserve(VkImage, image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain_details.swapchain, &image_count, images));
    context.swapchain_details.images = images;
    context.extent = extent;

    return true;
}

bool create_image_views() {
    VkImageView *image_views = darray_reserve(VkImageView, darray_length(context.swapchain_details.images));

    for (int i = 0; i < darray_length(context.swapchain_details.images); ++i) {
        VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        create_info.image = context.swapchain_details.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = context.swapchain_details.formats[context.swapchain_details.selected_format].format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context.device, &create_info, NULL, &image_views[i]));
    }

    context.swapchain_details.image_views = image_views;

    return true;
}

b8 pickPhysicalDevice(VulkanContext *context) {
    u32 deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, 0))
    VkPhysicalDevice *devices = darray_reserve(VkPhysicalDevice, deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices))

    VkPhysicalDevice *discrete_gpu = 0;
    VkPhysicalDevice *integrated_gpu = 0;
    for (u32 i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            LOG_INFO("Found Discreet GPU: %d - %s", properties.vendorID, properties.deviceName);
            discrete_gpu = &devices[i];
        } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            LOG_INFO("Found Integrated GPU: %d - %s", properties.vendorID, properties.deviceName);
            integrated_gpu = &devices[i];
        }
    }

    if (discrete_gpu) {
        context->physicalDevice = *discrete_gpu;
    } else if (integrated_gpu) {
        context->physicalDevice = *integrated_gpu;
    }

    darray_destroy(devices)
    return context->physicalDevice != 0;
}

b8 createDevice(VulkanContext *context) {
    VulkanQueues queues = {
            .graphicsIndex = UINT32_MAX,
            .presentIndex = UINT32_MAX,
            .graphicsQueue = 0,
            .presentQueue = 0,
    };

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &queueFamilyCount, 0);
    VkQueueFamilyProperties *queueFamilies = darray_reserve(VkQueueFamilyProperties, queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &queueFamilyCount, queueFamilies);

    u32 *queue_indexes = darray_create(u32);
    for (u32 i = 0; i < queueFamilyCount; ++i) {
        LOG_TRACE("Checking queue family index: %d", i)
        if (queues.graphicsIndex == UINT32_MAX && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            LOG_TRACE("Found Graphics queue: %d", i)
            queues.graphicsIndex = i;
            darray_push(queue_indexes, i);
        }

        if (queues.presentIndex == UINT32_MAX) {
            VkBool32 supported;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, i, context->surface, &supported))
            if (supported) {
                LOG_TRACE("Found Present queue: %d", i)
                queues.presentIndex = i;
                if (queues.graphicsIndex != queues.presentIndex) {
                    darray_push(queue_indexes, i);
                }
            }
        }

        if (queues.graphicsIndex != UINT32_MAX && queues.presentIndex != UINT32_MAX) {
            break;
        }
    }
    context->queues = queues;

    if (context->queues.graphicsIndex == UINT32_MAX || context->queues.presentIndex == UINT32_MAX) {
        return FALSE;
    }

    VkDeviceQueueCreateInfo *queue_create_infos = darray_create(VkDeviceQueueCreateInfo);
    float priority = 1.0f;
    for (int i = 0; i < darray_length(queue_indexes); ++i) {
        VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = queue_indexes[i];
        darray_push(queue_create_infos, queueCreateInfo);
    }

    query_available_device_extensions();
    const char **extensions = darray_create(const char *);
    if (!is_device_extension_available(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        LOG_ERROR("Vulkan Swapchain extension unavailable: %s", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        return FALSE;
    }
    darray_push(extensions, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (is_device_extension_available("VK_KHR_portability_subset")) {
        darray_push(extensions, &"VK_KHR_portability_subset");
    }

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = darray_length(queue_create_infos);
    createInfo.pQueueCreateInfos = queue_create_infos;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = darray_length(extensions);
    createInfo.pEnabledFeatures = &features;

    VK_CHECK(vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device))

    vkGetDeviceQueue(context->device, context->queues.graphicsIndex, 0, &context->queues.graphicsQueue);
    vkGetDeviceQueue(context->device, context->queues.presentIndex, 0, &context->queues.presentQueue);

    darray_destroy(queueFamilies);
    darray_destroy(queue_indexes);
    darray_destroy(queue_create_infos);
    darray_destroy(extensions);

    LOG_INFO("Successfully initialized Vulkan device.");
    return TRUE;
}

bool initVulkan(SDL_Window *window, const char *app_name) {
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = app_name;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VulkanDemoEngine";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    u32 sdl_extension_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, NULL);
    const char **extensions = darray_reserve(const char *, sdl_extension_count);
    SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, extensions);
    darray_push(extensions, &VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    const char **layers = darray_create(const char *);
    darray_push(layers, &"VK_LAYER_KHRONOS_validation");
    check_validation_layers(layers);

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = darray_length(extensions);
    instanceCreateInfo.ppEnabledExtensionNames = extensions;
    instanceCreateInfo.enabledLayerCount = darray_length(layers);
    instanceCreateInfo.ppEnabledLayerNames = layers;
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, 0, &context.instance));
    if (!SDL_Vulkan_CreateSurface(window, context.instance, &context.surface)) {
        LOG_ERROR("Unable to create Vulkan surface with SDL: %s", SDL_GetError());
        return false;
    }

    darray_destroy(extensions);
    darray_destroy(layers);

    if (!pickPhysicalDevice(&context)) {
        LOG_ERROR("Couldn't find a suitable GPU!");
        return false;
    }

    if (!createDevice(&context)) {
        LOG_ERROR("Couldn't create a logical device!");
        return false;
    }

    if (!init_swapchain(window)) {
        LOG_ERROR("Couldn't create a swapchain!");
        return false;
    }

    if (!create_image_views()) {
        LOG_ERROR("Couldn't create image views for the swapchain!");
        return false;
    }

    return true;
}

void initSwapChain() {

}

void shutdownVulkan() {
    for (int i = 0; i < darray_length(context.swapchain_details.image_views); ++i) {
        vkDestroyImageView(context.device, context.swapchain_details.image_views[i], NULL);
    }

    darray_destroy(context.swapchain_details.image_views);
    darray_destroy(context.available_device_extensions);
    darray_destroy(context.swapchain_details.images);
    vkDestroySwapchainKHR(context.device, context.swapchain_details.swapchain, 0);
    vkDestroySurfaceKHR(context.instance, context.surface, 0);
    vkDestroyDevice(context.device, 0);
    vkDestroyInstance(context.instance, 0);
}