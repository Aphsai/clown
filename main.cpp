#include <string>
#include <stdio.h>

#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "ecs/coordinator.hpp"


SDL_Window *main_window;
SDL_GLContext main_context;
std::string program_name = "OPENGL TEST";
Coordinator coordinator;

bool set_opengl_attributes();
void run_game();
void cleanup();

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { printf("Failed to initialize SDL\n"); }
    main_window = SDL_CreateWindow(program_name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_OPENGL);
    if (!main_window) { printf("Unable to create window\n"); }
    main_context = SDL_GL_CreateContext(main_window);
    set_opengl_attributes();

    SDL_GL_SetSwapInterval(1);
    return true;
}

bool set_opengl_attributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    return true;
}

int main(int argc, char* argv[]) {
    if (!init()) return -1;
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(main_window);

    run_game();
    cleanup();
    return 0;
}

void run_game() {
    bool loop = true;
    while(loop) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) loop = false;
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: 
                        loop = false;
                        break;
                    case SDLK_r:
                        glClearColor(1.0, 0.0, 0.0, 1.0);
                        glClear(GL_COLOR_BUFFER_BIT);
                        SDL_GL_SwapWindow(main_window);
                        break;
                    case SDLK_g:
                        glClearColor(0.0, 1.0, 0.0, 1.0);
                        glClear(GL_COLOR_BUFFER_BIT);
                        SDL_GL_SwapWindow(main_window);
                        break;
                    case SDLK_b:
                        glClearColor(0.0, 0.0, 1.0, 1.0);
                        glClear(GL_COLOR_BUFFER_BIT);
                        SDL_GL_SwapWindow(main_window);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void cleanup() {
    SDL_GL_DeleteContext(main_context);
    SDL_DestroyWindow(main_window);
    SDL_Quit();
}

