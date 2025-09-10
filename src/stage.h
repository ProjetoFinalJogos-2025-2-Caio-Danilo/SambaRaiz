#ifndef STAGE_H
#define STAGE_H

#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include "note.h"
#include "defs.h"

#define MAX_NOTAS_POR_FASE 2048

typedef struct {
    Mix_Music* musica;
    SDL_Texture* background;
    SDL_Texture* rhythmTrack; 
    Nota beatmap[MAX_NOTAS_POR_FASE];
    int totalNotas;
    int proximaNotaIndex; // Para saber qual a próxima nota a ser spawnada
} Fase;

// Carrega os recursos da fase e define o beatmap
Fase* Fase_Carregar(SDL_Renderer* renderer);

Fase* Carregar_Fase_MeuLugar(SDL_Renderer* render);

// Libera a memória usada pela fase
void Fase_Liberar(Fase* fase);

#endif // STAGE_H