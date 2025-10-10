#include "game.h"
#include "defs.h"
#include "stage.h"
#include "note.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h> // Adicionado para malloc/calloc/free
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

// Estruturas para cache de texturas e organização
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

// Agrupamento vas variáveis de estado do Jogo em uma única struct
typedef struct {
    Fase* faseAtual;
    Checker checkers[3];
    int score;
    int combo;
    float comboPulseTimer;
    TTF_Font* font;
    SDL_Texture* hitSpritesheet;
    SDL_Texture* checkerContornoTex[3]; //Array para as texturas dos contornos
    SDL_Texture* checkerKeyTex[3];      // Texturas para as teclas Z, X, C

    float health;             // Vida do jogador (de 0.0 a 100.0)

    bool gameIsRunning;       // Flag para controlar o estado de Rodando
    bool isGameOver;          // Flag para controlar o fim de jogo
    bool isPaused;            // Flag para controlar o estado de pause

    float specialMeter;       // Medidor de 0.0 a 100.0 para o Especial
    bool isSpecialActive;     // Check do Especial se está ativo
    float specialTimer;       // Tempo do Especial
    
    ConfettiParticle* confetti;
    FeedbackText* feedbackTexts;

    Mix_Chunk* failSound;       // Som de game over
    Uint32 musicStartTime;      // Variavel para controle de tempo, usada em "tempoAtual". Substitiu a função: Mix_GetMusicPosition
    Uint32 pauseStartTime;      // Guarda o momento em que o jogo foi pausado

    // Cache para texturas de UI
    CachedTexture scoreTexture;
    CachedTexture comboTexture;
    SDL_Texture* feedbackTextures[3]; // 0: Otimo, 1: Bom, 2: Ok
    int cachedScore;
    int cachedCombo;

} GameState;

// Instância do estado do jogo
static GameState s_gameState;

// Constantes
#define MAX_CONFETTI 200        // Máximo de partículas de confetti na tela
#define SPECIAL_DURATION 10.0f  // Duração do especial em segundos
#define MAX_FEEDBACK_TEXTS 5    // Permite que até 5 textos apareçam ao mesmo tempo

// Protótipos de Funções Estáticas
static void SpawnConfettiParticle();
static void SpawnFeedbackText(int type, SDL_Rect checkerRect);
static void UpdateTextureCache(SDL_Renderer* renderer);

