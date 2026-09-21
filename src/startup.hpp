#pragma once
#include <SDL.h>
class Audio;
class Controller;
// Returns false if the user closes the window during the intro.
bool showStudioIntro(SDL_Window* window, Audio& audio, Controller& controller);
