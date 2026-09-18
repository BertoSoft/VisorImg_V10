
#ifndef IMAGEN_H


//1.- Extructuras

typedef struct {
    unsigned char   *pixels;
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




#endif