int Game_Init(SDL_Renderer* renderer) {
    // Evitar lixo de memória
    memset(&s_gameState, 0, sizeof(GameState));

    // Garante Vida cheia ao iniciar o jogo.
    s_gameState.health = 100.0f;
    s_gameState.isGameOver = false;
    s_gameState.isPaused = false;
    s_gameState.gameIsRunning = true;

    // Força a primeira atualização das texturas de placar/combo
    s_gameState.cachedScore = -1; 
    s_gameState.cachedCombo = -1;

    // Inicializa o sistema de especial
    s_gameState.specialMeter = 0.0f;
    s_gameState.isSpecialActive = false;
    s_gameState.specialTimer = 0.0f;

    // Inicializa o gerador de números aleatórios
    srand(time(NULL));

    // Inicializa TTF
    if (TTF_Init() == -1) {
        printf("Erro ao inicializar SDL2_ttf: %s\n", TTF_GetError());
        return 0;
    }

    // Alocação dinâmica para partículas e textos
    s_gameState.confetti = calloc(MAX_CONFETTI, sizeof(ConfettiParticle));
    s_gameState.feedbackTexts = calloc(MAX_FEEDBACK_TEXTS, sizeof(FeedbackText));

    if (!s_gameState.confetti || !s_gameState.feedbackTexts) {
        printf("Erro ao alocar memoria para particulas/feedback!\n");
        return 0; // Falha crítica
    }
    
    // Carrega o som de Game Over
    s_gameState.failSound = Mix_LoadWAV("assets/music/failBoo.mp3");
    if (!s_gameState.failSound) {
        printf("Aviso: Nao foi possivel carregar o som de gameOver: %s\n", Mix_GetError());
    }

    // Carrega a sprite sheet
    s_gameState.hitSpritesheet = IMG_LoadTexture(renderer, "assets/image/hitNotesSpriteSheet.png");
    if (!s_gameState.hitSpritesheet) {
        printf("Erro ao carregar a sprite sheet de acerto: %s\n", IMG_GetError());
        return 0;
    }

    // Carrega as texturas dos contornos dos checkers
    s_gameState.checkerContornoTex[0] = IMG_LoadTexture(renderer, "assets/image/redContorno.png");   // Para Z
    s_gameState.checkerContornoTex[1] = IMG_LoadTexture(renderer, "assets/image/greenContorno.png"); // Para X
    s_gameState.checkerContornoTex[2] = IMG_LoadTexture(renderer, "assets/image/blueContorno.png");  // Para C

    if (!s_gameState.checkerContornoTex[0] || !s_gameState.checkerContornoTex[1] || !s_gameState.checkerContornoTex[2]) {
        printf("Erro ao carregar uma ou mais texturas de contorno: %s\n", IMG_GetError());
        return 0; // Falha na inicialização
    }

    // Carrega a fase
    s_gameState.faseAtual = Carregar_Fase_MeuLugar(renderer);
    if (!s_gameState.faseAtual) {
        return 0; // Falha ao carregar a fase
    }

    // Inicializa checkers
    s_gameState.checkers[0] = (Checker){SDLK_z, {CHECKER_Z_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};
    s_gameState.checkers[1] = (Checker){SDLK_x, {CHECKER_X_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};
    s_gameState.checkers[2] = (Checker){SDLK_c, {CHECKER_C_X, CHECKER_Y, NOTE_WIDTH, NOTE_HEIGHT}, 0.0f};
    
    // Abre a fonte para score/combo
    s_gameState.font = TTF_OpenFont("assets/font/pixelFont.ttf", 48);
    if (!s_gameState.font) {
        printf("Erro ao abrir fonte: %s\n", TTF_GetError());
        return 0;
    }

    // Pré-renderizar texturas que não mudam ou mudam pouco
    SDL_Color black = {0, 0, 0, 255};
    const char* keys[] = {"Z", "X", "C"};
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
    
    // Inicia a música
    Mix_PlayMusic(s_gameState.faseAtual->musica, 0);
    s_gameState.musicStartTime = SDL_GetTicks();

    return 1;
}

void Game_HandleEvent(SDL_Event* e) {
    
    if (s_gameState.isGameOver){
        // Conseguir fechar o jogo quando der GameOver, por enquanto deixarei assim.
        if (e->type == SDL_QUIT || (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE)) {
            s_gameState.gameIsRunning = false;
        }
        return;
    }

    if (s_gameState.isPaused) {
        // Permite sair do jogo mesmo estando pausado
        if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
            s_gameState.gameIsRunning = 0;
        }
        else if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_p) {
            Uint32 pausedDuration = SDL_GetTicks() - s_gameState.pauseStartTime;
            s_gameState.musicStartTime += pausedDuration;
            Mix_ResumeMusic();
            s_gameState.isPaused = !s_gameState.isPaused;
        }
        return;
    }

    switch (e->type) {
        case SDL_QUIT: {
            s_gameState.gameIsRunning = false;
            break;
        }
        case SDL_KEYDOWN: {
            SDL_Keycode teclaPressionada = e->key.keysym.sym;

            //Necessário para poder deixar a tecla pressionada, e ajustado para detectar se for erro ou não.
             if (e->key.repeat != 0) {
                bool isCurrentlyHolding = false;
                // Verifica se o jogador já está no estado NOTA_SEGURANDO para esta tecla
                for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                    Nota* nota = &s_gameState.faseAtual->beatmap[i];
                    if (nota->estado == NOTA_SEGURANDO && nota->tecla == teclaPressionada) {
                        isCurrentlyHolding = true;
                        break;
                    }
                }
                // Se estiver segurando corretamente uma nota longa, ignora o evento de repetição.
                if (isCurrentlyHolding) {
                    break; 
                }
            }

            // Identifica qual checker foi ativado
            int checkerIndex = -1;
            switch (teclaPressionada) {
                case SDLK_p:{
                    s_gameState.pauseStartTime = SDL_GetTicks();
                    s_gameState.isPaused = !s_gameState.isPaused;
                    Mix_PauseMusic(); // Pausa a música
                }return;
                case SDLK_ESCAPE:{
                    s_gameState.gameIsRunning = false; return;
                } 
                case SDLK_z: {
                    checkerIndex = 0; break;
                }     
                case SDLK_x: {
                    checkerIndex = 1; break;
                }      
                case SDLK_c:{
                    checkerIndex = 2; break;
                }      
                case SDLK_SPACE: {
                    if (s_gameState.specialMeter >= 100.0f && !s_gameState.isSpecialActive) {
                        s_gameState.isSpecialActive = true;
                        s_gameState.specialTimer = SPECIAL_DURATION;
                        printf("ESPECIAL ATIVADO!\n");
                    }
                    return;
                }
                default: {
                    // Se não for uma tecla do jogo, encerra o evento.
                    return;
                } 
            }

            // Com o checker identificado, executa a lógica principal
            Checker* checker = &s_gameState.checkers[checkerIndex];

            // Ativa o highlight visual do checker
            checker->isPressedTimer = 0.15f;

            bool acertouNota = false;

            // Procura por uma nota correspondente para o checker ativado
            for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                Nota* nota = &s_gameState.faseAtual->beatmap[i];

                // Pula notas que não estão ativas ou não são para esta tecla
                if (nota->estado != NOTA_ATIVA || nota->tecla != checker->tecla) {
                    continue;
                }

                // Calcula a distância usando a posição do checker que já temos
                float dist = fabsf(nota->pos.x - checker->rect.x);

                if (dist <= HIT_WINDOW_OK) { // Checa a janela mais larga primeiro
                    //Diferenciar nota normal da nota longa
                    if (nota->duration > 0) {
                        nota->estado = NOTA_SEGURANDO;
                    } else {
                        nota->estado = NOTA_ATINGIDA;
                    }
                    acertouNota = true;
                    s_gameState.combo++;

                    if (s_gameState.combo > 0 && s_gameState.combo % 50 == 0) {
                        s_gameState.comboPulseTimer = 0.3f;
                    }

                    int points = 0;
                    
                    // Definir a precisão
                    if (dist <= HIT_WINDOW_OTIMO) {
                        // Recompensas de "Ótimo"
                        points = 20;
                        s_gameState.health += 2.0f;
                        if (!s_gameState.isSpecialActive) { s_gameState.specialMeter += 0.75f + (s_gameState.combo * 0.1f); }
                        SpawnFeedbackText(0, checker->rect); // 0 = Ótimo
                    } 
                    else if (dist <= HIT_WINDOW_BOM) {
                        // Recompensas de "Bom"
                        points = 10;
                        s_gameState.health += 1.0f;
                        if (!s_gameState.isSpecialActive) { s_gameState.specialMeter += 0.5f + (s_gameState.combo * 0.1f); }
                        SpawnFeedbackText(1, checker->rect); // 1 = Bom
                    } 
                    else {
                        //Recompensas de "Ok"
                        points = 2;
                        s_gameState.health += 0.5f; // Recupera menos vida
                        if (!s_gameState.isSpecialActive) { s_gameState.specialMeter += 0.25f; } // Quase não ganha especial
                        SpawnFeedbackText(2, checker->rect); // 2 = Ok
                    }
                    
                    // Limita a vida e o especial para não passarem de 100
                    if (s_gameState.health > 100.0f) s_gameState.health = 100.0f;
                    if (s_gameState.specialMeter > 100.0f) s_gameState.specialMeter = 100.0f;
                    
                    // Aplica os multiplicadores de combo e especial
                    if (nota->duration > 0) {
                        // NOTA LONGA: Apenas muda o estado. NENHUM PONTO É ADICIONADO AQUI.
                        nota->estado = NOTA_SEGURANDO;
                    } else {
                        // NOTA NORMAL: Pontuação é calculada e adicionada agora.
                        nota->estado = NOTA_ATINGIDA;
                        points *= (s_gameState.combo > 0 ? s_gameState.combo : 1);
                        if (s_gameState.isSpecialActive) points *= 2;
                        s_gameState.score += points;
                    }
                    
                    break;
                }
            }

            // Se o loop terminou e nenhuma nota foi acertada, foi um erro
            if (!acertouNota) {
                s_gameState.combo = 0;
                // Perde vida ao errar
                s_gameState.health -= 5.0f; // Perde 5% de vida
                if (s_gameState.health < 0.0f) s_gameState.health = 0.0f;

                printf("ERROU! Combo resetado.\n");
            }
            break;
        }

        //Case para quando soltar a tecla pressionada
        case SDL_KEYUP: {
            if (e->key.repeat != 0) break;
            SDL_Keycode teclaSolta = e->key.keysym.sym;

            int checkerIndex = -1;
            if (teclaSolta == SDLK_z) checkerIndex = 0;
            else if (teclaSolta == SDLK_x) checkerIndex = 1;
            else if (teclaSolta == SDLK_c) checkerIndex = 2;
            else break;
            
            Checker* checker = &s_gameState.checkers[checkerIndex];

            // Procura uma nota que estava sendo segurada com esta tecla
            for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
                Nota* nota = &s_gameState.faseAtual->beatmap[i];
                if (nota->estado == NOTA_SEGURANDO && nota->tecla == teclaSolta) {
                    
                    float bodyLength = NOTE_SPEED * (nota->duration / 1000.0f);
                    float tail_pos_x = nota->pos.x + bodyLength;
                    float dist = fabsf(tail_pos_x - checker->rect.x);

                    //Precisão ao soltar a tecla em notas longas
                    if (dist <= HIT_WINDOW_OK) {
                        nota->estado = NOTA_ATINGIDA;
                        s_gameState.combo++; // Bônus de combo por finalizar

                        int points = 0;
                        int feedbackType = 0;
                        
                        if (dist <= HIT_WINDOW_OTIMO) {
                            points = 40; feedbackType = 0; // Ótimo
                        } else if (dist <= HIT_WINDOW_BOM) {
                            points = 20; feedbackType = 1; // Bom
                        } else {
                            points = 5; feedbackType = 2; // Ok
                        }
                        
                        // Pontuação completa da nota longa é calculada e aplicada aqui (Se não soltou corretamente, não ganha ponto algum)
                        points *= (s_gameState.combo > 0 ? s_gameState.combo : 1);
                        if (s_gameState.isSpecialActive) points *= 2;
                        s_gameState.score += points;

                        SpawnFeedbackText(feedbackType, checker->rect);

                    } else { // Soltou fora da janela de acerto
                        nota->estado = NOTA_QUEBRADA;
                        nota->despawnTimer = 0.5f;
                        s_gameState.combo = 0;
                        s_gameState.health -= 2.0f;
                    }
                    break;
                }
            }
        } break;
    } 
}


