#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <std/core/memory.h>
#include "core/input.h"
#include "renderer/vulkan.h"

bool processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
                return !handle_keyboard_event(&event.key, true);
            case SDL_KEYUP:
                return !handle_keyboard_event(&event.key, false);
        }
    }

    return true;
}

int main() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("ERROR: failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    if (SDL_Vulkan_LoadLibrary(0) < 0) {
        printf("ERROR: SDL failed to load vulkan: %s", SDL_GetError());
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Vulkan Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 760,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

    if (window == NULL) {
        printf("ERROR: failed to initialize SDL: %s", SDL_GetError());
        return -2;
    }

    if (!vulkan_init(window, "Vulkan Demo")) {
        LOG_ERROR("Failed to initialize Vulkan! Exiting...");
        exit(-1);
    }

    bool running = true;
    while (running) {
        running = processEvents();
        render();
    }

    vulkan_shutdown();
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    return 0;
}
