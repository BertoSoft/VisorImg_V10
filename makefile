# Variables de configuración
CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -Iinclude

# Nombre de tu ejecutable final
TARGET = visorImg

# Archivos fuente apuntando a la carpeta src/
SRCS = src/main.c \
       src/tinyfiledialogs.c \
       src/imagen.c

# Regla por defecto (compila todo)
all: $(TARGET)

# Cómo construir el binario final
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -lX11

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET)