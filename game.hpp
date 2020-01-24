#pragma once

enum class GameState { PLAY, EXIT };

class Game {
    public:
        Game();
        ~Game();
        void run();
        void init();
        void process_input();
        void game_loop();

        SDL_Window* window;
        const unsigned int SCREEN_WIDTH = 1280;
        const unsigned int SCREEN_HEIGHT = 720;
        GameState current_game_state;
};
