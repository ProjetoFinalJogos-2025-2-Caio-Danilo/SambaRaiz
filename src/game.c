#include "game.h"
#include "defs.h"
#include "stage.h"
#include "note.h"
#include "leaderboard.h"
#include "app.h"
#include "auxFuncs/utils.h"

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

/* =========================
   Sprite animada (pandeirista)
   ========================= */
#define PANDEIRISTA_PATH  "assets/image/parallax/projeto danilo-Sheet.png"
#define PANDEIRISTA_FRAMES 8
#define PANDEIRISTA_FPS    6.0f

// Linha da rua: usar topo da pista como referência
#define STREET_LINE_Y      (RHYTHM_TRACK_POS_Y - 64)

// Ajustes finos pedidos: mais à esquerda e pisando na rua
#define PAN_X_OFFSET      (-180.0f)  // negativo = move para a esquerda
#define PAN_Y_OFFSET      (-10.0f)   // <0 sobe um pouco

typedef struct {
    SDL_Texture* tex;
    int frameW, frameH, frames, current;
    float fps, timer, scale;
    SDL_FPoint pos;
} AnimatedSprite;

static AnimatedSprite g_pandeirista = {0};

/* =========================
   Background único (City2.png)
   ========================= */
#define BG_PATH "assets/image/parallax/City2.png"
static SDL_Texture* g_bgCity = NULL;

// Preenche 100% da largura; base ancorada na pista
static void DrawCityBackground(SDL_Renderer* r) {
    if (!g_bgCity) return;

    int tw = 0, th = 0;
    SDL_QueryTexture(g_bgCity, NULL, NULL, &tw, &th);
    if (tw <= 0 || th <= 0) return;

    float scaleW = (float)SCREEN_WIDTH / (float)tw;

    int dstW = SCREEN_WIDTH;
    int dstH = (int)(th * scaleW);
    int dstX = 0;
    int dstY = RHYTHM_TRACK_POS_Y - dstH; // ancora base na pista

    SDL_Rect dst = { dstX, dstY, dstW, dstH };
    SDL_RenderCopy(r, g_bgCity, NULL, &dst);
}

/* =========================
   Constantes e Tipos
   ========================= */
#define MAX_CONFETTI 200
#define SPECIAL_DURATION 10.0f
#define MAX_FEEDBACK_TEXTS 5

#define MAX_LEADERBOARD_ENTRIES 10
#define MAX_SONGS_IN_LEADERBOARD 50
#define SONG_NAME_MAX_LEN 64

typedef enum {
    STATE_PLAYING,
    STATE_RESULTS_ANIMATING,
    STATE_RESULTS_NAME_ENTRY,
    STATE_RESULTS_LEADERBOARD,
    STATE_PAUSE,
    STATE_GAMEOVER
} GameFlowState;

typedef struct {
    char text[64];
    SDL_Texture* texture;
    int w, h;
} CachedTexture;

typedef struct {
    SDL_Keycode tecla;
    SDL_Rect rect;
    float isPressedTimer;
} Checker;

typedef struct {
    bool isActive;
    SDL_FRect pos;
    SDL_FPoint velocity;
    SDL_Color color;
    float lifetime;
} ConfettiParticle;

typedef struct {
    bool isActive;
    int type; // 0: Otimo, 1: Bom, 2: Ok
    SDL_FRect pos;
    float lifetime;
} FeedbackText;

typedef struct {
    GameFlowState gameFlowState;

    Fase* faseAtual;
    Checker checkers[3];
    int score;
    int combo;
    float comboPulseTimer;
    TTF_Font* font;
    SDL_Texture* hitSpritesheet;
    SDL_Texture* checkerContornoTex[3];
    SDL_Texture* checkerKeyTex[3];

    float health;
    bool gameIsRunning;

    float specialMeter;
    bool isSpecialActive;
    float specialTimer;

    ConfettiParticle* confetti;
    FeedbackText* feedbackTexts;

    Mix_Chunk* failSound;
    Uint32 musicStartTime;
    Uint32 pauseStartTime;

    // UI cache
    CachedTexture scoreTexture;
    CachedTexture comboTexture;
    SDL_Texture* feedbackTextures[3];
    int cachedScore;
    int cachedCombo;

    // Resultados
    int finalScore;
    int displayedScore;
    int notesHit;
    float accuracy;
    int newHighscoreRank;
    SongLeaderboard* currentSongLeaderboard;
    char currentName[4];
    int nameEntryCharIndex;
    int selectedButtonIndex;
    bool needsRestart;

    ApplicationState nextApplicationState;

    bool debug;
} GameState;

static GameState s_gameState;
static LeaderboardData s_leaderboardData;

/* =========================
   Protótipos
   ========================= */
static void FindOrCreateCurrentSongLeaderboard(const char* songName);
static void SpawnConfettiParticle(void);
static void SpawnFeedbackText(int type, SDL_Rect checkerRect);
static void UpdateTextureCache(SDL_Renderer* renderer);

/* =========================
   Inicialização
   ========================= */
