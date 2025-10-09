// Em note.h

#ifndef NOTE_H
#define NOTE_H

#include <SDL2/SDL.h>

// Estados possíveis de uma nota
typedef enum {
    NOTA_INATIVA,
    NOTA_ATIVA,
    NOTA_ATINGIDA,
    NOTA_PERDIDA,
    NOTA_SEGURANDO, 
    NOTA_QUEBRADA 
} EstadoNota;

typedef struct {
    SDL_Keycode tecla;
    Uint32 spawnTime;
    Uint32 duration;      // Duração em ms. 0 para notas normais.
    EstadoNota estado;
    SDL_FRect pos;
    float despawnTimer; // Timer para notas erradas desaparecerem.
} Nota;

// Protótipos das funções
Nota Note_Create(SDL_Keycode tecla, Uint32 spawnTime);
Nota Note_CreateLong(SDL_Keycode tecla, Uint32 spawnTime, Uint32 duration);
void Note_Update(Nota* nota, float deltaTime);
void Note_Render(const Nota* nota, SDL_Renderer* renderer, float checker_pos_x);

#endif