#ifndef DEFS_H
#define DEFS_H

// --- Configurações da Janela ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// --- Lógica do Jogo ---
#define NOTE_START_X (SCREEN_WIDTH + 50.0f) // Onde as notas aparecem
#define NOTE_Y (SCREEN_HEIGHT / 1.25)          // A linha única onde as notas se movem
#define NOTE_WIDTH 50
#define NOTE_HEIGHT 50
#define NOTE_SPEED 450.0f // Pixels por segundo

// Posição dos checkers (alvos)
#define CHECKER_Z_X 100
#define CHECKER_X_X 170
#define CHECKER_C_X 240
#define CHECKER_Y NOTE_Y

// Margem de acerto (em pixels)
#define HIT_WINDOW 30

#endif // DEFS_H