int Game_Init(SDL_Renderer* renderer, const char* songFilePath) {
    memset(&s_gameState, 0, sizeof(GameState));

    s_gameState.health = 100.0f;
    s_gameState.gameIsRunning = true;
    s_gameState.needsRestart = false;
    s_gameState.gameFlowState = STATE_PLAYING;
    s_gameState.notesHit = 0;

    s_gameState.cachedScore = -1;
    s_gameState.cachedCombo = -1;

    s_gameState.specialMeter = 0.0f;
    s_gameState.isSpecialActive = false;
    s_gameState.specialTimer = 0.0f;

    s_gameState.nextApplicationState = APP_STATE_MENU;

    srand((unsigned)time(NULL));

    s_gameState.confetti = (ConfettiParticle*)calloc(MAX_CONFETTI, sizeof(ConfettiParticle));
    s_gameState.feedbackTexts = (FeedbackText*)calloc(MAX_FEEDBACK_TEXTS, sizeof(FeedbackText));
    if (!s_gameState.confetti || !s_gameState.feedbackTexts) {
        printf("Erro ao alocar memoria para particulas/feedback!\n");
        return 0;
    }

    s_gameState.failSound = Mix_LoadWAV("assets/sound/failBoo.mp3");
    if (!s_gameState.failSound) {
        printf("Aviso: Nao foi possivel carregar o som de gameOver: %s\n", Mix_GetError());
    }

    s_gameState.hitSpritesheet = IMG_LoadTexture(renderer, "assets/image/hitNotesSpriteSheet.png");
    if (!s_gameState.hitSpritesheet) {
        printf("Erro ao carregar a sprite sheet de acerto: %s\n", IMG_GetError());
        return 0;
    }

    s_gameState.checkerContornoTex[0] = IMG_LoadTexture(renderer, "assets/image/redContorno.png");
    s_gameState.checkerContornoTex[1] = IMG_LoadTexture(renderer, "assets/image/greenContorno.png");
    s_gameState.checkerContornoTex[2] = IMG_LoadTexture(renderer, "assets/image/blueContorno.png");
    if (!s_gameState.checkerContornoTex[0] || !s_gameState.checkerContornoTex[1] || !s_gameState.checkerContornoTex[2]) {
        printf("Erro ao carregar uma ou mais texturas de contorno: %s\n", IMG_GetError());
        return 0;
    }

    s_gameState.faseAtual = Fase_CarregarDeArquivo(renderer, songFilePath);
    if (!s_gameState.faseAtual) return 0;

    // Background único
    g_bgCity = IMG_LoadTexture(renderer, BG_PATH);
    if (!g_bgCity) {
        SDL_Log("Falha ao carregar background '%s': %s", BG_PATH, IMG_GetError());
    }

    // Sprite do pandeirista 
    g_pandeirista.tex = IMG_LoadTexture(renderer, PANDEIRISTA_PATH);
    if (!g_pandeirista.tex) {
        SDL_Log("Falha ao carregar sprite do pandeirista: %s", IMG_GetError());
    } else {
        int texW = 0, texH = 0;
        SDL_QueryTexture(g_pandeirista.tex, NULL, NULL, &texW, &texH);
        g_pandeirista.frames  = PANDEIRISTA_FRAMES;
        g_pandeirista.frameW  = texW / PANDEIRISTA_FRAMES;
        g_pandeirista.frameH  = texH;
        g_pandeirista.current = 0;
        g_pandeirista.fps     = PANDEIRISTA_FPS;
        g_pandeirista.timer   = 0.0f;
        g_pandeirista.scale   = 0.6f; 

        float w = g_pandeirista.frameW * g_pandeirista.scale;
        float h = g_pandeirista.frameH * g_pandeirista.scale;

        // Centraliza e aplica offsets 
        g_pandeirista.pos.x = (SCREEN_WIDTH - w) * 0.5f + PAN_X_OFFSET;
        g_pandeirista.pos.y = STREET_LINE_Y - h + PAN_Y_OFFSET; // pés ancorados na rua
    }

    Leaderboard_Load(&s_leaderboardData);

    const char* simpleName = strrchr(songFilePath, '/');
    if (simpleName) simpleName++; else simpleName = songFilePath;

    char cleanName[SONG_NAME_MAX_LEN];
    strncpy(cleanName, simpleName, SONG_NAME_MAX_LEN - 1);
    cleanName[SONG_NAME_MAX_LEN - 1] = '\0';
    char* dot = strrchr(cleanName, '.');
    if (dot) *dot = '\0';

    FindOrCreateCurrentSongLeaderboard(cleanName);

    s_gameState.checkers[0] = (Checker){ SDLK_z, { CHECKER_Z_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT }, 0.0f };
    s_gameState.checkers[1] = (Checker){ SDLK_x, { CHECKER_X_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT }, 0.0f };
    s_gameState.checkers[2] = (Checker){ SDLK_c, { CHECKER_C_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT }, 0.0f };

    s_gameState.font = TTF_OpenFont("assets/font/pixelFont.ttf", 48);
    if (!s_gameState.font) {
        printf("Erro ao abrir fonte: %s\n", TTF_GetError());
        return 0;
    }

    SDL_Color black = { 0, 0, 0, 255 };
    const char* keys[] = { "Z", "X", "C" };
    for (int i = 0; i < 3; ++i) {
        SDL_Surface* surf = TTF_RenderText_Blended(s_gameState.font, keys[i], black);
        s_gameState.checkerKeyTex[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    SDL_Surface* surfOtimo = TTF_RenderText_Blended(s_gameState.font, "Otimo!", (SDL_Color){255, 223, 0, 255});
    s_gameState.feedbackTextures[0] = SDL_CreateTextureFromSurface(renderer, surfOtimo);
    SDL_FreeSurface(surfOtimo);

    SDL_Surface* surfBom = TTF_RenderText_Blended(s_gameState.font, "Bom", (SDL_Color){50, 205, 50, 255});
    s_gameState.feedbackTextures[1] = SDL_CreateTextureFromSurface(renderer, surfBom);
    SDL_FreeSurface(surfBom);

    SDL_Surface* surfOk = TTF_RenderText_Blended(s_gameState.font, "Ok", (SDL_Color){192, 192, 192, 255});
    s_gameState.feedbackTextures[2] = SDL_CreateTextureFromSurface(renderer, surfOk);
    SDL_FreeSurface(surfOk);

    Mix_PlayMusic(s_gameState.faseAtual->musica, 0);
    s_gameState.musicStartTime = SDL_GetTicks();

    s_gameState.debug = false;
    if (s_gameState.debug) {
        const double pularParaSegundos = 275.0;
        Mix_SetMusicPosition(pularParaSegundos);
        s_gameState.musicStartTime = SDL_GetTicks() - (Uint32)(pularParaSegundos * 1000.0);
    }

    return 1;
}

/* =========================
   Eventos
   ========================= */
void Game_HandleEvent(SDL_Event* e) {
    if (e->type == SDL_QUIT) { s_gameState.gameIsRunning = false; return; }
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) { s_gameState.gameIsRunning = false; return; }

    switch (s_gameState.gameFlowState) {
        case STATE_PLAYING: {
            if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_p && e->key.repeat == 0) {
                s_gameState.gameFlowState = STATE_PAUSE;
                s_gameState.pauseStartTime = SDL_GetTicks();
                Mix_PauseMusic();
                return;
            }

            switch (e->type) {
                case SDL_KEYDOWN: {
                    if (e->key.repeat != 0) {
                        bool isCurrentlyHolding = false;
                        for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                            Nota* nota = &s_gameState.faseAtual->beatmap[i];
                            if (nota->estado == NOTA_SEGURANDO && nota->tecla == e->key.keysym.sym) { isCurrentlyHolding = true; break; }
                        }
                        if (isCurrentlyHolding) break;
                    }

                    SDL_Keycode teclaPressionada = e->key.keysym.sym;
                    int checkerIndex = -1;

                    switch (teclaPressionada) {
                        case SDLK_z: checkerIndex = 0; break;
                        case SDLK_x: checkerIndex = 1; break;
                        case SDLK_c: checkerIndex = 2; break;
                        case SDLK_SPACE:
                            if (s_gameState.specialMeter >= 100.0f && !s_gameState.isSpecialActive) {
                                s_gameState.isSpecialActive = true;
                                s_gameState.specialTimer = SPECIAL_DURATION;
                            }
                            return;
                        default: return;
                    }

                    Checker* checker = &s_gameState.checkers[checkerIndex];
                    checker->isPressedTimer = 0.15f;
                    bool acertouNota = false;

                    for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                        Nota* nota = &s_gameState.faseAtual->beatmap[i];
                        if (nota->estado != NOTA_ATIVA || nota->tecla != checker->tecla) continue;

                        float dist = fabsf(nota->pos.x - checker->rect.x);
                        if (dist <= HIT_WINDOW_OK) {
                            s_gameState.notesHit++;
                            acertouNota = true;
                            s_gameState.combo++;
                            if (s_gameState.combo > 0 && s_gameState.combo % 50 == 0) s_gameState.comboPulseTimer = 0.3f;

                            int points = 0; int feedbackType = 0;
                            if (dist <= HIT_WINDOW_OTIMO) {
                                points = 20; feedbackType = 0; s_gameState.health += 2.0f;
                                if (!s_gameState.isSpecialActive) s_gameState.specialMeter += 0.75f + (s_gameState.combo * 0.1f);
                            } else if (dist <= HIT_WINDOW_BOM) {
                                points = 10; feedbackType = 1; s_gameState.health += 1.0f;
                                if (!s_gameState.isSpecialActive) s_gameState.specialMeter += 0.5f + (s_gameState.combo * 0.1f);
                            } else {
                                points = 2;  feedbackType = 2; s_gameState.health += 0.5f;
                                if (!s_gameState.isSpecialActive) s_gameState.specialMeter += 0.25f;
                            }

                            SpawnFeedbackText(feedbackType, checker->rect);

                            if (nota->duration > 0) {
                                nota->estado = NOTA_SEGURANDO;
                            } else {
                                nota->estado = NOTA_ATINGIDA;
                                points *= (s_gameState.combo > 0 ? s_gameState.combo : 1);
                                if (s_gameState.isSpecialActive) points *= 2;
                                s_gameState.score += points;
                            }
                            if (s_gameState.health > 100.0f) s_gameState.health = 100.0f;
                            if (s_gameState.specialMeter > 100.0f) s_gameState.specialMeter = 100.0f;
                            break;
                        }
                    }
                    if (!acertouNota) {
                        s_gameState.combo = 0;
                        s_gameState.health -= 5.0f;
                        if (s_gameState.health < 0.0f) s_gameState.health = 0.0f;
                    }
                } break;

                case SDL_KEYUP: {
                    if (e->key.repeat != 0) break;
                    SDL_Keycode teclaSolta = e->key.keysym.sym;
                    int checkerIndex = -1;
                    if (teclaSolta == SDLK_z) checkerIndex = 0; else if (teclaSolta == SDLK_x) checkerIndex = 1; else if (teclaSolta == SDLK_c) checkerIndex = 2; else break;
                    Checker* checker = &s_gameState.checkers[checkerIndex];

                    for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                        Nota* nota = &s_gameState.faseAtual->beatmap[i];
                        if (nota->estado == NOTA_SEGURANDO && nota->tecla == teclaSolta) {
                            float bodyLength = NOTE_SPEED * (nota->duration / 1000.0f);
                            float tail_pos_x = nota->pos.x + bodyLength;
                            float dist = fabsf(tail_pos_x - checker->rect.x);

                            if (dist <= HIT_WINDOW_OK) {
                                nota->estado = NOTA_ATINGIDA; s_gameState.combo++;
                                int points = 0; int feedbackType = 0;
                                if (dist <= HIT_WINDOW_OTIMO) { points = 40; feedbackType = 0; }
                                else if (dist <= HIT_WINDOW_BOM) { points = 20; feedbackType = 1; }
                                else { points = 5; feedbackType = 2; }

                                points *= (s_gameState.combo > 0 ? s_gameState.combo : 1);
                                if (s_gameState.isSpecialActive) points *= 2;
                                s_gameState.score += points;
                                SpawnFeedbackText(feedbackType, checker->rect);
                            } else {
                                nota->estado = NOTA_QUEBRADA; nota->despawnTimer = 0.5f;
                                s_gameState.combo = 0; s_gameState.health -= 2.0f;
                            }
                            break;
                        }
                    }
                } break;
            }
        } break;

        case STATE_PAUSE: {
            if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_p && e->key.repeat == 0) {
                s_gameState.gameFlowState = STATE_PLAYING;
                Uint32 pausedDuration = SDL_GetTicks() - s_gameState.pauseStartTime;
                s_gameState.musicStartTime += pausedDuration;
                Mix_ResumeMusic();
            }
        } break;

        case STATE_RESULTS_NAME_ENTRY: {
            if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
                SDL_Keycode key = e->key.keysym.sym;
                if (key == SDLK_UP) {
                    s_gameState.currentName[s_gameState.nameEntryCharIndex]++;
                    if (s_gameState.currentName[s_gameState.nameEntryCharIndex] > 'Z') s_gameState.currentName[s_gameState.nameEntryCharIndex] = 'A';
                } else if (key == SDLK_DOWN) {
                    s_gameState.currentName[s_gameState.nameEntryCharIndex]--;
                    if (s_gameState.currentName[s_gameState.nameEntryCharIndex] < 'A') s_gameState.currentName[s_gameState.nameEntryCharIndex] = 'Z';
                } else if (key == SDLK_RIGHT) {
                    s_gameState.nameEntryCharIndex = (s_gameState.nameEntryCharIndex + 1) % 3;
                } else if (key == SDLK_LEFT) {
                    s_gameState.nameEntryCharIndex = (s_gameState.nameEntryCharIndex - 1 + 3) % 3;
                } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (s_gameState.currentSongLeaderboard) {
                        for (int i = MAX_LEADERBOARD_ENTRIES - 1; i > s_gameState.newHighscoreRank; --i) {
                            s_gameState.currentSongLeaderboard->scores[i] = s_gameState.currentSongLeaderboard->scores[i - 1];
                        }
                        strcpy(s_gameState.currentSongLeaderboard->scores[s_gameState.newHighscoreRank].name, s_gameState.currentName);
                        s_gameState.currentSongLeaderboard->scores[s_gameState.newHighscoreRank].score = s_gameState.finalScore;
                        Leaderboard_Save(&s_leaderboardData);
                    }
                    s_gameState.gameFlowState = STATE_RESULTS_LEADERBOARD;
                }
            }
        } break;

        case STATE_RESULTS_LEADERBOARD: {
            if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
                SDL_Keycode key = e->key.keysym.sym;
                if (key == SDLK_UP)   s_gameState.selectedButtonIndex = (s_gameState.selectedButtonIndex - 1 + 3) % 3;
                else if (key == SDLK_DOWN) s_gameState.selectedButtonIndex = (s_gameState.selectedButtonIndex + 1) % 3;
                else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    switch (s_gameState.selectedButtonIndex) {
                        case 0: s_gameState.needsRestart = true;  s_gameState.gameIsRunning = false; break; // Jogar Novamente
                        case 1: s_gameState.nextApplicationState = APP_STATE_MENU; s_gameState.needsRestart = false; s_gameState.gameIsRunning = false; break;
                        case 2: s_gameState.nextApplicationState = APP_STATE_EXIT;  s_gameState.gameIsRunning = false; break;
                    }
                }
            }
        } break;

        case STATE_GAMEOVER: {
            if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
                SDL_Keycode key = e->key.keysym.sym;
                if (key == SDLK_UP)   s_gameState.selectedButtonIndex = (s_gameState.selectedButtonIndex - 1 + 3) % 3;
                else if (key == SDLK_DOWN) s_gameState.selectedButtonIndex = (s_gameState.selectedButtonIndex + 1) % 3;
                else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    switch (s_gameState.selectedButtonIndex) {
                        case 0: s_gameState.needsRestart = true;  s_gameState.gameIsRunning = false; break;
                        case 1: s_gameState.nextApplicationState = APP_STATE_MENU; s_gameState.needsRestart = false; s_gameState.gameIsRunning = false; break;
                        case 2: s_gameState.nextApplicationState = APP_STATE_EXIT;  s_gameState.needsRestart = false; s_gameState.gameIsRunning = false; break;
                    }
                }
            }
        } break;

        case STATE_RESULTS_ANIMATING: { /* sem entrada */ } break;
    }
}

