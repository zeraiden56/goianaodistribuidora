#include "startup.hpp"
#include "audio.hpp"
#include "controller.hpp"
#include <SDL_opengl.h>
#include <png.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

bool showStudioIntro(SDL_Window* window, Audio& audio, Controller& controller)
{
    char* base = SDL_GetBasePath();
    const std::string path = std::string(base ? base : "") + "images/logo-studio.png";
    SDL_free(base);
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&png, path.c_str())) {
        std::cerr << "Studio logo unavailable: " << png.message << '\n';
        png_image_free(&png);
        return true;
    }
    png.format = PNG_FORMAT_RGBA;
    std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(png));
    if (!png_image_finish_read(&png, nullptr, pixels.data(), 0, nullptr)) {
        std::cerr << "Studio logo decode failed: " << png.message << '\n';
        png_image_free(&png);
        return true;
    }
    const float aspect = static_cast<float>(png.width) / png.height;
    GLuint texture = 0;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, png.width, png.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    png_image_free(&png);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    SDL_SetRelativeMouseMode(SDL_FALSE);
    bool running = true, done = false, sounded = false, focused = true;
    float elapsed = 0;
    Uint64 previous = SDL_GetPerformanceCounter();
    while (!done && elapsed < 3.8f) {
        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = static_cast<float>(double(now - previous) / SDL_GetPerformanceFrequency());
        previous = now;
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if(controller.handle(event)&&event.type==SDL_CONTROLLERBUTTONDOWN)done=true;
            if (event.type == SDL_QUIT) { running = false; done = true; }
            if ((event.type == SDL_KEYDOWN && !event.key.repeat) || event.type == SDL_MOUSEBUTTONDOWN)
                done = true;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) focused = false;
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) focused = true;
            }
        }
        if (done) break;
        audio.pause(!focused);
        if (!focused) { SDL_Delay(16); continue; }
        elapsed += std::min(dt, .05f);
        if (!sounded && elapsed >= .85f) {
            audio.play(Sound::Startup);
            sounded = true;
        }
        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);
        width = std::max(width, 1); height = std::max(height, 1);
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(.96f, .94f, .88f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        const float size = std::min(height * .64f, width * .65f / aspect);
        const float floor = height * .5f + size * .5f;
        float lift = 0, squash = 0;
        if (elapsed < .85f) {
            const float p = elapsed / .85f;
            lift = (height + size) * .65f * (1 - p * p);
            squash = -.12f * p;
        } else {
            const float t = elapsed - .85f;
            lift = size * .30f * std::abs(std::sin(t * 7.5f)) * std::exp(-2.8f * t);
            squash = .29f * std::cos(t * 15.f) * std::exp(-3.2f * t);
        }
        const float sx = 1 + squash, sy = 1 / sx;
        const float halfWidth = size * aspect * sx * .5f;
        const float bottom = floor - lift;
        const float alpha = std::clamp(elapsed / .22f, 0.f, 1.f) *
                            std::clamp((3.8f - elapsed) / .45f, 0.f, 1.f);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1, 1, 1, alpha);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(width * .5f - halfWidth, bottom - size * sy);
        glTexCoord2f(1, 0); glVertex2f(width * .5f + halfWidth, bottom - size * sy);
        glTexCoord2f(1, 1); glVertex2f(width * .5f + halfWidth, bottom);
        glTexCoord2f(0, 1); glVertex2f(width * .5f - halfWidth, bottom);
        glEnd();
        SDL_GL_SwapWindow(window);
        SDL_Delay(8);
    }
    audio.clear();
    audio.pause(true);
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glDeleteTextures(1, &texture);
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
    return running;
}
