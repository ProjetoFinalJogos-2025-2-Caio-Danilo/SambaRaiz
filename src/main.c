#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "defs.h"
#include "game.h"
#include "auxFuncs/auxWaitEvent.h"

int main(int argc, char* argv[]) {
    // Inicialização da SDL e suas bibliotecas
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL não pôde inicializar! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("SDL_image não pôde inicializar! IMG_Error: %s\n", IMG_GetError());
        return -1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer não pôde inicializar! Mix_Error: %s\n", Mix_GetError());
        return -1;
    }

    // Criação da janela e renderizador
    SDL_Window* window = SDL_CreateWindow("Meu Jogo de Ritmo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Janela não pôde ser criada! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderizador não pôde ser criado! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    // Inicializa o jogo
    if (!Game_Init(renderer)) {
        printf("Falha ao inicializar o jogo!\n");
        return -1;
    }
    
    Uint32 lastFrameTime = SDL_GetTicks();

    // Loop principal do jogo
    while (Game_IsRunning()) {
        // Lida com eventos
        SDL_Event e;
        Uint32 timeout = 16;
        if (AUX_WaitEventTimeout(&e, &timeout)) {
            Game_HandleEvent(&e);
        }

        // Calcula o delta time
        Uint32 currentFrameTime = SDL_GetTicks();
        float deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentFrameTime;

        // Atualiza e renderiza o jogo
        Game_Update(deltaTime);
        Game_Render(renderer);
    }

    // Encerramento
    Game_Shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}