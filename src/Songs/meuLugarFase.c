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
    // Este padrão já estava ótimo, vamos mantê-lo.
    for (int j = 0; j < 8; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + EIGHTH_NOTE_MS * 3);
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS * 2);
        currentTime += HALF_NOTE_MS * 2;
    }

    // --- VERSO 1 (0:18 - 0:52) ---
    // Este padrão também é a base perfeita. Mantido.
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

    // --- REFRÃO (0:52 - 1:25) - PADRÃO SUAVIZADO ---
    // Ritmo mais marcante que o verso, mas sem excesso de notas.
    // O surdo (Z) marca o tempo 1, e o cavaco (X) responde no contratempo.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); // Surdo no tempo 1
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS); // Cavaco no tempo 2
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS); // Cavaco sincopado
        // A cada 4 compassos, um preenchimento com pandeiro (C)
        if (j % 4 == 3) {
            fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        }
        currentTime += WHOLE_NOTE_MS;
    }

    // --- VERSO 2 (1:25 - 1:59) ---
    // Repete o padrão do primeiro verso para dar um respiro ao jogador.
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
    // Repete o padrão do refrão suavizado.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        if (j % 4 == 3) {
            fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        }
        currentTime += WHOLE_NOTE_MS;
    }

    // --- SOLO DE CAVAQUINHO (2:32 - 3:05) - MAIS MUSICAL ---
    // O desafio aumenta, mas com frases rítmicas em vez de notas constantes.
    for (int j = 0; j < 16; j++) {
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); // Base do surdo
        // Frase rápida do cavaco
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS + SIXTEENTH_NOTE_MS); // Rápida!
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS); // Pandeiro respondendo
        // Outra frase
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        currentTime += WHOLE_NOTE_MS;
    }
    
    // --- REFRÃO FINAL PT1 (3:05 - até o fim) ---
    // Repete o refrão, mas por mais tempo, acompanhando a música até o fim.
    for (int j = 0; j < 52; j++) { // Aumentado para 24 iterações para cobrir o final
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS);
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + HALF_NOTE_MS + EIGHTH_NOTE_MS);
        if (j % 4 == 3) {
            fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + HALF_NOTE_MS + QUARTER_NOTE_MS);
        }
        currentTime += WHOLE_NOTE_MS;
    }

    // --- REFRÃO FINAL PT2 (Últimos ~40 segundos) ---
    // Cobre aproximadamente 18 compassos.
    for (int j = 0; j < 30; j++) { 
        fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); // Surdo no tempo 1
        fase->beatmap[i++] = Note_Create(SDLK_c, currentTime + QUARTER_NOTE_MS); // Pandeiro no tempo 2
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS + EIGHTH_NOTE_MS); // Cavaco sincopado
        fase->beatmap[i++] = Note_Create(SDLK_x, currentTime + QUARTER_NOTE_MS * 3); // Cavaco no tempo 4
        currentTime += WHOLE_NOTE_MS;
    }

    // --- FINALIZAÇÃO (Após o último refrão) ---
    // Notas simples que marcam os acordes finais da partitura.
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); // Marcação
    currentTime += HALF_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime); // Acorde
    currentTime += HALF_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); // Marcação
    currentTime += QUARTER_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_c, currentTime); // Prato/Pandeiro final
    currentTime += QUARTER_NOTE_MS;
    // Acorde final triplo
    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); 
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    fase->beatmap[i++] = Note_Create(SDLK_c, currentTime);
    currentTime += HALF_NOTE_MS;

    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); 
    currentTime += EIGHTH_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    currentTime += EIGHTH_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_c, currentTime);
    currentTime += WHOLE_NOTE_MS;

    fase->beatmap[i++] = Note_Create(SDLK_z, currentTime); 
    currentTime += EIGHTH_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_x, currentTime);
    currentTime += EIGHTH_NOTE_MS;
    fase->beatmap[i++] = Note_Create(SDLK_c, currentTime);
    
    fase->totalNotas = i;
    fase->proximaNotaIndex = 0;

    return fase;
}
