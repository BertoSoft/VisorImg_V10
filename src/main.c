
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "imagen.h"

int main(){
    char            ruta[1024] = {0};
    ERROR_IMG       error = ERROR_FILE_NOT_FOUND;
    ImagenBuffer    *imagen = (ImagenBuffer *)malloc(sizeof(ImagenBuffer));

    //Inicializamos imagen, con pixels a nulo
    if(!imagen){
        printf("Memoria insuficiente...");
        return -1;
    }
    imagen->pixels = NULL;

    // Pedimos la ruta de la imagen
    if((error = getRutaImagen(ruta, 1024)) != IMG_OK){
        printf("Error: %s\n", getError(error));
        return -1;
    }

    // Obtenemos el tipo de imagen y la pasamos a memoria
    switch (getTipoImagen(ruta))
    {
    case IMG_BMP:
        if(bmpToImagenBuffer(ruta, imagen) != IMG_OK){
            printf("No se puede leer el archivo...");
            return -1;
        } 
        break;
    case IMG_PNG:
        /* code */
        break;
    case IMG_JPG:
        /* code */
        break;
    case IMG_JPEG:
        /* code */
        break;
    case IMG_ERROR:
        printf("No se puede abrir el fichero ...");
        return -1;
    default:
        printf("Formato de imagen no compatible\n");
        return -1;
    }

    // Si estamos aqui es que tenemo la imagen en imagen->pixels en formato RGB













    // 3. MOMENTO DE LIBERAR (Justo antes de salir del programa)
    if (imagen != NULL) {
        // Primero liberamos el array interno de píxeles
        if (imagen->pixels != NULL) {
            free(imagen->pixels);
        }
        // Después liberamos la estructura contenedora
        free(imagen);
    }

    return 0;
}