void Game_Update(float deltaTime) {
    if (s_gameState.isPaused) return;
    if (s_gameState.isGameOver) return;
    if (!s_gameState.gameIsRunning || Mix_PlayingMusic() == 0) return;

    //Uint32 tempoAtual = (Uint32)(Mix_GetMusicPosition(s_faseAtual->musica) * 1000.0);
    Uint32 tempoAtual = SDL_GetTicks() - s_gameState.musicStartTime;

    // Lógica do Especial e dos Confetes
    if (s_gameState.isSpecialActive) {
        s_gameState.specialTimer -= deltaTime;

        // Atualiza o medidor para descer visualmente
        s_gameState.specialMeter = (s_gameState.specialTimer / SPECIAL_DURATION) * 100.0f;
        
        // Cria 2 partículas por frame durante o especial
        SpawnConfettiParticle();
        SpawnConfettiParticle();

        if (s_gameState.specialTimer <= 0) {
            s_gameState.isSpecialActive = false;
            printf("Especial acabou.\n");
        }
    }

    // Atualiza todas as partículas de confete ativas
    for (int i = 0; i < MAX_CONFETTI; ++i) {
        if (s_gameState.confetti[i].isActive) {
            ConfettiParticle* p = &s_gameState.confetti[i];
            p->lifetime -= deltaTime;
            if (p->lifetime <= 0) {
                p->isActive = false;
                continue;
            }
            p->pos.x += p->velocity.x * deltaTime;
            p->pos.y += p->velocity.y * deltaTime;
            p->velocity.y += 300.0f * deltaTime;
        }
    }

    // Spawner de notas
    if (s_gameState.faseAtual->proximaNotaIndex < s_gameState.faseAtual->totalNotas) {
        Nota* proxima = &s_gameState.faseAtual->beatmap[s_gameState.faseAtual->proximaNotaIndex];
        if (tempoAtual >= proxima->spawnTime) {
            proxima->estado = NOTA_ATIVA;
            s_gameState.faseAtual->proximaNotaIndex++;
        }
    }

    // Atualizar notas
    for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
        Nota* nota = &s_gameState.faseAtual->beatmap[i];
        
        // Atualiza a posição apenas se a nota estiver em movimento
        Note_Update(nota, deltaTime);

        float checker_pos_x = 0;
        for(int j=0; j < 3; ++j) {
            if (s_gameState.checkers[j].tecla == nota->tecla) {
                checker_pos_x = s_gameState.checkers[j].rect.x;
                break;
            }
        }
        
        // Lógica de falha e limpeza
        
        // Checa se uma nota ativa foi perdida (passou do checker)
        if (nota->estado == NOTA_ATIVA) {
            if (nota->pos.x < (checker_pos_x - HIT_WINDOW_OK)) {
                nota->estado = NOTA_INATIVA; 
                s_gameState.combo = 0;
                s_gameState.health -= 5.0f;
                if (s_gameState.health < 0.0f) s_gameState.health = 0.0f;
                printf("ERROU (Nota perdida)! Combo resetado.\n");
            }
        } 
        // Checa se o jogador segurou uma nota longa por tempo demais
        else if (nota->estado == NOTA_SEGURANDO) {
            float bodyLength = NOTE_SPEED * (nota->duration / 1000.0f);
            float tail_pos_x = nota->pos.x + bodyLength;

            if (tail_pos_x < (checker_pos_x - HIT_WINDOW_OK)) {
                nota->estado = NOTA_INATIVA;
                s_gameState.combo = 0;
                s_gameState.health -= 2.0f;
                printf("ERROU (Não soltou a tempo)! Combo resetado.\n");
            }
        }

        // Processa o timer de desaparecimento 
        if (nota->despawnTimer > 0) {
            nota->despawnTimer -= deltaTime;
            if (nota->despawnTimer <= 0) {
                nota->estado = NOTA_INATIVA;
            }
        }
    }

    // Atualiza os timers de feedback dos checkers
    for (int i = 0; i < 3; ++i) {
        if (s_gameState.checkers[i].isPressedTimer > 0) {
            s_gameState.checkers[i].isPressedTimer -= deltaTime;
        }
    }

    // Atualiza timers de feedback
    if (s_gameState.comboPulseTimer > 0) {
        s_gameState.comboPulseTimer -= deltaTime;
    }

    for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
        if (s_gameState.feedbackTexts[i].isActive) {
            FeedbackText* ft = &s_gameState.feedbackTexts[i];
            ft->lifetime -= deltaTime;
            if (ft->lifetime <= 0) {
                ft->isActive = false;
            } else {
                // Efeito de subir e sumir
                ft->pos.y -= 50.0f * deltaTime;
            }
        }
    }

    // Checagem de Game Over
    if (s_gameState.health <= 0 && !s_gameState.isGameOver) {
        s_gameState.isGameOver = true;
        Mix_HaltMusic(); // Para a música Principal

        // Toca o som de falha
        if (s_gameState.failSound) {
            Mix_PlayChannel(-1, s_gameState.failSound, 0);
        }

        printf("GAME OVER!\n");
    }
}


