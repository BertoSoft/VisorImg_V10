
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>

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
    int ancho_ventana = ximage->width;
    int alto_ventana = ximage->height;

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
    XSelectInput(display, ventana, ExposureMask | KeyPressMask | StructureNotifyMask);

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
            XClearWindow(display, ventana);
            int dest_x = (ancho_ventana - ximage->width) / 2;
            int dest_y = (alto_ventana -ximage->height) / 2;
            if(dest_x < 0) dest_x = 0;
            if(dest_y < 0) dest_y = 0;

            XPutImage(
                display,
                ventana,
                gc,
                ximage,
                0,                  // X de imagen origen
                0,                  // Y de imagen origen
                dest_x,             // X de imagen destino
                dest_y,             // Y de imagen destino
                ximage->width,
                ximage->height
            );
            XFlush(display);
            break;

        case ConfigureNotify:
            ancho_ventana = event.xconfigure.width;
            alto_ventana = event.xconfigure.height;
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

ERROR_IMG pngToImagenBuffer(const char *ruta, ImagenBuffer *imagen){
    
    // Comprobacion inicial
    if(!ruta || !imagen){
        return ERROR_FILE_NOT_FOUND;
    }

    // Abrimos el archivo fisico
    FILE *file = fopen(ruta, "rb");
    if(!file){
        return ERROR_FILE_NOT_FOUND;
    }

    //Estructura principal del lectura png
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr){
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr); 
    if(!info_ptr){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(file);
    }

     // 4. Configurar el punto de restauración de errores
    if (setjmp(png_jmpbuf(png_ptr))) {
        // Si libpng falla en cualquier punto posterior, el flujo saltará mágicamente aquí.
        // Limpiamos todo lo creado hasta el momento y salimos con error.
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
        return ERROR_FORMATO_NO_RECONOCIDO;
    }

    // vinculamos el archivo fisico con el manejador de libpng
    png_init_io(png_ptr, file);

    // leemos los metadatos
    png_read_info(png_ptr, info_ptr);

    //Extraemos las propiedades de la imagen
    png_uint_32 ancho_png, alto_png;
    int bit_depth, tipo_color;
    png_get_IHDR(png_ptr, info_ptr, &ancho_png, &alto_png, &bit_depth, &tipo_color, NULL, NULL, NULL);
    imagen->ancho = (int)ancho_png;
    imagen->alto = (int)alto_png;

    // Si la imagen está indexada (usa paleta), la expande a RGB directo
    if(tipo_color == PNG_COLOR_TYPE_PALETTE){
        png_set_palette_to_rgb(png_ptr);
    }

    // Si es escala de grises con menos de 8 bits, expande a 8 bits
    if (tipo_color == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    // Si tiene transparencia mediante un chunk tRNS, conviértela en un canal Alfa real
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }

    // Si cada canal tiene 16 bits (HDR), los reduce a 8 bits que es lo que maneja tu estructura
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }

    // Si es escala de grises pura, la pasa a RGB (R=G=B)
    if (tipo_color == PNG_COLOR_TYPE_GRAY || tipo_color == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }

    // Forzamos a que si falta el canal Alfa, se añada un relleno opaco (255) de forma automática
    if (!(tipo_color & PNG_COLOR_MASK_ALPHA)) {
        png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    // Aplicar los cambios en las estructuras de actualización de libpng
    png_read_update_info(png_ptr, info_ptr);
    
    // Ahora garantizamos que siempre tendrá 4 canales (RGBA)
    imagen->channels = 4; 

    // REservamos memoria para los pixeles
    imagen->pixels = (PixelRGBA *)malloc(imagen->ancho * imagen->alto * sizeof(PixelRGBA));
    if(!imagen->pixels){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    // Creamos un puntero a filas exigido por png
    png_bytepp fila_ptr = (png_bytepp)malloc(sizeof(png_bytep) * imagen->alto);
    if(!fila_ptr){
         free(imagen->pixels);
        imagen->pixels = NULL;
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    // Hacemos que cada puntero de fila apunte a una fila de imagen
    for(int i=0; i<imagen->alto; i++){
        fila_ptr[i] = (png_bytep)&imagen->pixels[i * imagen->ancho];
    }

    // Leemos toda la imagen de golpe
    png_read_image(png_ptr, fila_ptr);

    //Terminamos de leer
    png_read_end(png_ptr, NULL);

    // Liberaos memoria y retornamos
    free(fila_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(file);

    return IMG_OK;
}

ERROR_IMG jpgToImagenBuffer(const char *ruta, ImagenBuffer *imagen){
    if(!ruta || !imagen){
        return ERROR_FILE_NOT_FOUND;
    }

    // abrimos el archivo
    FILE *file = fopen(ruta, "rb");
    if(!file){
        return ERROR_FILE_NOT_FOUND;
    }

    // Declaramos las estructuras
    struct jpeg_decompress_struct compress_info;
    struct mi_error_mgr jerror;

    // configurar el manejador de errrores personalizado
    compress_info.err = jpeg_std_error(&jerror.pub);
    jerror.pub.error_exit = mi_error_exit;
    if (setjmp(jerror.setjmp_buffer)) {
        jpeg_destroy_decompress(&compress_info);
        fclose(file);
        if (imagen->pixels) {
            free(imagen->pixels);
            imagen->pixels = NULL;
        }
        return ERROR_FORMATO_NO_RECONOCIDO;
    }

// Inicializar descompresión
    jpeg_create_decompress(&compress_info);
    jpeg_stdio_src(&compress_info, file);
    jpeg_read_header(&compress_info, TRUE);

    // Forzar a libjpeg a que siempre nos devuelva RGB de 3 canales (24 bits)
    compress_info.out_color_space = JCS_RGB;
    jpeg_start_decompress(&compress_info);

    // Asignar propiedades a tu estructura unificada
    imagen->ancho = compress_info.output_width;
    imagen->alto = compress_info.output_height;
    imagen->channels = 4; // Tu buffer final trabaja siempre con 4 canales (RGBA)

    // Reservar memoria para los píxeles
    imagen->pixels = (PixelRGBA *)malloc(imagen->ancho * imagen->alto * sizeof(PixelRGBA));
    if (!imagen->pixels) {
        jpeg_destroy_decompress(&compress_info);
        fclose(file);
        return ERROR_MEMORIA_INSUFICIENTE;
    }

    // Reservar un buffer temporal para almacenar una sola fila leída (3 bytes por píxel: RGB)
    int row_stride = compress_info.output_width * compress_info.output_components;
    JSAMPARRAY buffer = (*compress_info.mem->alloc_sarray)((j_common_ptr) &compress_info, JPOOL_IMAGE, row_stride, 1);

    // Leer la imagen fila por fila de arriba a abajo
    while (compress_info.output_scanline < compress_info.output_height) {
        int fila_actual = compress_info.output_scanline;
        jpeg_read_scanlines(&compress_info, buffer, 1);

        // Mapear los datos del buffer temporal RGB al buffer final de tu estructura PixelRGBA (con Alfa opaco)
        for (int x = 0; x < imagen->ancho; x++) {
            int src_idx = x * 3;
            int dest_idx = (fila_actual * imagen->ancho) + x;

            imagen->pixels[dest_idx].rojo  = buffer[0][src_idx + 0];
            imagen->pixels[dest_idx].verde = buffer[0][src_idx + 1];
            imagen->pixels[dest_idx].azul  = buffer[0][src_idx + 2];
            imagen->pixels[dest_idx].alfa  = 255; // JPEG no tiene transparencia, ponemos opaco por defecto
        }
    }

    // Finalizar descompresión y liberar recursos locales
    jpeg_finish_decompress(&compress_info);
    jpeg_destroy_decompress(&compress_info);
    fclose(file);

    return IMG_OK;
}

void mi_error_exit(j_common_ptr cinfo) {
    struct mi_error_mgr *myerr = (struct mi_error_mgr *) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}