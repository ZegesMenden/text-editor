#ifndef WINDOWMAN_H
#define WINDOWMAN_H

#include <ncurses.h>
#include "textMan.h"
#include "textErr.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {

    size_t win_width;
    size_t win_height;

    size_t cursor_x;
    size_t cursor_y;

} windowman_t;

textErr windowman_init(windowman_t** inst);
textErr windowman_render(windowman_t* ctx, filebuf* fbuf);
textErr windowman_destroy(windowman_t** inst);

#endif /* WINDOWMAN_H */
