#include "note.h"
#include "defs.h"
#include <SDL2/SDL2_gfxPrimitives.h>

// Cria e inicializa uma nota com valores padrão.
Nota Note_Create(SDL_Keycode tecla, Uint32 spawnTime) {
    Nota n;
    n.tecla = tecla;
    n.spawnTime = spawnTime;
    n.estado = NOTA_INATIVA;
    n.pos.w = NOTE_WIDTH;
    n.pos.h = NOTE_HEIGHT;
    n.pos.y = NOTE_Y;
    n.pos.x = NOTE_START_X; // Todas começam fora da tela
    return n;
}

// Atualiza a posição de uma nota se ela estiver ativa.
void Note_Update(Nota* nota, float deltaTime) {
    if (nota->estado == NOTA_ATIVA) {
        nota->pos.x -= NOTE_SPEED * deltaTime;
    }
}

// Renderiza uma nota na tela se ela estiver ativa.
void Note_Render(const Nota* nota, SDL_Renderer* renderer) {
    if (nota->estado == NOTA_ATIVA) {
        // Define a cor da nota baseada na tecla
        Uint8 r = 255, g = 255, b = 255;
        if (nota->tecla == SDLK_z) { g = 50; b = 50; }   // Z = Vermelho
        if (nota->tecla == SDLK_x) { r = 50; b = 50; }   // X = Verde
        if (nota->tecla == SDLK_c) { r = 50; g = 50; }   // C = Azul

        // Calcular o centro e o raio para o círculo
        Sint16 centerX = (Sint16)(nota->pos.x + nota->pos.w / 2);
        Sint16 centerY = (Sint16)(nota->pos.y + nota->pos.h / 2);
        Sint16 radius = (Sint16)(nota->pos.w / 2);

        // Desenha a nota como um círculo preenchido
        filledCircleRGBA(renderer, centerX, centerY, radius, r, g, b, 255);
        
        aacircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 255); // Contorno branco
    }
}