#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "app.h" 

// Inicializa o estado do jogo, carrega recursos, etc.
// Retorna 1 em caso de sucesso, 0 em caso de falha.
int Game_Init(SDL_Renderer* renderer, const char* songFilePath);

// Lida com um único evento SDL (teclado, mouse, etc.).
void Game_HandleEvent(SDL_Event* e);

// Atualiza o estado de todos os objetos do jogo.
void Game_Update(float deltaTime);

// Renderiza tudo na tela.
void Game_Render(SDL_Renderer* renderer);

// Libera todos os recursos carregados pelo jogo.
void Game_Shutdown();

// Retorna true se o loop do jogo principal deve continuar.
bool Game_IsRunning();

// Retorna true se o jogo deve ser reiniciado após o encerramento.
bool Game_NeedsRestart();

ApplicationState Game_Run(SDL_Renderer* renderer, const char* songFilePath);


#endif // GAME_H