/* =========================
   Update
   ========================= */
void Game_Update(float deltaTime) {
    if (s_gameState.debug) s_gameState.health = 100;

    // Atualiza animação do pandeirista (a cada frame)
    if (g_pandeirista.tex && s_gameState.gameFlowState == STATE_PLAYING) {
        g_pandeirista.timer += deltaTime;
        const float frameTime = 1.0f / g_pandeirista.fps;
        while (g_pandeirista.timer >= frameTime) {
            g_pandeirista.timer -= frameTime;
            g_pandeirista.current = (g_pandeirista.current + 1) % g_pandeirista.frames;
        }
    }

    switch (s_gameState.gameFlowState) {
        case STATE_PLAYING: {
            Uint32 tempoAtual = SDL_GetTicks() - s_gameState.musicStartTime;

            if (s_gameState.isSpecialActive) {
                s_gameState.specialTimer -= deltaTime;
                s_gameState.specialMeter = (s_gameState.specialTimer / SPECIAL_DURATION) * 100.0f;
                SpawnConfettiParticle();
                SpawnConfettiParticle();
                if (s_gameState.specialTimer <= 0) { s_gameState.isSpecialActive = false; s_gameState.specialMeter = 0; }
            }

            for (int i = 0; i < MAX_CONFETTI; ++i) {
                if (s_gameState.confetti[i].isActive) {
                    ConfettiParticle* p = &s_gameState.confetti[i];
                    p->lifetime -= deltaTime;
                    if (p->lifetime <= 0) { p->isActive = false; continue; }
                    p->pos.x += p->velocity.x * deltaTime;
                    p->pos.y += p->velocity.y * deltaTime;
                    p->velocity.y += 300.0f * deltaTime;
                }
            }

            if (s_gameState.faseAtual->proximaNotaIndex < s_gameState.faseAtual->totalNotas) {
                Nota* proxima = &s_gameState.faseAtual->beatmap[s_gameState.faseAtual->proximaNotaIndex];
                if (tempoAtual >= proxima->spawnTime) {
                    proxima->estado = NOTA_ATIVA;
                    s_gameState.faseAtual->proximaNotaIndex++;
                }
            }

            for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                Nota* nota = &s_gameState.faseAtual->beatmap[i];
                if (nota->estado == NOTA_ATIVA || nota->estado == NOTA_SEGURANDO) {
                    Note_Update(nota, deltaTime);
                }

                float checker_pos_x = 0;
                for (int j = 0; j < 3; ++j) if (s_gameState.checkers[j].tecla == nota->tecla) { checker_pos_x = s_gameState.checkers[j].rect.x; break; }

                if (nota->estado == NOTA_ATIVA) {
                    if (nota->pos.x < (checker_pos_x - HIT_WINDOW_OK)) {
                        nota->estado = NOTA_INATIVA; s_gameState.combo = 0; s_gameState.health -= 5.0f;
                    }
                } else if (nota->estado == NOTA_SEGURANDO) {
                    float bodyLength = NOTE_SPEED * (nota->duration / 1000.0f);
                    float tail_pos_x = nota->pos.x + bodyLength;
                    if (tail_pos_x < (checker_pos_x - HIT_WINDOW_OK)) {
                        nota->estado = NOTA_INATIVA; s_gameState.combo = 0; s_gameState.health -= 2.0f;
                    }
                }

                if (nota->despawnTimer > 0) {
                    nota->despawnTimer -= deltaTime;
                    if (nota->despawnTimer <= 0) nota->estado = NOTA_INATIVA;
                }
            }

            for (int i = 0; i < 3; ++i)
                if (s_gameState.checkers[i].isPressedTimer > 0)
                    s_gameState.checkers[i].isPressedTimer -= deltaTime;

            if (s_gameState.comboPulseTimer > 0) s_gameState.comboPulseTimer -= deltaTime;

            for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
                if (s_gameState.feedbackTexts[i].isActive) {
                    FeedbackText* ft = &s_gameState.feedbackTexts[i];
                    ft->lifetime -= deltaTime;
                    if (ft->lifetime <= 0) ft->isActive = false;
                    else ft->pos.y -= 50.0f * deltaTime;
                }
            }

            if (tempoAtual >= s_gameState.faseAtual->durationMs) {
                s_gameState.gameFlowState = STATE_RESULTS_ANIMATING;
                s_gameState.finalScore = s_gameState.score;
                s_gameState.displayedScore = 0;
                if (s_gameState.faseAtual->totalNotas > 0)
                    s_gameState.accuracy = ((float)s_gameState.notesHit / (float)s_gameState.faseAtual->totalNotas) * 100.0f;

                s_gameState.newHighscoreRank = -1;
                if (s_gameState.currentSongLeaderboard)
                    for (int i = 0; i < MAX_LEADERBOARD_ENTRIES; ++i)
                        if (s_gameState.finalScore > s_gameState.currentSongLeaderboard->scores[i].score) { s_gameState.newHighscoreRank = i; break; }
            }

            if (s_gameState.health <= 0) {
                s_gameState.gameFlowState = STATE_GAMEOVER;
                Mix_HaltMusic();
                if (s_gameState.failSound) Mix_PlayChannel(-1, s_gameState.failSound, 0);
                s_gameState.selectedButtonIndex = 0;
            }

            if (s_gameState.health < 0.0f) s_gameState.health = 0.0f;
        } break;

        case STATE_RESULTS_ANIMATING: {
            if (s_gameState.displayedScore < s_gameState.finalScore) {
                int increment = (int)((s_gameState.finalScore / 2.0f) * deltaTime) + 1;
                s_gameState.displayedScore += increment;
            } else {
                s_gameState.displayedScore = s_gameState.finalScore;
                if (s_gameState.newHighscoreRank != -1) {
                    s_gameState.gameFlowState = STATE_RESULTS_NAME_ENTRY;
                    strcpy(s_gameState.currentName, "AAA");
                    s_gameState.nameEntryCharIndex = 0;
                } else {
                    s_gameState.gameFlowState = STATE_RESULTS_LEADERBOARD;
                    s_gameState.selectedButtonIndex = 0;
                }
            }
        } break;

        case STATE_PAUSE:
        case STATE_RESULTS_NAME_ENTRY:
        case STATE_RESULTS_LEADERBOARD:
        case STATE_GAMEOVER:
            break;
    }
}

