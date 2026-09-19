
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
    switch (getTipoImagen(ruta)){
        case IMG_BMP:
            if(bmpToImagenBuffer(ruta, imagen) != IMG_OK){
                printf("No se puede leer el archivo...");
                return -1;
            } 
            break;
        case IMG_PNG:
            if(pngToImagenBuffer(ruta, imagen) != IMG_OK){
                printf("No se puede leer el archivo...");
                return -1;
            } 
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

    // Obtenemos el display
    Display *display = XOpenDisplay(NULL);
    if(!display){
        printf("Error al conectar con el servidor X11..");
        return -1;
    }
   
    // Creamos la ximage
    XImage *ximage = initXImage(display, imagen);
    if(!ximage){
        printf("Error al crear XImage\n");
        XCloseDisplay(display);
        if(imagen->pixels) free(imagen->pixels);
        free(imagen);
        return -1;
    }

    // Ponemos imagen en ximagen
    if(imagenBufferToXImage(imagen, ximage) != IMG_OK){
        printf("Error al convertir imagen -> ximagen...\n");
        XDestroyImage(ximage);
        XCloseDisplay(display);
        if(imagen->pixels) free(imagen->pixels);
        free(imagen);
        return -1;
    }

    //enseñamos la imagen
    if(showXImage(display, ximage) != IMG_OK){
        printf("Error al abrir XWindow...\n");
        XDestroyImage(ximage);
        XCloseDisplay(display);
        if(imagen->pixels) free(imagen->pixels);
        free(imagen);
    }



    printf(" XImage creada con exito");








     // ==========================================
    // 3. MOMENTO DE LIBERAR (Al final del programa)
    // ==========================================
    if (ximage != NULL) {
        XDestroyImage(ximage); // Libera la estructura y 'buffer_x11'
    }

    // Cerramos el display al final, después de destruir la XImage
    XCloseDisplay(display);

    if (imagen != NULL) {
        if (imagen->pixels != NULL) {
            free(imagen->pixels);
        }
        free(imagen);
    }
    return 0;
}