void Game_Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Desenha Background Principal
    SDL_RenderCopy(renderer, s_gameState.faseAtual->background, NULL, NULL);

    // Desenha a pista de ritmo
    SDL_Rect trackRect = {RHYTHM_TRACK_POS_X, RHYTHM_TRACK_POS_Y, RHYTHM_TRACK_WIDTH, RHYTHM_TRACK_HEIGHT};
    SDL_RenderCopy(renderer, s_gameState.faseAtual->rhythmTrack, NULL, &trackRect);
    rectangleRGBA(renderer, trackRect.x, trackRect.y, trackRect.x + trackRect.w, trackRect.y + trackRect.h, 255, 255, 255, 180); 

    // Render checkers
    for (int i = 0; i < 3; ++i) {
        Checker* checker = &s_gameState.checkers[i];

        // Calcula o centro do checker para desenhar círculos
        Sint16 centerX = checker->rect.x + (checker->rect.w / 2);
        Sint16 centerY = checker->rect.y + (checker->rect.h / 2);
        Sint16 radius = checker->rect.w / 2; // Raio do círculo

        // Define a área de desenho para a textura do contorno
        SDL_Rect contourDstRect = { checker->rect.x - CHECKER_CONTOUR_OFFSET_X, checker->rect.y - CHECKER_CONTOUR_OFFSET_Y, CHECKER_CONTOUR_WIDTH, CHECKER_CONTOUR_HEIGHT };

        // Desenha a imagem do contorno primeiro, para que o preenchimento fique por cima
        SDL_RenderCopy(renderer, s_gameState.checkerContornoTex[i], NULL, &contourDstRect);

        // Desenha o preenchimento interno do checker
        if (checker->isPressedTimer > 0) {
            // Preenchimento branco e mais visível no highlight
            filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 150);
        } else {
            // Preenchimento padrão, mais sutil
            filledCircleRGBA(renderer, centerX, centerY, radius, 255, 255, 255, 70);
        }

        // Desenha o texto da tecla (Z, X, C) no centro do círculo
        // Usa a textura pré-renderizada da tecla
        if (s_gameState.checkerKeyTex[i]) {
            int w, h;
            SDL_QueryTexture(s_gameState.checkerKeyTex[i], NULL, NULL, &w, &h);
            SDL_Rect textRect = { centerX - w / 2, centerY - h / 2, w, h };
            SDL_RenderCopy(renderer, s_gameState.checkerKeyTex[i], NULL, &textRect);
        }
    }

    // Render notas
    for (int i = 0; i < s_gameState.faseAtual->totalNotas; ++i) {
        Nota* nota = &s_gameState.faseAtual->beatmap[i];

        float checker_pos_x = 0;
        if (nota->tecla == SDLK_z) checker_pos_x = CHECKER_Z_X;
        else if (nota->tecla == SDLK_x) checker_pos_x = CHECKER_X_X;
        else if (nota->tecla == SDLK_c) checker_pos_x = CHECKER_C_X;
        
        Note_Render(nota, renderer, checker_pos_x);
    }
    
    // Renderiza os confetes (antes da UI)
    for (int i = 0; i < MAX_CONFETTI; ++i) {
        if (s_gameState.confetti[i].isActive) {
            ConfettiParticle* p = &s_gameState.confetti[i];
            boxRGBA(renderer, (Sint16)p->pos.x, (Sint16)p->pos.y, (Sint16)(p->pos.x + p->pos.w), (Sint16)(p->pos.y + p->pos.h), p->color.r, p->color.g, p->color.b, p->color.a);
        }
    }

    // Renderiza o texto de feedback (Ótimo/Bom/Ok)
    for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
        if (s_gameState.feedbackTexts[i].isActive) {
            FeedbackText* ft = &s_gameState.feedbackTexts[i];
            SDL_Texture* tex = s_gameState.feedbackTextures[ft->type];
            if (tex) {
                int w, h;
                SDL_QueryTexture(tex, NULL, NULL, &w, &h);
                // Efeito de fade-out
                Uint8 alpha = (Uint8)(255.0f * (ft->lifetime / 0.6f));
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_Rect dst = { (int)ft->pos.x - w / 2, (int)ft->pos.y, w, h };
                SDL_RenderCopy(renderer, tex, NULL, &dst);
                SDL_SetTextureAlphaMod(tex, 255); // Reseta o alpha para o próximo quadro
            }
        }
    }
    
    // Renderizar score e combo
    // Atualiza as texturas de placar/combo apenas quando necessário
    UpdateTextureCache(renderer);

    if (s_gameState.scoreTexture.texture) {
        SDL_Rect dst = {20, 20, s_gameState.scoreTexture.w, s_gameState.scoreTexture.h};
        SDL_RenderCopy(renderer, s_gameState.scoreTexture.texture, NULL, &dst);
    }

    if (s_gameState.combo > 1 && s_gameState.comboTexture.texture) {
        // Lógica do "pulso"
        float scale = 1.0f;
        if (s_gameState.comboPulseTimer > 0) {
            scale = 1.0f + 0.5f * (s_gameState.comboPulseTimer / 0.3f);
        }
        int w = (int)(s_gameState.comboTexture.w * scale);
        int h = (int)(s_gameState.comboTexture.h * scale);
        SDL_Rect dst = { SCREEN_WIDTH / 2 - w / 2, 100, w, h }; // Centraliza
        SDL_RenderCopy(renderer, s_gameState.comboTexture.texture, NULL, &dst);
    }
    
    // Renderizar Barra de Vida
    int barWidth = 400, barHeight = 20, barX = (SCREEN_WIDTH / 2) - (barWidth / 2), barY = 20;
    int currentHealthWidth = (int)((s_gameState.health / 100.0f) * barWidth);
    SDL_Color healthColor = {50, 205, 50, 255}; // Verde
    if (s_gameState.health < 50) healthColor = (SDL_Color){255, 215, 0, 255}; // Amarelo
    if (s_gameState.health < 25) healthColor = (SDL_Color){220, 20, 60, 255}; // Vermelho

    // Desenha o fundo da barra
    boxRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 50, 50, 50, 200);

    // Desenha o preenchimento da vida
    if (currentHealthWidth > 0) boxRGBA(renderer, barX, barY, barX + currentHealthWidth, barY + barHeight, healthColor.r, healthColor.g, healthColor.b, 255);
    
    // Desenha a borda da barra
    rectangleRGBA(renderer, barX, barY, barX + barWidth, barY + barHeight, 255, 255, 255, 255);
    
    // Renderizar Barra de Especial
    int specialBarWidth = 400;
    int specialBarHeight = 15;
    int specialBarX = (SCREEN_WIDTH / 2) - (specialBarWidth / 2);
    int specialBarY = RHYTHM_TRACK_POS_Y + RHYTHM_TRACK_HEIGHT + 10; // Posiciona a barra abaixo da pista de ritmo
    int currentSpecialWidth = (int)((s_gameState.specialMeter / 100.0f) * specialBarWidth);
    SDL_Color specialColor = {0, 191, 255, 255}; // Azul
    if (s_gameState.specialMeter >= 100.0f){
        specialColor = (SDL_Color){255, 223, 0, 255}; // Dourado
    } 
    
    // Desenha o fundo e a borda
    boxRGBA(renderer, specialBarX, specialBarY, specialBarX + specialBarWidth, specialBarY + specialBarHeight, 50, 50, 50, 200);
    
    // Desenha o preenchimento
    if (currentSpecialWidth > 0) boxRGBA(renderer, specialBarX, specialBarY, specialBarX + currentSpecialWidth, specialBarY + specialBarHeight, specialColor.r, specialColor.g, specialColor.b, 255);
    rectangleRGBA(renderer, specialBarX, specialBarY, specialBarX + specialBarWidth, specialBarY + specialBarHeight, 255, 255, 255, 255);

    // Mensagem de Game Over
    if (s_gameState.isGameOver && s_gameState.font) {
        SDL_Color red = {255, 0, 0, 255};
        SDL_Surface* surfGO = TTF_RenderText_Blended(s_gameState.font, "FIM DE JOGO", red);
        SDL_Texture* texGO = SDL_CreateTextureFromSurface(renderer, surfGO);
        SDL_Rect dstGO = { SCREEN_WIDTH / 2 - surfGO->w / 2, SCREEN_HEIGHT / 2 - surfGO->h / 2, surfGO->w, surfGO->h };
        SDL_RenderCopy(renderer, texGO, NULL, &dstGO);
        SDL_FreeSurface(surfGO);
        SDL_DestroyTexture(texGO);
    }

    // Desenha a tela de PAUSE se o jogo estiver pausado ---
    if (s_gameState.isPaused && s_gameState.font) {
        // 1. Desenha uma camada escura semi-transparente sobre o jogo
        boxRGBA(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);

        // 2. Desenha o texto "PAUSE" no centro
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surfPause = TTF_RenderText_Blended(s_gameState.font, "PAUSE", white);
        if (surfPause) {
            SDL_Texture* texPause = SDL_CreateTextureFromSurface(renderer, surfPause);
            SDL_Rect dstPause = {
                SCREEN_WIDTH / 2 - surfPause->w / 2,
                SCREEN_HEIGHT / 2 - surfPause->h / 2,
                surfPause->w,
                surfPause->h
            };
            SDL_RenderCopy(renderer, texPause, NULL, &dstPause);
            SDL_FreeSurface(surfPause);
            SDL_DestroyTexture(texPause);
        }
    }

    SDL_RenderPresent(renderer);
}

