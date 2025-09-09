#include "game.h"
#include "defs.h"
#include "stage.h"
#include "note.h"
#include <stdio.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
    SDL_KeyCode tecla;
    SDL_Rect rect;
} Checker;

// --- Estado Interno do Jogo (variáveis estáticas) ---
static Fase* s_faseAtual = NULL;
static Checker s_checkers[3];
static int s_gameIsRunning = 1;
static int s_score = 0;
static int s_combo = 0;
static TTF_Font* s_font = NULL;

// --- Implementação das Funções ---

int Game_Init(SDL_Renderer* renderer) {
    // Inicializa TTF
    if (TTF_Init() == -1) {
        printf("Erro ao inicializar SDL2_ttf: %s\n", TTF_GetError());
        return 0;
    }

    // Carrega a fase
    s_faseAtual = Fase_Carregar(renderer);
    if (!s_faseAtual) {
        return 0; // Falha ao carregar a fase
    }

    // Inicializa checkers
    s_checkers[0] = (Checker){SDLK_z, {CHECKER_Z_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};
    s_checkers[1] = (Checker){SDLK_x, {CHECKER_X_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};
    s_checkers[2] = (Checker){SDLK_c, {CHECKER_C_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}};

    // Inicia a música
    Mix_PlayMusic(s_faseAtual->musica, 0);

    // Abre a fonte para score/combo
    s_font = TTF_OpenFont("assets/font/pixelFont.ttf", 48);
    if (!s_font) {
        printf("Erro ao abrir fonte: %s\n", TTF_GetError());
        return 0;
    }

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
                        s_combo++;
                        s_score += 100 * s_combo; // Pontuação base * combo
                        printf("ACERTOU! Pontos: %d | Combo: %d\n", s_score, s_combo);
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

        // Checar perda
        if (nota->estado == NOTA_ATIVA) {
            float checker_pos_x = 0;
            if (nota->tecla == SDLK_z) checker_pos_x = CHECKER_Z_X;
            if (nota->tecla == SDLK_x) checker_pos_x = CHECKER_X_X;
            if (nota->tecla == SDLK_c) checker_pos_x = CHECKER_C_X;

            if (nota->pos.x < (checker_pos_x - HIT_WINDOW)) {
                nota->estado = NOTA_PERDIDA;
                s_combo = 0; // Zera combo
                printf("ERROU! Combo resetado.\n");
            }
        }
    }
}

void Game_Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, s_faseAtual->background, NULL, NULL);

    // Render checkers
    for (int i = 0; i < 3; ++i) {
        boxRGBA(renderer, s_checkers[i].rect.x, s_checkers[i].rect.y,
                s_checkers[i].rect.x + s_checkers[i].rect.w,
                s_checkers[i].rect.y + s_checkers[i].rect.h,
                255, 255, 255, 100);
        stringRGBA(renderer, s_checkers[i].rect.x + 20, s_checkers[i].rect.y + 20,
                   (i == 0 ? "Z" : (i == 1 ? "X" : "C")), 255, 255, 255, 255);
    }

    // Render notas
    for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
        Note_Render(&s_faseAtual->beatmap[i], renderer);
    }

    // --- Renderizar score e combo ---
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 223, 0, 255};

    char scoreText[64];
    sprintf(scoreText, "Pontos: %d", s_score);
    if (s_font) {
        SDL_Surface* surfScore = TTF_RenderText_Blended(s_font, scoreText, white);
        SDL_Texture* texScore = SDL_CreateTextureFromSurface(renderer, surfScore);
        SDL_Rect dstScore = {20, 20, surfScore->w, surfScore->h};
        SDL_RenderCopy(renderer, texScore, NULL, &dstScore);
        SDL_FreeSurface(surfScore);
        SDL_DestroyTexture(texScore);
    }

    if (s_combo > 1 && s_font) {
        char comboText[64];
        sprintf(comboText, "Combo: %d", s_combo);
        SDL_Surface* surfCombo = TTF_RenderText_Blended(s_font, comboText, gold);
        SDL_Texture* texCombo = SDL_CreateTextureFromSurface(renderer, surfCombo);
        SDL_Rect dstCombo = {SCREEN_WIDTH/2 - surfCombo->w/2, 50, surfCombo->w, surfCombo->h};
        SDL_RenderCopy(renderer, texCombo, NULL, &dstCombo);
        SDL_FreeSurface(surfCombo);
        SDL_DestroyTexture(texCombo);
    }

    SDL_RenderPresent(renderer);
}

void Game_Shutdown() {
    Fase_Liberar(s_faseAtual);

    if (s_font) {
        TTF_CloseFont(s_font);
        s_font = NULL;
    }
    TTF_Quit();
}

int Game_IsRunning() {
    return s_gameIsRunning;
}
