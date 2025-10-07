# Nome do Compilador
CC = gcc

# Pacotes SDL2 a serem encontrados pelo pkg-config
SDL_PACKAGES = sdl2 SDL2_gfx SDL2_image SDL2_mixer SDL2_ttf

# Flags de Compilação (-I) e Linker (-L, -l) obtidas automaticamente
# -Wall habilita todos os avisos importantes
CFLAGS = -Wall $(shell pkg-config --cflags $(SDL_PACKAGES))
LDFLAGS = $(shell pkg-config --libs $(SDL_PACKAGES)) -lSDL2main

# Diretório onde o executável final será colocado
TARGET_DIR = output

# Lista de todos os arquivos fonte .c
SRC = src/game.c \
      src/main.c \
      src/note.c \
      src/stage.c \
      src/auxFuncs/auxWaitEvent.c \
      src/Songs/meuLugarFase.c

OBJ = $(SRC:.c=.o)

# Padrão para sistemas baseados em Unix (Linux, macOS)
EXECUTABLE = $(TARGET_DIR)/main
RM = rm -f

# Detecta se o sistema é Windows e sobrepõe as variáveis
ifeq ($(OS),Windows_NT)
    EXECUTABLE = $(TARGET_DIR)/main.exe
endif


# --- Regras do Make ---

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) | $(TARGET_DIR)
	@echo "Ligando os objetos para criar o executável..."
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo "Executável '$(EXECUTABLE)' criado com sucesso!"

%.o: %.c
	@echo "Compilando $<..."
	$(CC) -c $< -o $@ $(CFLAGS)

$(TARGET_DIR):
	@echo "Criando diretório de saída: $(TARGET_DIR)"
	mkdir -p $(TARGET_DIR)

clean:
	@echo "Limpando arquivos compilados..."
	$(RM) $(OBJ)
	$(RM) $(EXECUTABLE)
	@echo "Limpeza concluída."

.PHONY: all clean