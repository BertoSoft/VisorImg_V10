
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

    // Si estamos aqui es que tenemo la imagen en imagen->pixels en formato RGBA
    Display *display = XOpenDisplay(NULL);
    if(!display){
        printf("Error al conectar con el servidor X11..");
        return -1;
    }
    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    // Reservamos la memoria del buffer X11
    char *buffer_x11 = (char *)malloc(imagen->ancho * imagen->alto * 4);
    if(!buffer_x11){
        printf("Memoria insuficiente...");
        XCloseDisplay(display);
        return -1;
    }

    XImage *ximage = XCreateImage(
        display,
        visual,
        depth,
        ZPixmap,
        0,
        buffer_x11,
        imagen->ancho,
        imagen->alto,
        32,
        0
    );

    if(!ximage){
        printf("Error al crear XImage");
        free(buffer_x11);
        XCloseDisplay(display);
        return -1;
    }

    // Ponemos imagen en ximagen
    if(imagenBufferToXImage(imagen, ximage) != IMG_OK){
        printf("Error al convertir imagen -> ximagen...");
        free(buffer_x11);
        XCloseDisplay(display);
        return -1;
    }
    



    printf(" XImage creada con exito");








     // ==========================================
    // 3. MOMENTO DE LIBERAR (Al final del programa)
    // ==========================================
    if (ximage != NULL) {
        // ¡Ojo! XDestroyImage libera la estructura Y TAMBIÉN hace free() de 'x11_buffer' 
        // de forma automática porque se lo asociamos en XCreateImage.
        XDestroyImage(ximage); 
    }

    if (display != NULL) {
        XCloseDisplay(display);
    }

    if (imagen != NULL) {
        if (imagen->pixels != NULL) {
            free(imagen->pixels);
        }
        free(imagen);
    }
    return 0;
}