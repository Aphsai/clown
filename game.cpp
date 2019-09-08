#include "game.hpp"

Game::Game() {
    window = nullptr;
    current_game_state = GameState::PLAY;
}

Game::~Game() {
}

void Game::init() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("Clown", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    glewExperimental = GL_TRUE;
    glewInit();

}

void Game::process_input() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
    }
}

void Game::game_loop() {
    while(current_game_state != GameState::EXIT) {
        process_input();
    }
}

void Game::run() {
    init();
    game_loop();
}
