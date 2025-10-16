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
    // NOTA: Ler diretórios em C pode ser complexo e dependente de plataforma.
    // Para simplificar, lista de músicas ficará hardcoded por enquanto.
    
    strcpy(s_songList[0].fileName, "assets/beatMaps/meu_lugar.samba");
    strcpy(s_songList[0].displayName, "Meu Lugar");

    strcpy(s_songList[1].fileName, "assets/beatMaps/do_fundo_do_nosso_quintal.samba");
    strcpy(s_songList[1].displayName, "Do Fundo do Nosso Quintal");
    
    s_songCount = 2;
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
        RenderText(renderer, s_font, "Escolha uma Musica", SCREEN_WIDTH / 2, 100, gold, true);
        for (int i = 0; i < s_songCount; ++i) {
            SDL_Color color = (s_selectedButton == i) ? gold : white;
            RenderText(renderer, s_font, s_songList[i].displayName, SCREEN_WIDTH / 2, 250 + i * 60, color, true);
            // Aqui você renderizaria o recorde (s_songList[i].topScore) ao lado
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