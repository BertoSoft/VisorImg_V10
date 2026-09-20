
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "imagen.h"

int main(){
    char            ruta[1024]  = {0};
    ERROR_IMG       error       = ERROR_FILE_NOT_FOUND;
    ImagenBuffer    *imagen     = (ImagenBuffer *)malloc(sizeof(ImagenBuffer));
    Display         *display    = NULL;
    XImage          *ximage     = NULL;

    //1.- Inicializamos imagen, con pixels a nulo
    if(!imagen){
        printf("Memoria insuficiente...");
        return -1;
    }
    imagen->pixels = NULL;

    //2.- Pedimos la ruta de la imagen
    if((error = getRutaImagen(ruta, 1024)) != IMG_OK){
        printf("Error: %s\n", getError(error));
        goto cleanup;
    }

    //3.- Obtenemos el tipo de imagen y la pasamos a memoria
    error = ERROR_FORMATO_NO_RECONOCIDO;
    switch (getTipoImagen(ruta)){
        case IMG_BMP:   error = bmpToImagenBuffer(ruta, imagen); break;
        case IMG_PNG:   error = pngToImagenBuffer(ruta, imagen); break;
        case IMG_JPG:   error = jpgToImagenBuffer(ruta, imagen); break;
        case IMG_JPEG:  error = jpgToImagenBuffer(ruta, imagen); break;
        case IMG_ERROR:
            printf("No se puede abrir el fichero ...");
            goto cleanup;
        default:
            printf("Formato de imagen no compatible\n");
            goto cleanup;
    }

    if (error != IMG_OK) {
        printf("Error al procesar los píxeles de la imagen...\n");
        goto cleanup;
    }

    // 4.- Obtenemos el display
    display = XOpenDisplay(NULL);
    if(!display){
        printf("Error al conectar con el servidor X11..");
        goto cleanup;
    }
   
    // 5.- Creamos la ximage
    ximage = initXImage(display, imagen);
    if(!ximage){
        printf("Error al crear XImage\n");
        goto cleanup;
    }

    // 6.- Ponemos imagen en ximagen
    if(imagenBufferToXImage(imagen, ximage) != IMG_OK){
        printf("Error al convertir imagen -> ximagen...\n");
        goto cleanup;
    }

    // 7.- Enseñamos la imagen
    if(showXImage(display, ximage) != IMG_OK){
        printf("Error al abrir XWindow...\n");
        goto cleanup;
    }

    // 8.- Cierre unificado de recursos (evita repetir código en cada IF)
    cleanup:
        if (ximage) XDestroyImage(ximage); 
        if (display) XCloseDisplay(display);
        if (imagen) {
            if (imagen->pixels) free(imagen->pixels);
            free(imagen);
        }


    return 0;
}

