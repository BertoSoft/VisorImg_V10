
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "imagen.h"
#include "tinyfiledialogs.h"

const char *getError(ERROR_IMG error){
    switch (error)
    {
    case IMG_OK:
        return "Todo correcto...";
    case ERROR_FILE_NOT_FOUND:
        return "No existe el fichero...";
    case ERROR_FORMATO_NO_RECONOCIDO:
        return "No se reconoce el formato...";
    case ERROR_MEMORIA_INSUFICIENTE:
        return "Memoria insuficiente...";
    default:
        return "Error desconocido...";
    }
}

ERROR_IMG getRutaImagen(char *ruta, size_t tamano){
    const char *filtros[]={"*.bmp", "*.png", "*.jpg", "*.jpeg"};

    const char *fileDir = tinyfd_openFileDialog(
        "Selecciona una Imagen",     // Título de la ventana emergente
        "./",                       // Ruta inicial por defecto (vacío = carpeta actual)
        4,                        // Cantidad de extensiones en el filtro (png, jpg, jpeg, bmp)
        filtros,                  // El array con los filtros permitidos
        "Formatos de Imagen (*.bmp. *.png, *.jpg, *.jpeg)",     // Texto descriptivo del selector
        0                         // 0 = Selección de un único archivo (1 sería múltiple)
    );

    // Si se pulsa cancelar y no hay fileDir
    if(fileDir == NULL){
        return ERROR_FILE_NOT_FOUND;
    }

    strncpy(ruta, fileDir, tamano -1);
    ruta[tamano -1] = '\0';

    return IMG_OK;
}

TipoImagen getTipoImagen(const char *ruta){
    unsigned char   bytes[4];

    FILE *file = fopen(ruta, "rb");
    if(!file){
        return IMG_ERROR;
    }

    // Leemos 4 bytes del archivo
    if((fread(bytes, sizeof(unsigned char), 4, file)) != 4){
        fclose(file);
        return IMG_ERROR;
    }
    fclose(file);

    // retornamos el tipo de archivo
    if(bytes[0] == 0x42 && bytes[1] == 0x4D){
        return IMG_BMP;
    }
    if(bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47){
        return IMG_PNG;
    }
    if(bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF){
        return IMG_JPEG;
    }

    return IMG_DESCONOCIDA;
}

ERROR_IMG bmpToImagenBuffer(const char *ruta, ImagenBuffer *imagen){
    FileHeader      fileHeader;
    InfoHeader      infoHeader;

    FILE *file = fopen(ruta, "rb");

    // leemos la cabeceras
    if(fread(&fileHeader, sizeof(FileHeader), 1, file) != 1 ||
        fread(&infoHeader, sizeof(InfoHeader), 1, file) != 1){
            fclose(file);
            return ERROR_FORMATO_NO_RECONOCIDO;
    }

    // Obtenemos la profundidad
    int bpp = infoHeader.bitCount;
    if((bpp != 24 && bpp != 32) || infoHeader.compression != 0){
        fclose(file);
        return ERROR_FORMATO_NO_RECONOCIDO;
    }

    // Asignamos propiedades a imagen
    imagen->ancho = infoHeader.width;
    imagen->alto = abs(infoHeader.height);
    imagen->channels = (bpp == 32) ? 4 : 3;

    // Reservamos la memoria para pixels
    imagen->pixels = (PixelRGBA *)malloc(imagen->ancho * imagen->alto * sizeof(PixelRGBA));
    if(!imagen->pixels){
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    // posicionamos el puntero
    fseek(file, fileHeader.offset, SEEK_SET);

    //Obtenemos el relleno
    int bytes_por_pixel = bpp / 8;
    int relleno = (4 - (imagen->ancho * bytes_por_pixel) % 4) % 4;

    // 1.- Recorremos las filas
    for(int i=0; i<imagen->alto; i++){
        int fila_destino = (infoHeader.height > 0) ? (imagen->alto -1 -i) : i;

        //2.- Recorremos las columnas
        for(int j=0; j<imagen->ancho; j++){

            // 3.- Leemos un pixel
            unsigned char pixel[4];
            fread(pixel, sizeof(unsigned char), bytes_por_pixel, file);

            // 4.- Obtenemos el indice de destino en el imagen->pixels
            int indice_destino = (fila_destino * imagen->ancho) + j;

            // 5.- MApeamos los valores RGB
            imagen->pixels[indice_destino].rojo    = pixel[2];
            imagen->pixels[indice_destino].verde   = pixel[1];
            imagen->pixels[indice_destino].azul    = pixel[0];

            // 6.- Canal alfa
            imagen->pixels[indice_destino].alfa    = (bpp == 32) ? pixel[3] : 255;
        }

        //7.-  Ahora el relleno de cada fila
        if (relleno > 0) {
            fseek(file, relleno, SEEK_CUR);
        }
    }

    fclose(file);
    return IMG_OK;
}

ERROR_IMG imagenBufferToXImage(ImagenBuffer *imagen, XImage *ximage){
    if(!imagen || !imagen->pixels || !ximage || !ximage->data){
        return ERROR_FILE_NOT_FOUND;
    }
    if(imagen->ancho != ximage->width || imagen->alto != ximage->height){
        return ERROR_FORMATO_NO_RECONOCIDO;
    }

    // Asignamos a x11_buffer la direccion de memoria de ximage->data
    unsigned char *x11_buffer = (unsigned char *)ximage->data;

    int bytes_por_pixel = ximage->bits_per_pixel / 8;
    
    //MApeamos los valores imagen->pixels RGBA a ximage->data BGRA
    for(int i=0; i<(imagen->ancho * imagen->alto); i++){
        int idx = i * bytes_por_pixel;

        x11_buffer[idx + 0] = imagen->pixels[i].azul;
        x11_buffer[idx + 1] = imagen->pixels[i].verde;
        x11_buffer[idx + 2] = imagen->pixels[i].rojo;
        
        x11_buffer[idx + 3] = imagen->pixels[i].alfa;
    }

    return IMG_OK;
}


