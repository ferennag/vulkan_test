#include "shader.h"
#include <std/core/file.h>
#include <std/core/logger.h>
#include <std/core/memory.h>

void shader_create_module(Device *device, BinaryContents *code, VkShaderModule *out) {
    VkShaderModuleCreateInfo create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create_info.codeSize = code->size;
    create_info.pCode = code->data;

    VK_CHECK(vkCreateShaderModule(device->vk_device, &create_info, NULL, out));
}

bool shader_load(Device *device, const char *vertex, const char *fragment, Shader *out) {
    BinaryContents vertex_binary = {0};
    BinaryContents fragment_binary = {0};
    if (!file_read_binary(vertex, &vertex_binary)) {
        LOG_ERROR("Failed to load vertex shader: %s", vertex);
        return false;
    }

    if (!file_read_binary(fragment, &fragment_binary)) {
        LOG_ERROR("Failed to load fragment shader: %s", fragment);
        return false;
    }

    shader_create_module(device, &vertex_binary, &out->vertex);
    shader_create_module(device, &fragment_binary, &out->fragment);

    return true;
}

void shader_destroy(Device *device, Shader *shader) {
    vkDestroyShaderModule(device->vk_device, shader->vertex, NULL);
    shader->vertex = NULL;
    vkDestroyShaderModule(device->vk_device, shader->fragment, NULL);
    shader->fragment = NULL;
}