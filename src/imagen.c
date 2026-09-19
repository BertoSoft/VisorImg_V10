
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

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

XImage *initXImage(Display *display,ImagenBuffer *imagen){
    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    // Reservamos la memoria del buffer X11
    char *buffer_x11 = (char *)malloc(imagen->ancho * imagen->alto * 4);
    if(!buffer_x11){
        printf("Memoria insuficiente...");
        XCloseDisplay(display);
        return NULL;
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
    return ximage;
}

ERROR_IMG showXImage(Display *display, XImage *ximage){
    if(!display || !ximage){
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    int     scr     = DefaultScreen(display);
    Window  root    = DefaultRootWindow(display);

    Window ventana = XCreateSimpleWindow(
        display, 
        root, 
        100, 100,            // Posición inicial en la pantalla (x, y)
        ximage->width,       // Ancho de la ventana igual al de la imagen
        ximage->height,      // Alto de la ventana igual al de la imagen
        1,                   // Ancho del borde
        BlackPixel(display, scr), // Color del borde
        WhitePixel(display, scr)  // Color de fondo
    );
    // Titulo ventana
    XStoreName(display, ventana, NOMBRE_APP);

    // 3. Seleccionar los eventos que queremos escuchar
    // ExposeMask: Nos avisa cuándo debemos redibujar la ventana (ej. al abrirse o maximizarse)
    // KeyPressMask: Captura pulsaciones de teclado para poder cerrar la ventana
    XSelectInput(display, ventana, ExposureMask | KeyPressMask);

    // 4. Crear el Contexto Gráfico (GC) necesario para dibujar
    GC gc = XCreateGC(display, ventana, 0, NULL);

    // 5. Hacer visible la ventana en la pantalla
    XMapWindow(display, ventana);

    // Bucle de eventos
    XEvent event;
    int ejecutar = 1;
    while (ejecutar){
        XNextEvent(display, &event);

        switch (event.type)
        {
        case Expose:
            XPutImage(
                display,
                ventana,
                gc,
                ximage,
                0,              // X de imagen origen
                0,              // Y de imagen origen
                0,              // X de imagen destino
                0,              // Y de imagen destino
                ximage->width,
                ximage->height
            );
            XFlush(display);
            break;

        case KeyPress:
            KeySym keySim = XLookupKeysym(&event.xkey, 0);
            if(keySim == XK_Escape){
                ejecutar = 0;
            }
            break;
        }
    }
    
    // 7. Limpieza local de recursos de la ventana antes de salir
    XFreeGC(display, gc);
    XDestroyWindow(display, ventana);
    return IMG_OK;
}

