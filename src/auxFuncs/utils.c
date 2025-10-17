#include "utils.h"
#include "../leaderboard.h"
#include <stdio.h>

void RenderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, TextAlignment align) {
    if (!font || !text) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destRect = { x, y, surface->w, surface->h };
    
    // --- LÓGICA DE ALINHAMENTO AQUI ---
    if (align == TEXT_ALIGN_CENTER) {
        destRect.x = x - surface->w / 2;
    } else if (align == TEXT_ALIGN_RIGHT) {
        destRect.x = x - surface->w;
    }
    // Para TEXT_ALIGN_LEFT, não fazemos nada, pois o padrão já é alinhar à esquerda.

    SDL_RenderCopy(renderer, texture, NULL, &destRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void Leaderboard_Load(LeaderboardData* data) {
    FILE* file = fopen("leaderboards.dat", "rb");
    if (file) {
        fread(data, sizeof(LeaderboardData), 1, file);
        fclose(file);
    } else {
        // Se o arquivo não existe, apenas zera a contagem
        data->songCount = 0;
    }
}

// Salva a estrutura de placares inteira em um arquivo.
void Leaderboard_Save(const LeaderboardData* data) {
    FILE* file = fopen("leaderboards.dat", "wb");
    if (file) {
        fwrite(data, sizeof(LeaderboardData), 1, file);
        fclose(file);
    }
}