#include "vulkan_instance.h"

#include <SDL_vulkan.h>
#include <std/containers/darray.h>

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

bool vulkan_instance_create(SDL_Window *window, const char *app_name, VulkanInstance *out) {
    u32 api_version = 0;
    vkEnumerateInstanceVersion(&api_version);
    LOG_INFO("Vulkan version: %d.%d.%d.%d", VK_API_VERSION_VARIANT(api_version), VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version), VK_API_VERSION_PATCH(api_version));

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

    VulkanInstance instance = {0};
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, 0, &instance.vk_instance));
    *out = instance;

    darray_destroy(extensions);
    darray_destroy(layers);
    return true;
}

void vulkan_instance_destroy(VulkanInstance *instance) {
    vkDestroyInstance(instance->vk_instance, NULL);
    instance->vk_instance = NULL;
}