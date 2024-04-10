#include "input.h"

bool handle_keyboard_event(SDL_KeyboardEvent *event, bool pressed) {
    if (event->keysym.sym == SDLK_ESCAPE && pressed) {
        return true;
    }

    return false;
}