// Função para gerenciar o cache de texturas
static void UpdateTextureCache(SDL_Renderer* renderer) {
    
    // Cache do Placar
    if (s_gameState.score != s_gameState.cachedScore) {
        s_gameState.cachedScore = s_gameState.score;
        char buffer[64];
        sprintf(buffer, "Pontos: %d", s_gameState.score);
        
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surf = TTF_RenderText_Blended(s_gameState.font, buffer, white);
        if (surf) {
            if (s_gameState.scoreTexture.texture) SDL_DestroyTexture(s_gameState.scoreTexture.texture);
            s_gameState.scoreTexture.texture = SDL_CreateTextureFromSurface(renderer, surf);
            s_gameState.scoreTexture.w = surf->w;
            s_gameState.scoreTexture.h = surf->h;
            SDL_FreeSurface(surf);
        }
    }

    // Cache do Combo
    if (s_gameState.combo != s_gameState.cachedCombo) {
        s_gameState.cachedCombo = s_gameState.combo;
        char buffer[64];
        sprintf(buffer, "Combo: %d", s_gameState.combo);
        
        SDL_Color gold = {255, 223, 0, 255};
        SDL_Surface* surf = TTF_RenderText_Blended(s_gameState.font, buffer, gold);
        if (surf) {
            if (s_gameState.comboTexture.texture) SDL_DestroyTexture(s_gameState.comboTexture.texture);
            s_gameState.comboTexture.texture = SDL_CreateTextureFromSurface(renderer, surf);
            s_gameState.comboTexture.w = surf->w;
            s_gameState.comboTexture.h = surf->h;
            SDL_FreeSurface(surf);
        }
    }
}


