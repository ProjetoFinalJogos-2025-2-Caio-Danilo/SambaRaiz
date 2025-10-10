#include "stage.h"
#include <stdio.h>
#include <string.h>

// Converte um char ('z', 'x', 'c') para o SDL_Keycode correspondente
static SDL_Keycode charParaTecla(char c) {
    switch (c) {
        case 'z':{
            return SDLK_z; break;
        }
        case 'x': {
            return SDLK_x; break;
        }
        case 'c': {
            return SDLK_c; break;
        } 
    }
    return SDLK_UNKNOWN;
}

Fase* Fase_CarregarDeArquivo(SDL_Renderer* renderer, const char* caminhoDoArquivo) {
    FILE* file = fopen(caminhoDoArquivo, "r");
    if (!file) {
        printf("Erro: Nao foi possivel abrir o arquivo da fase: %s\n", caminhoDoArquivo);
        return NULL;
    }

    Fase* fase = (Fase*) calloc(1, sizeof(Fase));
    if (!fase) {
        fclose(file);
        return NULL;
    }

    char linha[256];
    char chave[64];
    char valor[192];

    // Parte 1: Ler o Cabeçalho (Metadados)
    while (fgets(linha, sizeof(linha), file)) {
        if (strcmp(linha, "---\n") == 0) {
            break; // Fim do cabeçalho
        }
        if (sscanf(linha, "%63[^:]:%191s", chave, valor) == 2) { // Regex para : : Leia até 63 caracteres que não sejam um dois-pontos e guarde em chave.  Espere encontrar um dois-pontos literal e descarte-o. Leia o resto da string (até 191 caracteres) e guarde em valor 
            if (strcmp(chave, "MUSICA") == 0) {
                fase->musica = Mix_LoadMUS(valor);
            } else if (strcmp(chave, "BACKGROUND") == 0) {
                fase->background = IMG_LoadTexture(renderer, valor);
            } else if (strcmp(chave, "RHYTHMTRACK") == 0) {
                fase->rhythmTrack = IMG_LoadTexture(renderer, valor);
            }
        }
    }

    if (!fase->musica || !fase->background || !fase->rhythmTrack) {
        printf("Erro ao carregar recursos da fase a partir do arquivo: %s\n", Mix_GetError());
        Fase_Liberar(fase);
        fclose(file);
        return NULL;
    }

    // Parte 2: Ler o Beatmap 
    int i = 0;
    char tipo, tecla;
    Uint32 tempo, duracao;
    while (i < MAX_NOTAS_POR_FASE && fgets(linha, sizeof(linha), file)) {
        if (linha[0] == '#' || linha[0] == '\n') continue; // Ignora comentários e linhas vazias

        // Tenta ler o formato de nota longa primeiro
        if (sscanf(linha, "%c,%c,%u,%u", &tipo, &tecla, &tempo, &duracao) == 4) {
            if (tipo == 'l') {
                fase->beatmap[i++] = Note_CreateLong(charParaTecla(tecla), tempo, duracao);
            }
        } 
        // Se não, tenta ler o formato de nota normal
        else if (sscanf(linha, "%c,%c,%u", &tipo, &tecla, &tempo) == 3) {
            if (tipo == 'n') {
                fase->beatmap[i++] = Note_Create(charParaTecla(tecla), tempo);
            }
        }
    }

    fase->totalNotas = i;
    fase->proximaNotaIndex = 0;
    fclose(file);
    printf("Fase '%s' carregada com %d notas.\n", caminhoDoArquivo, fase->totalNotas);
    return fase;
}

void Fase_Liberar(Fase* fase) {
    if (fase) {
        if (fase->background) SDL_DestroyTexture(fase->background);
        if (fase->rhythmTrack) SDL_DestroyTexture(fase->rhythmTrack);
        if (fase->musica) Mix_FreeMusic(fase->musica);
        free(fase);
    }
}