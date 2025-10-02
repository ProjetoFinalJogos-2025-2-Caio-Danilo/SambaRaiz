#include "game.h"
#include "defs.h"
#include "stage.h"
#include "note.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h> 
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
    SDL_KeyCode tecla;
    SDL_Rect rect;
    float isPressedTimer;
} Checker;

typedef struct {
    int isActive;
    SDL_Rect srcRect;       
    SDL_FRect pos;
    float lifetime;
    float timeAlive;
} HitAnimation;

// --- Estado Interno do Jogo (variáveis estáticas) ---

#define MAX_HIT_ANIMATIONS 32
#define HIT_FRAME_WIDTH 384
#define HIT_FRAME_HEIGHT 512
#define HIT_FRAME_COUNT 8
#define HIT_FRAME_COLS 4

static Fase* s_faseAtual = NULL;
static Checker s_checkers[3];
static int s_gameIsRunning = 1;
static int s_score = 0;
static int s_combo = 0;
static TTF_Font* s_font = NULL;
static SDL_Texture* s_hitSpritesheet = NULL;
static HitAnimation s_hitAnimations[MAX_HIT_ANIMATIONS];
static SDL_Texture* s_checkerContornoTex[3] = {NULL, NULL, NULL}; //Array para as texturas dos contornos
static float s_health = 100.0f; // Vida do jogador (de 0.0 a 100.0)
static bool s_isGameOver = false; // Flag para controlar o fim de jogo


// --- Implementação das Funções ---

static void SpawnHitAnimation(SDL_Rect checkerPos);

int Game_Init(SDL_Renderer* renderer) {

    //Garante Vida cheia ao iniciar o jogo.
    s_health = 100.0f;
    s_isGameOver = false;

    // Inicializa o gerador de números aleatórios
    srand(time(NULL));

    // Inicializa TTF
    if (TTF_Init() == -1) {
        printf("Erro ao inicializar SDL2_ttf: %s\n", TTF_GetError());
        return 0;
    }

    // --- Carrega a sprite sheet ---
    s_hitSpritesheet = IMG_LoadTexture(renderer, "assets/image/hitNotesSpriteSheet.png");
    if (!s_hitSpritesheet) {
        printf("Erro ao carregar a sprite sheet de acerto: %s\n", IMG_GetError());
        return 0;
    }

    // Carrega as texturas dos contornos dos checkers
    s_checkerContornoTex[0] = IMG_LoadTexture(renderer, "assets/image/redContorno.png");   // Para Z
    s_checkerContornoTex[1] = IMG_LoadTexture(renderer, "assets/image/greenContorno.png"); // Para X
    s_checkerContornoTex[2] = IMG_LoadTexture(renderer, "assets/image/blueContorno.png");  // Para C

    if (!s_checkerContornoTex[0] || !s_checkerContornoTex[1] || !s_checkerContornoTex[2]) {
        printf("Erro ao carregar uma ou mais texturas de contorno: %s\n", IMG_GetError());
        return 0; // Falha na inicialização
    }

    // Carrega a fase
    s_faseAtual = Carregar_Fase_MeuLugar(renderer);
    if (!s_faseAtual) {
        return 0; // Falha ao carregar a fase
    }

    // Inicializa checkers
    s_checkers[0] = (Checker){SDLK_z, {CHECKER_Z_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};
    s_checkers[1] = (Checker){SDLK_x, {CHECKER_X_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};
    s_checkers[2] = (Checker){SDLK_c, {CHECKER_C_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};

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
    if (s_isGameOver){
        
        //Conseguir fechar o jogo quando der GameOver, por enquanto deixarei assim.
        switch (e->type){
            case SDL_QUIT:
                s_gameIsRunning = 0;
        
            case SDL_KEYDOWN:
                SDL_KeyCode teclaPressionada = e->key.keysym.sym;

                switch (teclaPressionada) {
                    case SDLK_ESCAPE: s_gameIsRunning = 0;
                    default:
                        // Se não for uma tecla do jogo, encerra o evento.
                        return; 
                }
        }
        
    }
    switch (e->type){

        case SDL_QUIT:
            s_gameIsRunning = 0;
        
        case SDL_KEYDOWN:
            SDL_KeyCode teclaPressionada = e->key.keysym.sym;

            // Identifica qual checker foi ativado
            int checkerIndex = -1;
            switch (teclaPressionada) {
                case SDLK_ESCAPE: s_gameIsRunning = 0;
                case SDLK_z: checkerIndex = 0; break;
                case SDLK_x: checkerIndex = 1; break;
                case SDLK_c: checkerIndex = 2; break;
                default:
                    // Se não for uma tecla do jogo, encerra o evento.
                    return; 
            }

            // Com o checker identificado, executa a lógica principal
            Checker* checker = &s_checkers[checkerIndex];

            // Ativa o highlight visual do checker 
            checker->isPressedTimer = 0.15f;

            bool acertouNota = false;

            // Procura por uma nota correspondente para o checker ativado
            for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
                Nota* nota = &s_faseAtual->beatmap[i];

                // Pula notas que não estão ativas ou não são para esta tecla
                if (nota->estado != NOTA_ATIVA || nota->tecla != checker->tecla) {
                    continue;
                }

                // Calcula a distância usando a posição do checker que já temos
                float dist = fabsf(nota->pos.x - checker->rect.x);

                if (dist <= HIT_WINDOW) {
                    nota->estado = NOTA_ATINGIDA;
                    acertouNota = true;
                    s_combo++;
                    s_score += 100 * (s_combo > 0 ? s_combo : 1);

                    //Recupera vida ao acertar ---
                    s_health += 2.0f; // Recupera 2% de vida
                    if (s_health > 100.0f) s_health = 100.0f; // Limita em 100%

                    printf("ACERTOU! Pontos: %d | Combo: %d\n", s_score, s_combo);
                    
                    SpawnHitAnimation(checker->rect); 
                    break;
                }
            }

            // Se o loop terminou e nenhuma nota foi acertada, foi um erro
            if (!acertouNota) {
                s_combo = 0;
                // Perde vida ao errar
                s_health -= 5.0f; // Perde 5% de vida
                if (s_health < 0.0f) s_health = 0.0f;

                printf("ERROU! Combo resetado.\n");
            }
            break;
    }

}

void Game_Update(float deltaTime) {
    if (s_isGameOver) return;
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

                //Perde vida ao deixar nota passar
                s_health -= 5.0f; // Perde 5% de vida
                if (s_health < 0.0f) s_health = 0.0f;

                printf("ERROU! Combo resetado.\n");
            }
        }
    }

    // Atualiza os timers de feedback dos checkers
    for (int i = 0; i < 3; ++i) {
        if (s_checkers[i].isPressedTimer > 0) {
            s_checkers[i].isPressedTimer -= deltaTime;
        }
    }

    for (int i = 0; i < MAX_HIT_ANIMATIONS; ++i) {
        if (s_hitAnimations[i].isActive) {
            HitAnimation* anim = &s_hitAnimations[i];
            anim->timeAlive += deltaTime;

            if (anim->timeAlive >= anim->lifetime) {
                anim->isActive = 0;
            } else {
                // Move para cima
                anim->pos.y -= 80.0f * deltaTime; 
            }
        }
    }

    // Checagem de Game Over ---
    if (s_health <= 0 && !s_isGameOver) {
        s_isGameOver = true;
        Mix_HaltMusic();
        printf("GAME OVER!\n");
    }
}

