#ifndef NOTE_H
#define NOTE_H

#include <SDL2/SDL.h>

typedef enum {
    NOTA_INATIVA,
    NOTA_ATIVA,
    NOTA_ATINGIDA,
    NOTA_PERDIDA
} EstadoNota;

typedef struct {
    SDL_Keycode tecla;
    Uint32 spawnTime;
    EstadoNota estado;
    SDL_FRect pos;
} Nota;

// --- Protótipos de Funções ---
Nota Note_Create(SDL_Keycode tecla, Uint32 spawnTime);
void Note_Update(Nota* nota, float deltaTime);
void Note_Render(const Nota* nota, SDL_Renderer* renderer);

#endif // NOTE_H