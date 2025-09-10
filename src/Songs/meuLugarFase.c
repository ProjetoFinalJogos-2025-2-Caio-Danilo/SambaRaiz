#include "../stage.h"
#include <stdio.h>

// --- DEFINIÇÕES DE TEMPO BASEADAS NA PARTITURA (BPM = 148) ---
// Usamos #define para facilitar a leitura e ajuste do beatmap.
#define QUARTER_NOTE_MS 405 // Duração de uma semínima (♩)
#define EIGHTH_NOTE_MS  202 // Duração de uma colcheia (♪)
#define SIXTEENTH_NOTE_MS 101 // Duração de uma semicolcheia (♬)
#define EIGHTEENTH_NOTE_MS 125
#define WHOLE_NOTE_MS (QUARTER_NOTE_MS * 4)
#define HALF_NOTE_MS (QUARTER_NOTE_MS * 2) // Duração de uma mínima (𝅗𝅥)
#define DOTTED_EIGHTH_MS (EIGHTH_NOTE_MS + SIXTEENTH_NOTE_MS) // Colcheia pontuada


//Função para carregar a Fase "Meu Lugar - Arlindo Cruz"
Fase* Carregar_Fase_MeuLugar(SDL_Renderer* renderer) {
    Fase* fase = (Fase*) malloc(sizeof(Fase));
    if (!fase) return NULL;

    fase->background = IMG_LoadTexture(renderer, "assets/image/meuLugarBG.png");
     fase->rhythmTrack = IMG_LoadTexture(renderer, "assets/image/rhythmTrack.png");
    fase->musica = Mix_LoadMUS("assets/music/MeuLugar_ArlindoCruz.mp3");

    if (!fase->background || !fase->musica || !fase->rhythmTrack) {
        printf("Erro ao carregar recursos da fase: %s\n", SDL_GetError());
        Fase_Liberar(fase);
        return NULL;
    }

    // --- DEFINIÇÃO DO BEATMAP ---
    int i = 0;
    Uint32 currentTime = 500; // Começa em 500ms para dar tempo ao jogador se preparar

   // --- INTRODUÇÃO (0:00 - 0:18) ---
    // Foco total no cavaquinho inicial.
    for (int j = 0; j < 8; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS * 3);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS * 2);
        currentTime += HALF_NOTE_MS * 2;
    }

    // --- VERSO 1 (0:18 - 0:52) ---
    // Padrão mais simples: Surdo nos tempos 2 e 4, cavaco no contratempo.
    for (int j = 0; j < 16; j++) {
        // Tempo 1 & 2
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + QUARTER_NOTE_MS);
        // Tempo 3 & 4
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        if (j % 4 == 3) { // Um preenchimento de pandeiro no final da frase
             fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        }
        currentTime += WHOLE_NOTE_MS;
    }

    // --- REFRÃO (0:52 - 1:25) ---
    // Ritmo mais intenso, mais notas de cavaco e pandeiro.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);

        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        currentTime += WHOLE_NOTE_MS;
    }

    // --- VERSO 2 (1:25 - 1:59) ---
    // Repete o padrão do primeiro verso.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        if (j % 4 == 3) {
             fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        }
        currentTime += WHOLE_NOTE_MS;
    }

    // --- REFRÃO 2 (1:59 - 2:32) ---
    // Repete o padrão do refrão.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        currentTime += WHOLE_NOTE_MS;
    }

    // --- SOLO DE CAVAQUINHO (2:32 - 3:05) ---
    // Dificuldade aumenta! Mais notas rápidas de cavaco.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS + SIXTEENTH_NOTE_MS); // Nota rápida
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS + SIXTEENTH_NOTE_MS); // Nota rápida
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        currentTime += WHOLE_NOTE_MS;
    }
    
    // --- REFRÃO 3 (3:05 - 4:00) ---
    // Repete o padrão do refrão, um pouco mais longo para o final.
    for (int j = 0; j < 28; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime + HALF_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS + EIGHTH_NOTE_MS);
        currentTime += WHOLE_NOTE_MS;
    }

    // --- FINALIZAÇÃO (4:00 - Fim) ---
    // Notas finais marcando os acordes de encerramento.
    currentTime += HALF_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    currentTime += HALF_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    currentTime += HALF_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    fase->beatmap[i++] = Note_Create(SDLK_c, currentTime); // Acorde final
    
    fase->totalNotas = i;
    fase->proximaNotaIndex = 0;

    return fase;
}
