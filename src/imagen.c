
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

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

ERROR_IMG bmpToImagenBuffer(const char *ruta, ImagenBuffer **imagen){
    FileHeader      fileHeader;
    InfoHeader      infoHeader;

    FILE *file = fopen(ruta, "rb");

    // leemos la cabeceras
    if(fread(&fileHeader, sizeof(FileHeader), 1, file) != 1 ||
        fread(&infoHeader, sizeof(InfoHeader), 1, file) != 1){
            fclose(file);
            return ERROR_FORMATO_NO_RECONOCIDO;
    }

     //reservamos la memoria de la estructura
    *imagen = (ImagenBuffer *)malloc(sizeof(ImagenBuffer));
    if(!(*imagen)){
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    // Rellenamos valores
    (*imagen)->ancho        = infoHeader.width;
    (*imagen)->alto         = abs(infoHeader.height);
    (*imagen)->channels     = infoHeader.bitCount/8;

    // reservamos la memoria de pixels
    size_t tamano = (*imagen)->ancho * (*imagen)->alto * (*imagen)->channels;
    (*imagen)->pixels = (unsigned char *)malloc(tamano);
    if(!(*imagen)->pixels){
        // CORRECCIÓN: Liberamos la estructura previa para evitar fugas de memoria (Memory Leak)
        free(*imagen);
        *imagen = NULL;
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

     // CORRECCIÓN: Calcular el padding por fila en el archivo BMP
    int relleno = (4 - ((*imagen)->ancho * (*imagen)->channels) % 4) % 4;

    // situamos el punteero
    fseek(file, fileHeader.offset, SEEK_SET);

    // copiamos los datos
    //recorremos las filas de la imagen
    for(int i=0;i < (*imagen)->alto; i++){
        // calculamos fila destino en imagen->pixels
        int fila_destino = (infoHeader.height > 0) ? ((*imagen)->alto -1 -i) : i;

        // Puntero a fila destino
        unsigned char   *ptr_fila_destino   = &((*imagen)->pixels[fila_destino * (*imagen)->ancho * (*imagen)->channels]);

        // leemos toda la fila
        fread(ptr_fila_destino, sizeof(unsigned char), (*imagen)->ancho * (*imagen)->channels, file);

        // corregimos los colores de 3 en 3 o de 4 en 4 para pasar de BGR -> RGB
        for(int j=0; j<((*imagen)->ancho * (*imagen)->channels); j+= (*imagen)->channels){
            unsigned char temporal = ptr_fila_destino[j];  // azul-> temporal
            ptr_fila_destino[j] = ptr_fila_destino[j+2];    // Rojo pasa al sitio del azul
            ptr_fila_destino[j+2] = temporal;               // El azul pasa al ultimo puesto
            // si existe cuarto canal se queda como esta
        }
        // CORRECCIÓN: Saltar los bytes de padding al terminar de leer la fila en el archivo
        fseek(file, relleno, SEEK_CUR);
    }

    fclose(file);
    return IMG_OK;
}


