
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


    // Aqui metemos los datos en imagen->








    fclose(file);
    return IMG_OK;
}