static void SpawnConfettiParticle() {
    for (int i = 0; i < MAX_CONFETTI; ++i) {
        if (!s_gameState.confetti[i].isActive) {
            ConfettiParticle* p = &s_gameState.confetti[i];
            p->isActive = true;
            p->lifetime = 2.0f + (rand() % 100) / 100.0f; // Vida de 2 a 3 segundos

            // Posição inicial (nasce na lateral esquerda ou direita)
            p->pos.x = (rand() % 2 == 0) ? -10.0f : SCREEN_WIDTH + 10.0f;
            p->velocity.x = (p->pos.x < 0) ? (100 + (rand() % 100)) : (-100 - (rand() % 100)); // Velocidade correspondente
            p->pos.y = rand() % SCREEN_HEIGHT;
            p->velocity.y = -100 - (rand() % 150); // Velocidade para cima

            // Tamanho e cor aleatórios
            p->pos.w = 5 + (rand() % 5);
            p->pos.h = p->pos.w;
            p->color = (SDL_Color){100 + rand() % 156, 100 + rand() % 156, 100 + rand() % 156, 255};
            
            break; // Sai do loop após criar uma partícula
        }
    }
}

static void SpawnFeedbackText(int type, SDL_Rect checkerRect) {
    for (int i = 0; i < MAX_FEEDBACK_TEXTS; ++i) {
        if (!s_gameState.feedbackTexts[i].isActive) {
            FeedbackText* ft = &s_gameState.feedbackTexts[i];

            // Define o conteúdo do texto
            ft->isActive = true;
            ft->type = type;
            ft->lifetime = 0.6f;

            // Lógica de Posicionamento e Anti-Sobreposição
            // Posição inicial: centralizado acima do checker
            float startY = checkerRect.y - 20.0f;
            
            bool positionFound = false;
            while (!positionFound) {
                positionFound = true;
                // Verifica se a posição Y inicial já está ocupada
                for (int j = 0; j < MAX_FEEDBACK_TEXTS; ++j) {
                    if (i == j || !s_gameState.feedbackTexts[j].isActive) continue;

                    // Se outro texto estiver muito perto verticalmente
                    if (fabsf(s_gameState.feedbackTexts[j].pos.y - startY) < 25.0f) {
                        startY -= 25.0f; // Empurra o novo texto para cima
                        positionFound = false; // Tenta a nova posição
                        break;
                    }
                }
            }

            // Define a posição final, já corrigida
            ft->pos.x = checkerRect.x + (checkerRect.w / 2.0f);
            ft->pos.y = startY;
            
            break; // Sai do loop após encontrar um espaço
        }
    }
}

