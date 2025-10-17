// Em menu.c
#include "menu.h"
#include "defs.h"
#include "leaderboard.h"
#include "game.h"
#include "auxFuncs/utils.h"
#include "auxFuncs/auxWaitEvent.h"
#include <dirent.h> 
#include <SDL2/SDL_image.h> 
#include <SDL2/SDL_ttf.h>

// Estados internos do menu
typedef enum {
    MENU_SCREEN_MAIN,
    MENU_SCREEN_SONG_SELECT
} MenuScreen;

// Estrutura para guardar informações de uma música encontrada
typedef struct {
    char fileName[256];
    char displayName[128];
    HighScore topScore;
} SongInfo;

// Variáveis estáticas do menu
static SDL_Texture* s_background = NULL;
static TTF_Font* s_font = NULL;
static MenuScreen s_currentScreen = MENU_SCREEN_MAIN;
static int s_selectedButton = 0;

static SongInfo s_songList[50]; // Suporta até 50 músicas
static int s_songCount = 0;

// Carrega a lista de músicas e seus recordes
static void Menu_LoadSongs() {
    // 1. Carrega a base de dados de todos os recordes do arquivo
    LeaderboardData leaderboardData;
    Leaderboard_Load(&leaderboardData);
    
    // 2. Define a lista de músicas do jogo. O primeiro nome é o "ID" para o ranking E para o nome do arquivo.
    const char* song_database[][2] = {
        {"meu_lugar", "Meu Lugar"},
        {"do_fundo_do_nosso_quintal", "Do Fundo do Nosso Quintal"} 
    };
    s_songCount = sizeof(song_database) / sizeof(song_database[0]);
    // 3. Itera sobre a lista de músicas do jogo para preencher a s_songList
    for (int i = 0; i < s_songCount; ++i) {
        // Pega o nome de exibição da nossa base de dados
        strcpy(s_songList[i].displayName, song_database[i][1]);

        // Gera o caminho completo do arquivo a partir do ID
        sprintf(s_songList[i].fileName, "assets/beatMaps/%s.samba", song_database[i][0]);

        // Procura o recorde para esta música usando o ID
        bool foundRecord = false;
        for (int j = 0; j < leaderboardData.songCount; ++j) {
            // Compara o ID da música com o nome salvo no leaderboard
            if (strcmp(leaderboardData.songLeaderboards[j].songName, song_database[i][0]) == 0) {
                 // Encontrou! Copia o recorde (nome e pontuação)
                 s_songList[i].topScore = leaderboardData.songLeaderboards[j].scores[0];
                 foundRecord = true;
                 break; // Para de procurar, pois já achou a música
            }
        }

        // Se, após o loop, não encontrou um recorde para a música, define os valores padrão
        if (!foundRecord) {
            strcpy(s_songList[i].topScore.name, "---");
            s_songList[i].topScore.score = 0;
        }
    }
}
// Inicializa os recursos do menu
static bool Menu_Init(SDL_Renderer* renderer) {
    s_background = IMG_LoadTexture(renderer, "assets/image/menuBG.png");
    s_font = TTF_OpenFont("assets/font/pixelFont.ttf", 48);
    if (!s_background || !s_font) return false;
    
    Menu_LoadSongs();
    s_currentScreen = MENU_SCREEN_MAIN;
    s_selectedButton = 1; // Começa em "Músicas"
    return true;
}

// Libera os recursos do menu
static void Menu_Shutdown() {
    SDL_DestroyTexture(s_background);
    TTF_CloseFont(s_font);
}