void Game_Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Desenha Background Principal
    SDL_RenderCopy(renderer, s_faseAtual->background, NULL, NULL);

    // Desenha a pista de ritmo
    SDL_Rect trackRect = {RHYTHM_TRACK_POS_X, RHYTHM_TRACK_POS_Y, RHYTHM_TRACK_WIDTH, RHYTHM_TRACK_HEIGHT};
    SDL_RenderCopy(renderer, s_faseAtual->rhythmTrack, NULL, &trackRect);
    rectangleRGBA(renderer, trackRect.x, trackRect.y,
                  trackRect.x + trackRect.w, trackRect.y + trackRect.h,
                  255, 255, 255, 180); 

    // Render checkers
    for (int i = 0; i < 3; ++i) {
        Checker* checker = &s_checkers[i];

        // Calcula o centro do checker para desenhar círculos
        Sint16 centerX = checker->rect.x + (checker->rect.w / 2);
        Sint16 centerY = checker->rect.y + (checker->rect.h / 2);
        Sint16 radius = checker->rect.w / 2; // Raio do círculo

        // Define a área de desenho para a textura do contorno
        SDL_Rect contourDstRect = {
            checker->rect.x - CHECKER_CONTOUR_OFFSET_X,
            checker->rect.y - CHECKER_CONTOUR_OFFSET_Y,
            CHECKER_CONTOUR_WIDTH,
            CHECKER_CONTOUR_HEIGHT
        };

        // Desenha a imagem do contorno primeiro, para que o preenchimento fique por cima
        SDL_RenderCopy(renderer, s_checkerContornoTex[i], NULL, &contourDstRect);

        // Desenha o preenchimento interno do checker
        if (checker->isPressedTimer > 0) {
            // Preenchimento branco e mais visível no highlight
            filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 150); // Raio -2 para ter uma pequena borda
        } else {
            // Preenchimento padrão, mais sutil
            filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 70); // Raio -2 para ter uma pequena borda
        }

        // Desenha o texto da tecla (Z, X, C) no centro do círculo
        const char* keyText = (i == 0) ? "Z" : ((i == 1) ? "X" : "C");
        SDL_Surface* textSurface = TTF_RenderText_Blended(s_font, keyText, (SDL_Color){0, 0, 0, 255}); // Texto preto
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                centerX - (textSurface->w / 2),
                centerY - (textSurface->h / 2),
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }
    }


    // Render notas
    for (int i = 0; i < s_faseAtual->totalNotas; ++i) {
        Note_Render(&s_faseAtual->beatmap[i], renderer);
    }

    // --- Renderiza as animações de acerto ---
    for (int i = 0; i < MAX_HIT_ANIMATIONS; ++i) {
        if (s_hitAnimations[i].isActive) {
            HitAnimation* anim = &s_hitAnimations[i];
            
            float lifeRatio = anim->timeAlive / anim->lifetime;
            Uint8 alpha = 255 * (1.0f - lifeRatio);

            //Transparência da textura
            SDL_SetTextureAlphaMod(s_hitSpritesheet, alpha);
            SDL_RenderCopyF(renderer, s_hitSpritesheet, &anim->srcRect, &anim->pos);
            SDL_SetTextureAlphaMod(s_hitSpritesheet, 255);
        }
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

    // Renderizar Barra de Vida ---

    // Posição e dimensões da barra
    int barWidth = 400;
    int barHeight = 20;
    int barX = (SCREEN_WIDTH / 2) - (barWidth / 2);
    int barY = 20;

    // Calcula a largura atual da vida
    int currentHealthWidth = (int)((s_health / 100.0f) * barWidth);

    // Define a cor da barra baseada na porcentagem de vida
    SDL_Color healthColor = {50, 205, 50, 255}; // Verde
    if (s_health < 50) {
        healthColor = (SDL_Color){255, 215, 0, 255}; // Amarelo
    }
    if (s_health < 25) {
        healthColor = (SDL_Color){220, 20, 60, 255}; // Vermelho
    }

    // Desenha o fundo da barra
    boxRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 50, 50, 50, 200);

    // Desenha o preenchimento da vida
    if (currentHealthWidth > 0) {
        boxRGBA(renderer, barX, barY, barX + currentHealthWidth, barY + barHeight, 
                healthColor.r, healthColor.g, healthColor.b, 255);
    }

    // Desenha a borda da barra
    rectangleRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 255, 255, 255, 255);


    // Mensagem de Game Over ---
    if (s_isGameOver && s_font) {
        SDL_Color red = {255, 0, 0, 255};
        SDL_Surface* surfGO = TTF_RenderText_Blended(s_font, "FIM DE JOGO", red);
        SDL_Texture* texGO = SDL_CreateTextureFromSurface(renderer, surfGO);
        SDL_Rect dstGO = {
            SCREEN_WIDTH / 2 - surfGO->w / 2,
            SCREEN_HEIGHT / 2 - surfGO->h / 2,
            surfGO->w,
            surfGO->h
        };
        SDL_RenderCopy(renderer, texGO, NULL, &dstGO);
        SDL_FreeSurface(surfGO);
        SDL_DestroyTexture(texGO);
    }

    SDL_RenderPresent(renderer);
}

