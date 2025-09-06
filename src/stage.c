#include "stage.h"
#include <stdio.h>

//Nome "Stage" arbitrário, apenas fase de teste, cada música terá sua prórpria fase com estrutura similar, alterando as notas.
Fase* Fase_Carregar(SDL_Renderer* renderer) {
    Fase* fase = (Fase*) malloc(sizeof(Fase));
    if (!fase) return NULL;

    fase->background = IMG_LoadTexture(renderer, "assets/image/rjbg.jpg");
    fase->musica = Mix_LoadMUS("assets/music/song.wav");

    if (!fase->background || !fase->musica) {
        printf("Erro ao carregar recursos da fase: %s\n", SDL_GetError());
        Fase_Liberar(fase);
        return NULL;
    }

    // --- DEFINIÇÃO DO BEATMAP ---
    int i = 0;
    fase->beatmap[i++] = Note_Create(SDLK_z, 1000);
    fase->beatmap[i++] = Note_Create(SDLK_x, 1500);
    fase->beatmap[i++] = Note_Create(SDLK_c, 2000);
    fase->beatmap[i++] = Note_Create(SDLK_z, 2250);
    fase->beatmap[i++] = Note_Create(SDLK_z, 2500);
    fase->beatmap[i++] = Note_Create(SDLK_x, 3000);
    fase->beatmap[i++] = Note_Create(SDLK_c, 3500);
    fase->beatmap[i++] = Note_Create(SDLK_c, 3750);
    
    fase->totalNotas = i;
    fase->proximaNotaIndex = 0;

    return fase;
}

void Fase_Liberar(Fase* fase) {
    if (fase) {
        SDL_DestroyTexture(fase->background);
        Mix_FreeMusic(fase->musica);
        free(fase);
    }
}