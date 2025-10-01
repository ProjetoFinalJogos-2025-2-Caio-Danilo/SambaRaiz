# Compilador e flags
CC = gcc
CFLAGS = -Wall -IC:/msys64/mingw64/include
LDFLAGS = -LC:/msys64/mingw64/lib
LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_gfx -lSDL2_image -lSDL2_mixer -lSDL2_ttf

# Fontes e objetos
SRC = src/game.c src/main.c src/note.c src/stage.c src/auxFuncs/auxWaitEvent.c src/Songs/meuLugarFase.c
OBJ = $(SRC:.c=.o)

# Executável
BIN = output/main.exe

# Regra padrão
all: $(BIN)

# Como criar o executável
$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LIBS)

# Como criar os objetos
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Limpeza
clean:
	rm -f src/*.o src/auxFuncs/*.o src/Songs/*.o output/main.exe
