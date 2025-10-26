
#include "windowMan.h"

textErr windowman_init(windowman_t** inst) {

    if ( inst == NULL ) { return ERR_NULL; }

    windowman_t* ctx = (windowman_t*)calloc(1, sizeof(windowman_t));
    if ( ctx == NULL ) { return ERR_MEM; }

    if ( initscr() == NULL ) {
        free(ctx);
        return ERR_MEM;
    }

    cbreak();            // disable line buffering
    noecho();            // don't echo typed characters
    keypad(stdscr, TRUE);// enable function and arrow keys
    
    // try to hide the cursor
    curs_set(0);

    if ( has_colors() ) {
        start_color();
    }

    int h, w;
    getmaxyx(stdscr, h, w);
    ctx->win_width = (size_t)w;
    ctx->win_height = (size_t)h;
    ctx->cursor_x = 0;
    ctx->cursor_y = 0;

    *inst = ctx;

    return ERR_NONE;

}

textErr windowman_render(windowman_t* ctx, filebuf* fbuf) {

    if ( ctx == NULL ) { return ERR_NULL; }

    int _h, _w;
    getmaxyx(stdscr, _h, _w);
    ctx->win_height = _h;
    ctx->win_width = _w;

    ctx->cursor_y = getcury(stdscr);
    ctx->cursor_x = getcurx(stdscr);

    // Display window size at top left
    mvprintw(0, 0, "File: %s | Size: %zu x %zu", fbuf->fname, ctx->win_width, ctx->win_height);

    // Non-blocking keyboard input
    nodelay(stdscr, TRUE);
    int ch = getch();
    const int keypress = ch == ERR ? -1 : ch;
    nodelay(stdscr, FALSE);

    // LINES / SPACERS

    // Horizontal file name line
    mvhline(1, 0, ACS_HLINE, ctx->win_width);

    // Vertical LOC line

    int maxloc = fbuf->view->headline + fbuf->viewlines;
    int digits = 0;
    while ( maxloc > 0 ) {
        maxloc /= 10;
        digits += 1;
    }

    mvvline(2, digits+1, ACS_VLINE, ctx->win_height-2);

    // Write line numbers

    // for ( int i = 2; i < ctx->win_height; i++ ) {
    //     mvprintw(i, 0, "%ld", fbuf->view->headline+i);
    // }

    // Write the text

    int lineno = 0;
    int lineposition = 0;

    linebuf* cur = fbuf->lines;
    while ( cur != NULL ) {

        mvprintw(lineposition+2, 0, "%ld", fbuf->view->headline+lineno);

        if ( strlen(cur->line) < ctx->win_width-3 ) {
            mvprintw(lineposition+2, digits+2, cur->line);
            lineno += 1;
        } else {
            
        }

        cur = cur->next;
        lineposition += 1;

    }

    refresh();

    return ERR_NONE;

}

textErr windowman_destroy(windowman_t** inst) {

    if ( inst == NULL || *inst == NULL ) { return ERR_NULL; }

    /* End ncurses mode and free context */
    endwin();

    free(*inst);
    *inst = NULL;

    return ERR_NONE;

}
