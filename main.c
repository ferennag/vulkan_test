#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <std/core/memory.h>
#include "core/input.h"
#include "renderer/vulkan.h"

void handle_window_event(SDL_Window *window, SDL_WindowEvent event) {
    if (event.event == SDL_WINDOWEVENT_RESIZED) {
        vulkan_window_resized(window);
    }
}

bool processEvents(SDL_Window *window) {
    SDL_Event event;
    
    bool running = true;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                running = !handle_keyboard_event(&event.key, true);
                break;
            case SDL_KEYUP:
                running = !handle_keyboard_event(&event.key, false);
                break;
            case SDL_WINDOWEVENT:
                handle_window_event(window, event.window);
                break;
        }
    }

    return running;
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
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

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
        running = processEvents(window);
        vulkan_render();
    }

    vulkan_shutdown();
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    return 0;
}
