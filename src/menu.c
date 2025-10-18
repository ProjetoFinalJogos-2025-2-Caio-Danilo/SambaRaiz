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
#include <SDL2/SDL_mixer.h>

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
static TTF_Font* s_fontSmall = NULL;   //fonte menor p/ instruções
static MenuScreen s_currentScreen = MENU_SCREEN_MAIN;
static int s_selectedButton = 0;

static SongInfo s_songList[50]; // Suporta até 50 músicas
static int s_songCount = 0;

//Hitboxes dos botões do menu
static SDL_Rect s_btnRects[4];

// Preview de música (estado)
static Mix_Music* s_previewMusic = NULL;
static int        s_previewPlayingIndex = -1;

// Verifica se um ponto (mx, my) está dentro de um retângulo r
static bool ptInRect(int mx, int my, SDL_Rect r) {
    return (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h);
}

//Retângulo clicável na tela de seleção de musicas
static SDL_Rect GetSongRowRect(int index) {
    const int listStartY = 250;      
    const int listLineH  = 60;       
    const int left       = 80;       
    const int right      = SCREEN_WIDTH - 80; 

    SDL_Rect r = { left, listStartY + index*listLineH - 28, right - left, 56 };
    return r;
}

//Controle do preview
static void Menu_StopPreview(void) {
    if (Mix_PlayingMusic()) Mix_HaltMusic();
    if (s_previewMusic) { Mix_FreeMusic(s_previewMusic); s_previewMusic = NULL; }
    s_previewPlayingIndex = -1;
}
static void Menu_PlayPreview(int idx) {
    if (idx < 0 || idx >= s_songCount) return;
    // Toca o .mp3 correspondente ao .samba (mesmo ID)
    // Mapeamento simples: troca a extensão e a pasta
    // Ex.: assets/beatMaps/meu_lugar.samba -> assets/music/MeuLugar_ArlindoCruz.mp3 (ajuste abaixo se quiser nomes exatos)
    // Aqui vamos construir um caminho padrão "assets/music/<id>.mp3" se existir.
    const char* mp3Path = NULL;
    if (strstr(s_songList[idx].fileName, "meu_lugar")) {
        mp3Path = "assets/music/MeuLugar_ArlindoCruz.mp3";
    } else if (strstr(s_songList[idx].fileName, "do_fundo_do_nosso_quintal")) {
        mp3Path = "assets/music/DoFundoDoNossoQuintal_JorgeAragao.mp3";
    }

    if (!mp3Path) return;

    // Se já está tocando essa mesma, pausar
    if (s_previewPlayingIndex == idx && Mix_PlayingMusic()) {
        Menu_StopPreview();
        return;
    }

    Menu_StopPreview();
    s_previewMusic = Mix_LoadMUS(mp3Path);
    if (s_previewMusic) {
        Mix_PlayMusic(s_previewMusic, 0);   // toca 1x (mude p/ -1 se quiser loop)
        s_previewPlayingIndex = idx;
    }
}

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

    //abre uma fonte menor só para textos longos/rodapé
    s_fontSmall = TTF_OpenFont("assets/font/pixelFont.ttf", 28);
    if (!s_fontSmall) return false;
    
    Menu_LoadSongs();
    s_currentScreen = MENU_SCREEN_MAIN;
    s_selectedButton = 1; // Começa em "Músicas"

    // Calcula as hitboxes dos botões (coordenadas batendo com o RenderText)
    {
        const int w = 420, h = 58;
        const int x = SCREEN_WIDTH / 2 - w / 2;
        for (int i = 0; i < 4; ++i) {
            const int y = 300 + i * 80 - h / 2;
            s_btnRects[i] = (SDL_Rect){ x, y, w, h };
        }
    }

    //garante que não há preview tocando ao entrar
    Menu_StopPreview();

    return true;
}

// Libera os recursos do menu
static void Menu_Shutdown() {
    SDL_DestroyTexture(s_background);
    TTF_CloseFont(s_font);
    if (s_fontSmall) TTF_CloseFont(s_fontSmall);
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
                 Menu_StopPreview();       //para preview ao sair
                s_currentScreen = MENU_SCREEN_MAIN;
                s_selectedButton = 1;
            }
            if (key == SDLK_SPACE) {      //play/pause preview
                Menu_PlayPreview(s_selectedButton);
            }
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) { // Selecionou uma música
                 Menu_StopPreview();       //para garantir que o gameplay não sobreponha preview
                strcpy(selectedSongPath, s_songList[s_selectedButton].fileName);
                return APP_STATE_GAMEPLAY;
            }
        }
    }

    //Mouse: hover + click apenas para "Músicas" (1) e "Sair" (3) na tela principal
    if (s_currentScreen == MENU_SCREEN_MAIN &&
        (e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN)) {

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // hover atualiza a seleção visual (somente botões habilitados)
        if (ptInRect(mx, my, s_btnRects[1])) {
            s_selectedButton = 1; // Músicas
        } else if (ptInRect(mx, my, s_btnRects[3])) {
            s_selectedButton = 3; // Sair
        }

        // clique esquerdo executa ação
        if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
            if (ptInRect(mx, my, s_btnRects[1])) {
                s_currentScreen = MENU_SCREEN_SONG_SELECT;
                s_selectedButton = 0; // primeira música
            } else if (ptInRect(mx, my, s_btnRects[3])) {
                return APP_STATE_EXIT;
            }
        }
    }

    //Mouse: hover + click na TELA DE SELEÇÃO DE MÚSICAS
    if (s_currentScreen == MENU_SCREEN_SONG_SELECT &&
        (e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN)) {

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        for (int i = 0; i < s_songCount; ++i) {
            SDL_Rect row = GetSongRowRect(i);
            if (ptInRect(mx, my, row)) {
                s_selectedButton = i; // hover destaca a linha
                if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
                   // clique toca/pausa o preview
                    Menu_PlayPreview(i); 
                }
                break; // já achou a linha sob o mouse
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
            RenderText(renderer, s_font, buttons[i], SCREEN_WIDTH / 2, 300 + i * 80, color, TEXT_ALIGN_CENTER);
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

            // indicador ▶ na música em preview
            char nameBuf[192];
            if (i == s_previewPlayingIndex && Mix_PlayingMusic())
                snprintf(nameBuf, sizeof(nameBuf), "> %s", s_songList[i].displayName);
            else
                snprintf(nameBuf, sizeof(nameBuf), "   %s", s_songList[i].displayName);

            // 1. Desenha o nome da música, alinhado à ESQUERDA
            RenderText(renderer, s_font, nameBuf, nameX, y_pos, color, TEXT_ALIGN_LEFT);
      
            // 2. Desenha o recorde, alinhado à DIREITA
            char recordBuffer[128];
            if (s_songList[i].topScore.score > 0) {
                sprintf(recordBuffer, "%s - %d", s_songList[i].topScore.name, s_songList[i].topScore.score);
            } else {
                sprintf(recordBuffer, "Sem recorde");
            }
            RenderText(renderer, s_font, recordBuffer, scoreX, y_pos, color, TEXT_ALIGN_RIGHT);
        }

        const int instrY1 = 440;
        const int instrY2 = 480;
        RenderText(renderer, s_fontSmall, "Barra de Espaco / Clique: Preview",
           SCREEN_WIDTH/2, instrY1, white, TEXT_ALIGN_CENTER);
        RenderText(renderer, s_fontSmall, "Enter: Iniciar    ->   ESC: Voltar",
           SCREEN_WIDTH/2, instrY2, white, TEXT_ALIGN_CENTER);
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