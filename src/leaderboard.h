#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#define MAX_LEADERBOARD_ENTRIES 10
#define MAX_SONGS_IN_LEADERBOARD 50
#define SONG_NAME_MAX_LEN 64

#include <SDL2/SDL_ttf.h>

// Estrutura para um único recorde (Nome, Pontuação)
typedef struct {
    char name[4];
    int score;
} HighScore;

// Estrutura que agrupa os recordes de UMA música
typedef struct {
    char songName[SONG_NAME_MAX_LEN];
    HighScore scores[MAX_LEADERBOARD_ENTRIES];
} SongLeaderboard;

// Estrutura principal que será salva em arquivo
typedef struct {
    int songCount;
    SongLeaderboard songLeaderboards[MAX_SONGS_IN_LEADERBOARD];
} LeaderboardData;

void Leaderboard_Load(LeaderboardData* data);
void Leaderboard_Save(const LeaderboardData* data);

#endif