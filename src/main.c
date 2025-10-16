#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "defs.h"
#include "game.h"
#include "auxFuncs/auxWaitEvent.h"
#include "app.h"
#include "menu.h" 

// Inicializa todos os subsistemas da SDL de uma só vez.
// Retorna 'true' em caso de sucesso, 'false' em caso de falha.
bool App_Init() {
    printf("Inicializando subsistemas da SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("ERRO: SDL nao pode inicializar! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("ERRO: SDL_image nao pode inicializar! IMG_Error: %s\n", IMG_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("ERRO: SDL_mixer nao pode inicializar! Mix_Error: %s\n", Mix_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        printf("ERRO: SDL_ttf nao pode inicializar! TTF_Error: %s\n", TTF_GetError());
        return false;
    }
    
    printf("Subsistemas inicializados com sucesso!\n");
    return true;
}

// Encerra todos os subsistemas da SDL.
void App_Shutdown() {
    printf("Encerrando subsistemas da SDL...\n");
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}


// FUNÇÃO PRINCIPAL

int main(int argc, char* argv[]) {

    // Inicializa todas as bibliotecas de uma vez.
    if (!App_Init()) {
        return -1;
    }

    // Cria a janela e o renderizador.
    SDL_Window* window = SDL_CreateWindow("Samba Raiz", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("ERRO: Janela nao pode ser criada! SDL_Error: %s\n", SDL_GetError());
        App_Shutdown(); // Limpa o que já foi inicializado
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("ERRO: Renderizador nao pode ser criado! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        App_Shutdown();
        return -1;
    }

    // Inicia o loop principal do jogo, que lida com o reinício.
    // Loop principal da APLICAÇÃO

    ApplicationState currentState = APP_STATE_MENU;
    char selectedSongPath[256] = {0}; // Para guardar a música que o menu escolheu

    while (currentState != APP_STATE_EXIT) {
        if (currentState == APP_STATE_MENU) {
            // Roda o módulo de menu e espera ele decidir o próximo estado
            currentState = Menu_Run(renderer, selectedSongPath); 
        } 
        else if (currentState == APP_STATE_GAMEPLAY) {
            // Roda o módulo de jogo com a música escolhida
            currentState = Game_Run(renderer, selectedSongPath);
        }
    }

    // Encerramento final de tudo.
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    App_Shutdown();

    return 0;
}