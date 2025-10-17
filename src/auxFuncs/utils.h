#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
} TextAlignment;

// Declaração da função de renderização de texto compartilhada
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, TextAlignment align);

#endif // UTILS_H