static void SpawnHitAnimation(SDL_Rect checkerPos) {
    for (int i = 0; i < MAX_HIT_ANIMATIONS; ++i) {
        if (!s_hitAnimations[i].isActive) {
            HitAnimation* anim = &s_hitAnimations[i];
            anim->isActive = 1;

            // Escolhe um frame aleatório (0 a 7)
            int frameIndex = rand() % HIT_FRAME_COUNT;

            // Calcula posição do frame dentro da grade (4x2)
            int col = frameIndex % HIT_FRAME_COLS;
            int row = frameIndex / HIT_FRAME_COLS;

            anim->srcRect.x = col * HIT_FRAME_WIDTH;
            anim->srcRect.y = row * HIT_FRAME_HEIGHT;
            anim->srcRect.w = HIT_FRAME_WIDTH;
            anim->srcRect.h = HIT_FRAME_HEIGHT;

            // Renderizar em 64x64 centralizado no checker
            anim->pos.w = 64;
            anim->pos.h = 64;
            anim->pos.x = checkerPos.x + (checkerPos.w / 2) - (anim->pos.w / 2);
            anim->pos.y = checkerPos.y - anim->pos.h;

            anim->lifetime = 0.4f;
            anim->timeAlive = 0.0f;
            break;
        }
    }
}

void Game_Shutdown() {
    Fase_Liberar(s_faseAtual);

    // Libera as texturas dos contornos 
    for (int i = 0; i < 3; i++) {
        if (s_checkerContornoTex[i]) {
            SDL_DestroyTexture(s_checkerContornoTex[i]);
        }
    }

    SDL_DestroyTexture(s_hitSpritesheet);

    if (s_font) {
        TTF_CloseFont(s_font);
        s_font = NULL;
    }
    TTF_Quit();
}

int Game_IsRunning() {
    return s_gameIsRunning;
}
