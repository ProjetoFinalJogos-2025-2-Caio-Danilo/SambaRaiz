#include "game.h"
#include "defs.h"
#include "stage.h"
#include "note.h"
#include <stdio.h>
#include <SDL2/SDL2_gfxPrimitives.h>

typedef struct {
    SDL_KeyCode tecla;
    SDL_Rect rect;
} Checker;

// --- Estado Interno do Jogo (variáveis estáticas) ---
static Fase* s_faseAtual = NULL;
static Checker s_checkers[3];
static int s_gameIsRunning = 1;

// --- Implementação das Funções ---

int Game_Init(SDL_Renderer* renderer) {
    s_faseAtual = Fase_Carregar(renderer);
    if (!s_faseAtual) {
        return 0; // Falha ao carregar a fase
    }

    s_checkers[0] = (Checker){SDLK_z, {CHECKER_Z_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};
    s_checkers[1] = (Checker){SDLK_x, {CHECKER_X_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};
    s_checkers[2] = (Checker){SDLK_c, {CHECKER_C_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};
    
    // Inicia a música
    Mix_PlayMusic(s_faseAtual->musica, 0);

    return 1;
}

void Game_HandleEvent(SDL_Event* e) {
    if (e->type == SDL_QUIT) {
        s_gameIsRunning = 0;
    }
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_ESCAPE) {
            s_gameIsRunning = 0;
        }

        SDL_KeyCode teclaPressionada = e->key.keysym.sym;
        if (teclaPressionada == SDLK_z || teclaPressionada == SDLK_x || teclaPressionada == SDLK_c) {
            for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
                Nota* nota = &s_faseAtual->beatmap[i];
                if (nota->estado == NOTA_ATIVA && nota->tecla == teclaPressionada) {
                    float checker_pos_x = 0;
                    if (teclaPressionada == SDLK_z) checker_pos_x = CHECKER_Z_X;
                    if (teclaPressionada == SDLK_x) checker_pos_x = CHECKER_X_X;
                    if (teclaPressionada == SDLK_c) checker_pos_x = CHECKER_C_X;

                    if (fabs(nota->pos.x - checker_pos_x) <= HIT_WINDOW) {
                        nota->estado = NOTA_ATINGIDA;
                        printf("ACERTOU!\n");
                        break; 
                    }
                }
            }
        }
    }
}

void Game_Update(float deltaTime) {
    if (!s_gameIsRunning || Mix_PlayingMusic() == 0) return;

    Uint32 tempoAtual = (Uint32)(Mix_GetMusicPosition(s_faseAtual->musica) * 1000.0);
    
    // Spawner de notas
    if (s_faseAtual->proximaNotaIndex < s_faseAtual->totalNotas) {
        Nota* proxima = &s_faseAtual->beatmap[s_faseAtual->proximaNotaIndex];
        if (tempoAtual >= proxima->spawnTime) {
            proxima->estado = NOTA_ATIVA;
            s_faseAtual->proximaNotaIndex++;
        }
    }

    // Atualizar notas
    for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
        Nota* nota = &s_faseAtual->beatmap[i];
        Note_Update(nota, deltaTime);

        // Checar perda (apenas se ainda estiver ativa)
        if (nota->estado == NOTA_ATIVA) {
            float checker_pos_x = 0;
            if (nota->tecla == SDLK_z) checker_pos_x = CHECKER_Z_X;
            if (nota->tecla == SDLK_x) checker_pos_x = CHECKER_X_X;
            if (nota->tecla == SDLK_c) checker_pos_x = CHECKER_C_X;
            
            if (nota->pos.x < (checker_pos_x - HIT_WINDOW)) {
                nota->estado = NOTA_PERDIDA;
                printf("ERROU!\n");
            }
        }
    }
}

void Game_Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, s_faseAtual->background, NULL, NULL);

    for (int i = 0; i < 3; ++i) {
        boxRGBA(renderer, s_checkers[i].rect.x, s_checkers[i].rect.y,
                s_checkers[i].rect.x + s_checkers[i].rect.w, s_checkers[i].rect.y + s_checkers[i].rect.h,
                255, 255, 255, 100);
        stringRGBA(renderer, s_checkers[i].rect.x + 20, s_checkers[i].rect.y + 20,
                   (i == 0 ? "Z" : (i == 1 ? "X" : "C")), 255, 255, 255, 255);
    }
    
    for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
        Note_Render(&s_faseAtual->beatmap[i], renderer);
    }

    SDL_RenderPresent(renderer);
}

void Game_Shutdown() {
    Fase_Liberar(s_faseAtual);
}

int Game_IsRunning() {
    return s_gameIsRunning;
}