/* =========================
   Render
   ========================= */
void Game_Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* 1) Fundo (cidade) */
    DrawCityBackground(renderer);

    /* 2) Pista de ritmo */
    SDL_Rect trackRect = { RHYTHM_TRACK_POS_X, RHYTHM_TRACK_POS_Y, RHYTHM_TRACK_WIDTH, RHYTHM_TRACK_HEIGHT };
    SDL_RenderCopy(renderer, s_gameState.faseAtual->rhythmTrack, NULL, &trackRect);
    rectangleRGBA(renderer, trackRect.x, trackRect.y, trackRect.x + trackRect.w, trackRect.y + trackRect.h, 255, 255, 255, 180);

    /* 3) Checkers + contornos */
    for (int i = 0; i < 3; ++i) {
        Checker* checker = &s_gameState.checkers[i];
        Sint16 centerX = checker->rect.x + (checker->rect.w / 2);
        Sint16 centerY = checker->rect.y + (checker->rect.h / 2);
        Sint16 radius  = checker->rect.w / 2;

        SDL_Rect contourDstRect = { checker->rect.x - CHECKER_CONTOUR_OFFSET_X, checker->rect.y - CHECKER_CONTOUR_OFFSET_Y, CHECKER_CONTOUR_WIDTH, CHECKER_CONTOUR_HEIGHT };
        SDL_RenderCopy(renderer, s_gameState.checkerContornoTex[i], NULL, &contourDstRect);

        if (checker->isPressedTimer > 0) filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 150);
        else                              filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 70);

        if (s_gameState.checkerKeyTex[i]) {
            int w, h; SDL_QueryTexture(s_gameState.checkerKeyTex[i], NULL, NULL, &w, &h);
            SDL_Rect textRect = { centerX - w / 2, centerY - h / 2, w, h };
            SDL_RenderCopy(renderer, s_gameState.checkerKeyTex[i], NULL, &textRect);
        }
    }

    /* 4) PANDEIRISTA */
    if (g_pandeirista.tex) {
        SDL_Rect src = {
            g_pandeirista.current * g_pandeirista.frameW, 0,
            g_pandeirista.frameW, g_pandeirista.frameH
        };
        SDL_FRect dst = {
            g_pandeirista.pos.x, g_pandeirista.pos.y,
            g_pandeirista.frameW * g_pandeirista.scale,
            g_pandeirista.frameH * g_pandeirista.scale
        };
        SDL_RenderCopyF(renderer, g_pandeirista.tex, &src, &dst);
    }

    /* 5) Notas */
    for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
        Nota* nota = &s_gameState.faseAtual->beatmap[i];
        float checker_pos_x = (nota->tecla == SDLK_z) ? CHECKER_Z_X : (nota->tecla == SDLK_x) ? CHECKER_X_X : CHECKER_C_X;
        Note_Render(nota, renderer, checker_pos_x);
    }

    /* 6) Confetes */
    for (int i = 0; i < MAX_CONFETTI; ++i) {
        if (s_gameState.confetti[i].isActive) {
            ConfettiParticle* p = &s_gameState.confetti[i];
            boxRGBA(renderer, (Sint16)p->pos.x, (Sint16)p->pos.y, (Sint16)(p->pos.x + p->pos.w), (Sint16)(p->pos.y + p->pos.h),
                    p->color.r, p->color.g, p->color.b, p->color.a);
        }
    }

    /* 7) Feedback (Ótimo/Bom/Ok) */
    for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
        if (s_gameState.feedbackTexts[i].isActive) {
            FeedbackText* ft = &s_gameState.feedbackTexts[i];
            SDL_Texture* tex = s_gameState.feedbackTextures[ft->type];
            if (tex) {
                int w, h; SDL_QueryTexture(tex, NULL, NULL, &w, &h);
                Uint8 alpha = (Uint8)(255.0f * (ft->lifetime / 0.6f));
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_Rect dst = { (int)ft->pos.x - w / 2, (int)ft->pos.y, w, h };
                SDL_RenderCopy(renderer, tex, NULL, &dst);
                SDL_SetTextureAlphaMod(tex, 255);
            }
        }
    }

    /* 8) Score/Combo */
    UpdateTextureCache(renderer);
    if (s_gameState.scoreTexture.texture) {
        SDL_Rect dst = { 20, 20, s_gameState.scoreTexture.w, s_gameState.scoreTexture.h };
        SDL_RenderCopy(renderer, s_gameState.scoreTexture.texture, NULL, &dst);
    }
    if (s_gameState.combo > 1 && s_gameState.comboTexture.texture) {
        float scale = (s_gameState.comboPulseTimer > 0) ? 1.0f + 0.5f * (s_gameState.comboPulseTimer / 0.3f) : 1.0f;
        int w = (int)(s_gameState.comboTexture.w * scale);
        int h = (int)(s_gameState.comboTexture.h * scale);
        SDL_Rect dst = { SCREEN_WIDTH / 2 - w / 2, 100, w, h };
        SDL_RenderCopy(renderer, s_gameState.comboTexture.texture, NULL, &dst);
    }

    /* 9) Barras de Vida e Especial */
    int barWidth = 400, barHeight = 20, barX = (SCREEN_WIDTH / 2) - (barWidth / 2), barY = 20;
    int currentHealthWidth = (int)((s_gameState.health / 100.0f) * barWidth);
    SDL_Color healthColor = { 50, 205, 50, 255 };
    if (s_gameState.health < 50) healthColor = (SDL_Color){255, 215, 0, 255};
    if (s_gameState.health < 25) healthColor = (SDL_Color){220, 20, 60, 255};
    boxRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 50, 50, 50, 200);
    if (currentHealthWidth > 0) boxRGBA(renderer, barX, barY, barX + currentHealthWidth, barY + barHeight, healthColor.r, healthColor.g, healthColor.b, 255);
    rectangleRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 255, 255, 255, 255);

    int specialBarWidth = 400, specialBarHeight = 15;
    int specialBarX = (SCREEN_WIDTH / 2) - (specialBarWidth / 2);
    int specialBarY = RHYTHM_TRACK_POS_Y + RHYTHM_TRACK_HEIGHT + 10;
    int currentSpecialWidth = (int)((s_gameState.specialMeter / 100.0f) * specialBarWidth);
    SDL_Color specialColor = (s_gameState.specialMeter >= 100.0f) ? (SDL_Color){255,223,0,255} : (SDL_Color){0,191,255,255};
    boxRGBA(renderer, specialBarX, specialBarY, specialBarX + specialBarWidth, specialBarY + specialBarHeight, 50, 50, 50, 200);
    if (currentSpecialWidth > 0) boxRGBA(renderer, specialBarX, specialBarY, specialBarX + currentSpecialWidth, specialBarY + specialBarHeight, specialColor.r, specialColor.g, specialColor.b, 255);
    rectangleRGBA(renderer, specialBarX, specialBarY, specialBarX + specialBarWidth, specialBarY + specialBarHeight, 255, 255, 255, 255);

    /* 10) Overlays de pausa/resultados/game over */
    if (s_gameState.gameFlowState == STATE_GAMEOVER && s_gameState.font) {
        boxRGBA(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 200);
        RenderText(renderer, s_gameState.font, "FIM DE JOGO", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 3, (SDL_Color){255, 0, 0, 255}, true);
        const char* buttonLabels[] = { "Recomecar", "Voltar ao Menu", "Sair" };
        SDL_Color selectedColor = {255, 223, 0, 255};
        SDL_Color normalColor   = {255, 255, 255, 255};
        for (int i = 0; i < 3; ++i) {
            SDL_Color color = (i == s_gameState.selectedButtonIndex) ? selectedColor : normalColor;
            RenderText(renderer, s_gameState.font, buttonLabels[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 50 + i * 60, color, true);
        }
    }

    if ((s_gameState.gameFlowState == STATE_PAUSE) && s_gameState.font) {
        boxRGBA(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);
        SDL_Color white = {255,255,255,255};
        SDL_Surface* surfPause = TTF_RenderText_Blended(s_gameState.font, "PAUSE", white);
        if (surfPause) {
            SDL_Texture* texPause = SDL_CreateTextureFromSurface(renderer, surfPause);
            SDL_Rect dstPause = { SCREEN_WIDTH/2 - surfPause->w/2, SCREEN_HEIGHT/2 - surfPause->h/2, surfPause->w, surfPause->h };
            SDL_RenderCopy(renderer, texPause, NULL, &dstPause);
            SDL_FreeSurface(surfPause);
            SDL_DestroyTexture(texPause);
        }
    }

    if ((s_gameState.gameFlowState >= STATE_RESULTS_ANIMATING) && (s_gameState.gameFlowState != STATE_GAMEOVER)) {
        boxRGBA(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 200);
        char buffer[128];
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color gold  = {255, 223, 0, 255};
        sprintf(buffer, "%d", s_gameState.displayedScore);
        RenderText(renderer, s_gameState.font, "Placar Final", SCREEN_WIDTH / 2, 100, white, true);
        RenderText(renderer, s_gameState.font, buffer,         SCREEN_WIDTH / 2, 150, gold,  true);
        sprintf(buffer, "Precisao: %.2f%%", s_gameState.accuracy);
        RenderText(renderer, s_gameState.font, buffer,         SCREEN_WIDTH / 2, 200, white, true);

        if (s_gameState.gameFlowState == STATE_RESULTS_NAME_ENTRY) {
            RenderText(renderer, s_gameState.font, "NOVO RECORDE!",  SCREEN_WIDTH / 2, 300, gold, true);
            RenderText(renderer, s_gameState.font, "Insira seu nome:", SCREEN_WIDTH / 2, 350, white, true);
            for (int i = 0; i < 3; ++i) {
                char letterStr[2] = { s_gameState.currentName[i], '\0' };
                int x_pos = SCREEN_WIDTH / 2 + (i - 1) * 60;
                int y_pos = 420;
                bool showChar = !(i == s_gameState.nameEntryCharIndex && (SDL_GetTicks() / 400) % 2 == 0);
                if (showChar) RenderText(renderer, s_gameState.font, letterStr, x_pos, 420, white, true);
                int underline_y = y_pos + 55;
                boxRGBA(renderer, x_pos - 25, underline_y, x_pos + 25, underline_y + 2, 255, 255, 255, 255);
            }
        } else if (s_gameState.gameFlowState == STATE_RESULTS_LEADERBOARD) {
            const char* title = (s_gameState.newHighscoreRank != -1) ? "Parabens!" : "Ranking da Musica";
            RenderText(renderer, s_gameState.font, title, SCREEN_WIDTH / 2, 60, gold, true);
            for (int i = 0; i < 5; ++i) {
                sprintf(buffer, "%2d. %s", i + 1, s_gameState.currentSongLeaderboard->scores[i].name);
                RenderText(renderer, s_gameState.font, buffer, SCREEN_WIDTH / 4, 280 + i * 40, white, false);
                sprintf(buffer, "%d", s_gameState.currentSongLeaderboard->scores[i].score);
                RenderText(renderer, s_gameState.font, buffer, SCREEN_WIDTH / 4 + 200, 280 + i * 40, white, false);

                sprintf(buffer, "%2d. %s", i + 6, s_gameState.currentSongLeaderboard->scores[i+5].name);
                RenderText(renderer, s_gameState.font, buffer, SCREEN_WIDTH / 2 + 100, 280 + i * 40, white, false);
                sprintf(buffer, "%d", s_gameState.currentSongLeaderboard->scores[i+5].score);
                RenderText(renderer, s_gameState.font, buffer, SCREEN_WIDTH / 2 + 300, 280 + i * 40, white, false);
            }
            const char* buttonLabels[] = { "Jogar Novamente", "Voltar ao Menu", "Sair" };
            SDL_Color selectedColor = {255,223,0,255}, normalColor = {255,255,255,255};
            for (int i = 0; i < 3; ++i) {
                SDL_Color color = (i == s_gameState.selectedButtonIndex) ? selectedColor : normalColor;
                RenderText(renderer, s_gameState.font, buttonLabels[i], SCREEN_WIDTH / 2, 550 + i * 50, color, true);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

/* =========================
   Run Loop
   ========================= */
ApplicationState Game_Run(SDL_Renderer* renderer, const char* songFilePath) {
    bool restart = false;
    do {
        restart = false;
        if (!Game_Init(renderer, songFilePath)) return APP_STATE_MENU;

        Uint32 lastFrameTime = SDL_GetTicks();

        while (Game_IsRunning()) {
            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) Game_HandleEvent(&e);

            Uint32 currentFrameTime = SDL_GetTicks();
            float deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
            lastFrameTime = currentFrameTime;
            if (deltaTime > 0.1f) deltaTime = 0.1f;

            Game_Update(deltaTime);
            Game_Render(renderer);
        }

        restart = Game_NeedsRestart();
        Game_Shutdown();
    } while (restart);

    return s_gameState.nextApplicationState;
}

/* =========================
   UI cache
   ========================= */
static void UpdateTextureCache(SDL_Renderer* renderer) {
    if (s_gameState.score != s_gameState.cachedScore) {
        s_gameState.cachedScore = s_gameState.score;
        char buffer[64]; sprintf(buffer, "Pontos: %d", s_gameState.score);
        SDL_Color white = {255,255,255,255};
        SDL_Surface* surf = TTF_RenderText_Blended(s_gameState.font, buffer, white);
        if (surf) {
            if (s_gameState.scoreTexture.texture) SDL_DestroyTexture(s_gameState.scoreTexture.texture);
            s_gameState.scoreTexture.texture = SDL_CreateTextureFromSurface(renderer, surf);
            s_gameState.scoreTexture.w = surf->w; s_gameState.scoreTexture.h = surf->h;
            SDL_FreeSurface(surf);
        }
    }
    if (s_gameState.combo != s_gameState.cachedCombo) {
        s_gameState.cachedCombo = s_gameState.combo;
        char buffer[64]; sprintf(buffer, "Combo: %d", s_gameState.combo);
        SDL_Color gold = {255,223,0,255};
        SDL_Surface* surf = TTF_RenderText_Blended(s_gameState.font, buffer, gold);
        if (surf) {
            if (s_gameState.comboTexture.texture) SDL_DestroyTexture(s_gameState.comboTexture.texture);
            s_gameState.comboTexture.texture = SDL_CreateTextureFromSurface(renderer, surf);
            s_gameState.comboTexture.w = surf->w; s_gameState.comboTexture.h = surf->h;
            SDL_FreeSurface(surf);
        }
    }
}

/* =========================
   Helpers
   ========================= */
static void SpawnConfettiParticle(void) {
    for (int i = 0; i < MAX_CONFETTI; ++i) {
        if (!s_gameState.confetti[i].isActive) {
            ConfettiParticle* p = &s_gameState.confetti[i];
            p->isActive = true;
            p->lifetime = 2.0f + (rand() % 100) / 100.0f;
            p->pos.x = (rand() % 2 == 0) ? -10.0f : SCREEN_WIDTH + 10.0f;
            p->velocity.x = (p->pos.x < 0) ? (100 + (rand() % 100)) : (-100 - (rand() % 100));
            p->pos.y = rand() % SCREEN_HEIGHT;
            p->velocity.y = -100 - (rand() % 150);
            p->pos.w = 5 + (rand() % 5);
            p->pos.h = p->pos.w;
            p->color = (SDL_Color){100 + rand() % 156, 100 + rand() % 156, 100 + rand() % 156, 255};
            break;
        }
    }
}

static void FindOrCreateCurrentSongLeaderboard(const char* songName) {
    for (int i = 0; i < s_leaderboardData.songCount; ++i) {
        if (strcmp(s_leaderboardData.songLeaderboards[i].songName, songName) == 0) {
            s_gameState.currentSongLeaderboard = &s_leaderboardData.songLeaderboards[i];
            return;
        }
    }
    if (s_leaderboardData.songCount < MAX_SONGS_IN_LEADERBOARD) {
        int newIndex = s_leaderboardData.songCount;
        s_gameState.currentSongLeaderboard = &s_leaderboardData.songLeaderboards[newIndex];

        strcpy(s_gameState.currentSongLeaderboard->songName, songName);
        for (int i = 0; i < MAX_LEADERBOARD_ENTRIES; ++i) {
            strcpy(s_gameState.currentSongLeaderboard->scores[i].name, "---");
            s_gameState.currentSongLeaderboard->scores[i].score = 0;
        }
        s_leaderboardData.songCount++;
    } else {
        s_gameState.currentSongLeaderboard = NULL;
    }
}

static void SpawnFeedbackText(int type, SDL_Rect checkerRect) {
    for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
        if (!s_gameState.feedbackTexts[i].isActive) {
            FeedbackText* ft = &s_gameState.feedbackTexts[i];
            ft->isActive = true; ft->type = type; ft->lifetime = 0.6f;

            float startY = checkerRect.y - 20.0f;
            bool positionFound = false;
            while (!positionFound) {
                positionFound = true;
                for (int j = 0; j < MAX_FEEDBACK_TEXTS; ++j) {
                    if (i == j || !s_gameState.feedbackTexts[j].isActive) continue;
                    if (fabsf(s_gameState.feedbackTexts[j].pos.y - startY) < 25.0f) {
                        startY -= 25.0f; positionFound = false; break;
                    }
                }
            }
            ft->pos.x = checkerRect.x + (checkerRect.w / 2.0f);
            ft->pos.y = startY;
            break;
        }
    }
}

/* =========================
   Shutdown
   ========================= */
void Game_Shutdown(void) {
    Fase_Liberar(s_gameState.faseAtual);

    for (int i = 0; i < 3; i++) {
        if (s_gameState.checkerContornoTex[i]) SDL_DestroyTexture(s_gameState.checkerContornoTex[i]);
        if (s_gameState.checkerKeyTex[i]) SDL_DestroyTexture(s_gameState.checkerKeyTex[i]);
    }
    for (int i = 0; i < 3; i++) if (s_gameState.feedbackTextures[i]) SDL_DestroyTexture(s_gameState.feedbackTextures[i]);

    if (s_gameState.hitSpritesheet) SDL_DestroyTexture(s_gameState.hitSpritesheet);
    if (s_gameState.failSound)      Mix_FreeChunk(s_gameState.failSound);

    if (s_gameState.scoreTexture.texture) SDL_DestroyTexture(s_gameState.scoreTexture.texture);
    if (s_gameState.comboTexture.texture) SDL_DestroyTexture(s_gameState.comboTexture.texture);

    free(s_gameState.confetti);
    free(s_gameState.feedbackTexts);

    // libera background
    if (g_bgCity) { SDL_DestroyTexture(g_bgCity); g_bgCity = NULL; }

    // libera sprite do pandeirista
    if (g_pandeirista.tex) { SDL_DestroyTexture(g_pandeirista.tex); g_pandeirista.tex = NULL; }
}

bool Game_NeedsRestart(void) { return s_gameState.needsRestart; }
bool Game_IsRunning(void)    { return s_gameState.gameIsRunning; }
