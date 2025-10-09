
#include "note.h"
#include "defs.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <math.h> // Para fmaxf

// Cria nota simples
Nota Note_Create(SDL_Keycode tecla, Uint32 spawnTime) {
    Nota n = {0}; // Zera a struct
    n.tecla = tecla;
    n.spawnTime = spawnTime;
    n.duration = 0;
    n.estado = NOTA_INATIVA;
    n.pos = (SDL_FRect){NOTE_START_X, NOTE_Y, NOTE_WIDTH, NOTE_HEIGHT};
    return n;
}

// Cria nota longa
Nota Note_CreateLong(SDL_Keycode tecla, Uint32 spawnTime, Uint32 duration) {
    Nota n = Note_Create(tecla, spawnTime);
    n.duration = duration;
    return n;
}

// Atualiza a posição
void Note_Update(Nota* nota, float deltaTime) {
    if (nota->estado == NOTA_ATIVA || nota->estado == NOTA_SEGURANDO || nota->estado == NOTA_QUEBRADA) {
        nota->pos.x -= NOTE_SPEED * deltaTime;
    }
}

// Lógica de renderização completa
void Note_Render(const Nota* nota, SDL_Renderer* renderer, float checker_pos_x) {
    if (nota->estado == NOTA_INATIVA || nota->estado == NOTA_ATINGIDA) return;

    Uint8 r = 255, g = 255, b = 255, a = 255;
    if (nota->tecla == SDLK_z) { g = 50; b = 50; }
    if (nota->tecla == SDLK_x) { r = 50; b = 50; }
    if (nota->tecla == SDLK_c) { r = 50; g = 50; }

    if (nota->estado == NOTA_QUEBRADA || nota->estado == NOTA_PERDIDA) {
        r = 100; g = 100; b = 100; a = 150;
    }

    Sint16 head_centerX = (Sint16)(nota->pos.x + nota->pos.w / 2);
    Sint16 centerY = (Sint16)(nota->pos.y + nota->pos.h / 2);
    Sint16 radius = (Sint16)(nota->pos.w / 2);
    float checker_centerX = checker_pos_x + (NOTE_WIDTH / 2);

    if (nota->duration > 0) { // Lógica para NOTA LONGA
        float body_length = NOTE_SPEED * (nota->duration / 1000.0f);
        float tail_centerX = head_centerX + body_length;

        // Desenha o CORPO 
        float body_visible_start_x = head_centerX;
        if (nota->estado == NOTA_SEGURANDO) {
            body_visible_start_x = fmaxf(head_centerX, checker_centerX);
        }

        if (tail_centerX > body_visible_start_x) {
            boxRGBA(renderer,
                (Sint16)body_visible_start_x, (Sint16)(centerY - radius / 2.5f),
                (Sint16)tail_centerX, (Sint16)(centerY + radius / 2.5f),
                r, g, b, a);
        }

        // Desenha a CAUDA (círculo final) 
        if (tail_centerX > checker_pos_x) {
             filledCircleRGBA(renderer, (Sint16)tail_centerX, centerY, radius, r, g, b, a);
             aacircleRGBA(renderer, (Sint16)tail_centerX, centerY, radius, 255, 255, 255, a);
        }
    }
    
    // Desenha a CABEÇA (círculo inicial) 
    if (!(nota->estado == NOTA_SEGURANDO && head_centerX < checker_centerX)) {
        filledCircleRGBA(renderer, head_centerX, centerY, radius, r, g, b, a);
        aacircleRGBA(renderer, head_centerX, centerY, radius, 255, 255, 255, a);
    }
}