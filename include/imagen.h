
#ifndef IMAGEN_H
#define IMAGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


//1.- Estructuras

typedef struct{
    unsigned char rojo;
    unsigned char verde;
    unsigned char azul;
    unsigned char alfa;
}PixelRGBA;

typedef struct {
    PixelRGBA       *pixels;
    int             ancho;
    int             alto;
    int             channels;
}ImagenBuffer;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;          // 2 bytes - Firma del archivo ('BM')
    unsigned int   size;          // 4 bytes - Tamaño total del archivo
    unsigned short reserved1;     // 2 bytes - Reservado
    unsigned short reserved2;     // 2 bytes - Reservado
    unsigned int   offset;        // 4 bytes - Dónde empiezan los píxeles
}FileHeader;

typedef struct {
    unsigned int   size;          // 4 bytes - Tamaño de esta cabecera
    int            width;         // 4 bytes - Ancho (con signo)
    int            height;        // 4 bytes - Alto (con signo)
    unsigned short planes;        // 2 bytes - Planos de color
    unsigned short bitCount;      // 2 bytes - Bits por píxel (ej. 24)
    unsigned int   compression;   // 4 bytes - Compresión
    unsigned int   sizeImage;     // 4 bytes - Tamaño de los datos de imagen
    int            xPixelsPerMeter;// 4 bytes - Resolución X
    int            yPixelsPerMeter;// 4 bytes - Resolución Y
    unsigned int   colorsUsed;    // 4 bytes - Colores usados
    unsigned int   colorsImportant;// 4 bytes - Colores importantes
}InfoHeader;
#pragma pack(pop)

// 2.- ENUMS
typedef enum{
    IMG_OK = 0,
    ERROR_FILE_NOT_FOUND,
    ERROR_FORMATO_NO_RECONOCIDO,
    ERROR_MEMORIA_INSUFICIENTE
}ERROR_IMG;

typedef enum{
    IMG_BMP = 0,
    IMG_PNG,
    IMG_JPG,
    IMG_JPEG,
    IMG_ERROR,
    IMG_DESCONOCIDA
}TipoImagen;

// 3.- Funciones

ERROR_IMG   getRutaImagen(char *ruta, size_t tamano);
const char *getError(ERROR_IMG error);
TipoImagen  getTipoImagen(const char *ruta);
ERROR_IMG   bmpToImagenBuffer(const char *ruta, ImagenBuffer *imagen);
ERROR_IMG   imagenBufferToXImage(ImagenBuffer *imagen, XImage *ximage);


#endif