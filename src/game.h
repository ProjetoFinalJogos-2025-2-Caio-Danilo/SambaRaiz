#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

// Inicializa o estado do jogo, carrega recursos, etc.
// Retorna 1 em caso de sucesso, 0 em caso de falha.
int Game_Init(SDL_Renderer* renderer);

// Lida com um único evento SDL (teclado, mouse, etc.).
void Game_HandleEvent(SDL_Event* e);

// Atualiza o estado de todos os objetos do jogo.
void Game_Update(float deltaTime);

// Renderiza tudo na tela.
void Game_Render(SDL_Renderer* renderer);

// Libera todos os recursos carregados pelo jogo.
void Game_Shutdown();

// Retorna 0 se o jogo deve ser encerrado.
int Game_IsRunning();

#endif