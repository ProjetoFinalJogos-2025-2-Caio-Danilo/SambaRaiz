#ifndef DEFS_H
#define DEFS_H

// --- Configurações da Janela ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// --- Pista de Ritmo ---
#define RHYTHM_TRACK_WIDTH 1280 + 50 // Largura da sua imagem de pista
#define RHYTHM_TRACK_HEIGHT 150 // Altura da sua imagem de pista
#define RHYTHM_TRACK_POS_X (0) // Quase na direita
#define RHYTHM_TRACK_POS_Y (SCREEN_HEIGHT - RHYTHM_TRACK_HEIGHT - 50) // Perto do fundo


// --- Lógica do Jogo ---
#define NOTE_START_X (RHYTHM_TRACK_POS_X + RHYTHM_TRACK_WIDTH + 50.0f) 
#define NOTE_Y (RHYTHM_TRACK_POS_Y + (RHYTHM_TRACK_HEIGHT / 2) - (NOTE_HEIGHT / 2)) // Centro vertical da pista
#define NOTE_WIDTH 45
#define NOTE_HEIGHT 45
#define NOTE_SPEED 450.0f // Pixels por segundo

// Posição dos checkers (alvos) - AGORA DENTRO DA PISTA
#define CHECKER_Z_X (RHYTHM_TRACK_POS_X + 50) // Mais à esquerda da pista
#define CHECKER_X_X (CHECKER_Z_X + 70) // Distância entre os checkers
#define CHECKER_C_X (CHECKER_X_X + 70)
#define CHECKER_Y NOTE_Y // A mesma altura vertical das notas

// Tamanho e posição para o contorno do checker (maior que o checker base)
#define CHECKER_CONTOUR_WIDTH   72 // Um pouco maior, por exemplo, 8px de margem total (4px de cada lado)
#define CHECKER_CONTOUR_HEIGHT  72
#define CHECKER_CONTOUR_OFFSET_X ((CHECKER_CONTOUR_WIDTH - NOTE_WIDTH) / 2) // Calcular o offset para centralizar
#define CHECKER_CONTOUR_OFFSET_Y ((CHECKER_CONTOUR_HEIGHT - NOTE_HEIGHT) / 2)

// Altura para o texto das teclas sob os checkers
#define KEY_LABEL_Y (RHYTHM_TRACK_POS_Y + RHYTHM_TRACK_HEIGHT + 10) // Abaixo da pista

// Margem de acerto (em pixels)
#define HIT_WINDOW 30

#endif // DEFS_H