// Lida com os inputs do menu
static ApplicationState Menu_HandleEvent(SDL_Event* e, char* selectedSongPath) {
    if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
        SDL_Keycode key = e->key.keysym.sym;
        if (s_currentScreen == MENU_SCREEN_MAIN) {
            if (key == SDLK_UP) s_selectedButton = (s_selectedButton - 1 + 4) % 4;
            if (key == SDLK_DOWN) s_selectedButton = (s_selectedButton + 1) % 4;
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                if (s_selectedButton == 1) { // "Músicas"
                    s_currentScreen = MENU_SCREEN_SONG_SELECT;
                    s_selectedButton = 0; // Reseta para a primeira música
                } else if (s_selectedButton == 3) { // "Sair"
                    return APP_STATE_EXIT;
                }
            }
        } else if (s_currentScreen == MENU_SCREEN_SONG_SELECT) {
            if (key == SDLK_UP) s_selectedButton = (s_selectedButton - 1 + s_songCount) % s_songCount;
            if (key == SDLK_DOWN) s_selectedButton = (s_selectedButton + 1) % s_songCount;
            if (key == SDLK_ESCAPE) { // Voltar
                s_currentScreen = MENU_SCREEN_MAIN;
                s_selectedButton = 1;
            }
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) { // Selecionou uma música
                strcpy(selectedSongPath, s_songList[s_selectedButton].fileName);
                return APP_STATE_GAMEPLAY;
            }
        }
    }
    return APP_STATE_MENU; // Por padrão, continua no menu
}

// Desenha o menu na tela
static void Menu_Render(SDL_Renderer* renderer) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, s_background, NULL, NULL);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gold = {255, 223, 0, 255};
    SDL_Color grey = {100, 100, 100, 255};

    if (s_currentScreen == MENU_SCREEN_MAIN) {
        const char* buttons[] = {"Iniciar", "Musicas", "Configuracoes", "Sair"};
        for (int i = 0; i < 4; ++i) {
            bool isEnabled = (i == 1 || i == 3); // Apenas "Músicas" e "Sair" estão habilitados
            SDL_Color color = (s_selectedButton == i) ? gold : (isEnabled ? white : grey);
            RenderText(renderer, s_font, buttons[i], SCREEN_WIDTH / 2, 300 + i * 80, color, true);
        }
    } else if (s_currentScreen == MENU_SCREEN_SONG_SELECT) {
        RenderText(renderer, s_font, "Escolha uma Musica", SCREEN_WIDTH / 2, 100, gold, TEXT_ALIGN_CENTER);

        // --- Lógica de Renderização da Lista de Músicas com Recordes ---
        int listStartY = 250;     // Posição Y inicial da lista
        int listLineHeight = 60;  // Espaçamento entre as músicas
        int nameX = 150;          // Posição X para o nome da música (alinhado à esquerda)
        int scoreX = SCREEN_WIDTH - 150; // Posição X para o recorde (alinhado à direita)

        for (int i = 0; i < s_songCount; ++i) {
            SDL_Color color = (s_selectedButton == i) ? gold : white;
            int y_pos = listStartY + i * listLineHeight;

            // 1. Desenha o nome da música, alinhado à ESQUERDA
            RenderText(renderer, s_font, s_songList[i].displayName, nameX, y_pos, color, TEXT_ALIGN_LEFT);

            // 2. Desenha o recorde, alinhado à DIREITA
            char recordBuffer[128];
            if (s_songList[i].topScore.score > 0) {
                sprintf(recordBuffer, "%s - %d", s_songList[i].topScore.name, s_songList[i].topScore.score);
            } else {
                sprintf(recordBuffer, "Sem recorde");
            }
            RenderText(renderer, s_font, recordBuffer, scoreX, y_pos, color, TEXT_ALIGN_RIGHT);
        }
    }
    
    SDL_RenderPresent(renderer);
}

// A função principal que será chamada pela main
ApplicationState Menu_Run(SDL_Renderer* renderer, char* selectedSongPath) {
    if (!Menu_Init(renderer)) return APP_STATE_EXIT;

    ApplicationState nextState = APP_STATE_MENU;
    Uint32 timeout = 16;

    while (nextState == APP_STATE_MENU) {
        SDL_Event e;
        while (AUX_WaitEventTimeout(&e, &timeout) != 0) {
            if (e.type == SDL_QUIT) {
                nextState = APP_STATE_EXIT;
                break;
            }
            nextState = Menu_HandleEvent(&e, selectedSongPath);
        }
        Menu_Render(renderer);
    }
    
    Menu_Shutdown();
    return nextState;
}