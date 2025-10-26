#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>

#include "textErr.h"
#include "textMan.h"
#include "windowMan.h"

#define REQUIRED_ARGS \
    REQUIRED_STRING_ARG(input_file, "input", "Input file path") \

#include "easyargs.h"

int main(int argc, char** argv) {

    args_t args = make_default_args();

    if (!parse_args(argc, argv, &args)) {
        return 1;
    }

    FILE* fptr = fopen(args.input_file, "rb");
    if ( fptr == NULL ) {
        printf("Failed to open <%s>\n", args.input_file);
        return 1;
    }

    fseek(fptr, 0, SEEK_END);
    long file_size = ftell(fptr);
    rewind(fptr);

    char* strbuf = (char*)calloc(sizeof(char), file_size);
    if ( strbuf == NULL ) { return 1; }

    size_t bytes_read = fread(strbuf, 1, file_size, fptr);

    fclose(fptr);

    filebuf* file_ctx = NULL;
    textErr ret = filebuf_init(&file_ctx, 8);
    if ( ret != ERR_NONE ) { 
        printf("Failed to initialize filebuf, reason: %s\n", textErr_tostr(ret));
        return 1;
    }

    ret = filebuf_load(&file_ctx, strbuf, args.input_file, 0);
    if ( ret != ERR_NONE ) { 
        printf("Failed to load data to filebuf, reason: %s\n", textErr_tostr(ret));
        return 1;
    }

    windowman_t* window_ctx;

    ret = windowman_init(&window_ctx);
    if ( ret != ERR_NONE ) {
        printf("Failed to initialize window manager, reason: %s\n", textErr_tostr(ret));
        windowman_destroy(&window_ctx);
        return 1;
    }

    while (true) {

        windowman_render(window_ctx, file_ctx);

    }

    ret = windowman_destroy(&window_ctx);
    if ( ret != ERR_NONE ) {
        printf("Failed to close window manager, reason: %s\n", textErr_tostr(ret));
        return 1;
    }

    return 0;

}