void Game_Shutdown() {
    Fase_Liberar(s_gameState.faseAtual);

    // Libera as texturas dos contornos
    for (int i = 0; i < 3; i++) {
        if (s_gameState.checkerContornoTex[i]) SDL_DestroyTexture(s_gameState.checkerContornoTex[i]);
        if (s_gameState.checkerKeyTex[i]) SDL_DestroyTexture(s_gameState.checkerKeyTex[i]);
    }
    
    // Libera as texturas de feedback pré-renderizadas
    for (int i = 0; i < 3; i++) {
        if(s_gameState.feedbackTextures[i]) SDL_DestroyTexture(s_gameState.feedbackTextures[i]);
    }

    SDL_DestroyTexture(s_gameState.hitSpritesheet);

    // Libera o som de falha
    if (s_gameState.failSound) {
        Mix_FreeChunk(s_gameState.failSound);
    }

    // Libera texturas cacheadas
    if (s_gameState.scoreTexture.texture) SDL_DestroyTexture(s_gameState.scoreTexture.texture);
    if (s_gameState.comboTexture.texture) SDL_DestroyTexture(s_gameState.comboTexture.texture);

    // Libera a memória alocada com calloc
    free(s_gameState.confetti);
    free(s_gameState.feedbackTexts);
    
    if (s_gameState.font) {
        TTF_CloseFont(s_gameState.font);
    }
    TTF_Quit();
}

int Game_IsRunning() {
    return s_gameState.gameIsRunning;
}