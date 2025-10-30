
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
    intrflush(stdscr, FALSE);
    
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

    // clear();
    if (fbuf->viewlines != ctx->win_height - 2) { clear(); }

    fbuf->viewlines = ctx->win_height - 2;
    textErr ret = filebuf_resize(&fbuf);
    if ( ret != ERR_NONE ) { 
        return ret;
    }

    // Display window size at top left
    mvprintw(0, 0, "File: %s | Size: %zu x %zu", fbuf->fname, ctx->win_width, ctx->win_height);

    // Non-blocking keyboard input
    // nodelay(stdscr, TRUE);
    timeout(1);
    int ch = getch();
    const int keypress = ch == ERR ? -1 : ch;
    (void)keypress; // silence unused warning for now
    // nodelay(stdscr, FALSE);

    if ( keypress != ERR ) { mvprintw(0, 32, "keypress: %03d", keypress); }
    mvprintw(0, 48, "cursor x: %ld y: %ld", ctx->cursor_x, ctx->cursor_y);

    // LINES / SPACERS

    // Horizontal file name line
    mvhline(1, 0, ACS_HLINE, ctx->win_width);

    // Vertical LOC line

    int maxloc = (int)(fbuf->view->headline + fbuf->viewlines);
    int digits = 0;
    do {
        maxloc /= 10;
        digits += 1;
    } while ( maxloc > 0 );

    mvvline(2, digits+1, ACS_VLINE, ctx->win_height-2);

    // Write text and line numbers

    int lineno = 0;
    int lineposition = 0;

    size_t* linelen_lut = (size_t*)malloc(sizeof(size_t) * fbuf->viewlines);
    linebuf** linebuf_lut = (linebuf**)malloc(sizeof(linebuf*) * fbuf->viewlines);
    if ( linebuf_lut == NULL ) { return ERR_MEM; }
    if ( linelen_lut == NULL ) { return ERR_MEM; }

    linebuf* cur = fbuf->view->head;
    while ( cur != NULL ) {
        if (lineposition >= (int)(ctx->win_height - 2)) { break; }
        if (lineno >= fbuf->viewlines) { break; }

        mvprintw(lineposition+2, 0, "%zu", fbuf->view->headline+lineno);

        // clear line
        move(lineposition+2, digits+2);
        clrtoeol();

        // compute printable length excluding trailing newline to avoid moving the cursor
        size_t plen = cur->len;
        if ( plen > 0 && cur->line[plen-1] == '\n' ) { plen -= 1; }

        // clip to available width
        size_t max_text = (ctx->win_width > (size_t)(digits+2)) ? (ctx->win_width - (size_t)(digits+2)) : 0;
        size_t to_print = (plen < max_text) ? plen : max_text;

        if (to_print > 0) {
            linebuf_lut[lineposition] = cur;
            linelen_lut[lineposition] = to_print;
            mvprintw(lineposition+2, digits+2, "%.*s", (int)to_print, cur->line);
        }

        if ( max_text < plen ) {
            lineposition += 1;
            linebuf_lut[lineposition] = cur;
            linelen_lut[lineposition] = (int)(plen-max_text);
            for ( int j = 0; j < digits; j++ ) { mvprintw(lineposition+2, j, " "); }
            mvprintw(lineposition+2, digits+2, "%.*s", (int)(plen-max_text), &cur->line[to_print]);
        }

        lineno += 1;
        cur = cur->next;
        lineposition += 1;

    }

    int char_at_cursor = mvinch(ctx->cursor_y+2, ctx->cursor_x+digits+2);    
    // Highlight character at cursor position
    attron(A_REVERSE);
    mvaddch(ctx->cursor_y + 2, ctx->cursor_x + digits + 2, char_at_cursor);
    attroff(A_REVERSE);

    if ( keypress == KEY_RIGHT ) {
        ctx->cursor_x = ctx->cursor_x + 1;
        if ( ctx->cursor_x > linelen_lut[ctx->cursor_y] ) {
            if ( ctx->cursor_y < ctx->win_height-2 ) {
                ctx->cursor_y += 1;
                ctx->cursor_x = 0;
            } else {
                ctx->cursor_x = linelen_lut[ctx->cursor_y];
            }
        }
        if ( ctx->cursor_x > ctx->win_width-5 ) { ctx->cursor_x = ctx->win_width-5; }
    } else if ( keypress == KEY_LEFT ) {
        
        if ( ctx->cursor_x == 0 && ctx->cursor_y > 0 ) {
            ctx->cursor_x = linelen_lut[ctx->cursor_y-1]-1;
            ctx->cursor_y -= 1;
        } else if ( ctx->cursor_x > 0 ) { ctx->cursor_x = ctx->cursor_x - 1; }

    } else if ( keypress == KEY_DOWN ) {
        if ( ctx->cursor_y < ctx->win_height-4 ) { ctx->cursor_y = ctx->cursor_y + 1; }
        else {
            // handle scroll down
            ret = filebuf_scroll_down(&fbuf);
            if ( ret != ERR_EOF && ret != ERR_NONE ) {
                free(linelen_lut);
                free(linebuf_lut);
                return ret;
            }
        }
        size_t max_x = linelen_lut[ctx->cursor_y];
        if (ctx->cursor_x > (int)max_x) {
            ctx->cursor_x = (int)max_x;
        }
    
    } else if ( keypress == KEY_UP ) {
        if ( ctx->cursor_y > 0 ) { ctx->cursor_y = ctx->cursor_y - 1; }
        else {
            // handle scroll down
            ret = filebuf_scroll_up(&fbuf);
            if ( ret != ERR_EOF && ret != ERR_NONE ) {
                free(linelen_lut);
                free(linebuf_lut);
                return ret;
            }
        }
        size_t max_x = linelen_lut[ctx->cursor_y];
        if (ctx->cursor_x > (int)max_x) {
            ctx->cursor_x = (int)max_x;
        }
    }

    // calculate cursor index in buffer (for line manupulation)

    size_t textposition = ctx->cursor_x;
    if ( ctx->cursor_y > 0 ) {
        for ( size_t i = ctx->cursor_y-1; i > 0; i-- ) {
            if ( linebuf_lut[i] == linebuf_lut[ctx->cursor_y] ) {
                textposition += (ctx->win_width - (size_t)(digits+2));
            } else {
                break;
            }
        }
    }

    // insert newline at character (yikes!)
    if ( keypress == 10 ) {

        mvprintw(0, 64, "enter!");

        linebuf* newline;
        ret = linebuf_init(&newline, &linebuf_lut[ctx->cursor_y]->line[textposition], linebuf_lut[ctx->cursor_y]->len-textposition);
        if ( ret != ERR_NONE ) {
            free(linebuf_lut);
            free(linelen_lut);
            return ret;
        }

        linebuf* next = linebuf_lut[ctx->cursor_y]->next;
        linebuf_lut[ctx->cursor_y]->next = newline;
        newline->prev = linebuf_lut[ctx->cursor_y];
        newline->next = next;
        next->prev = newline;

        linebuf_lut[ctx->cursor_y]->len = textposition;
        linebuf_lut[ctx->cursor_y]->line[textposition] = '\0';    
        
        fbuf->view->lines += 1;

    }

    if ( keypress == KEY_BACKSPACE || keypress == KEY_DL ) {

        // shift text
        memmove(&linebuf_lut[ctx->cursor_y]->line[textposition], &linebuf_lut[ctx->cursor_y]->line[textposition+1], linebuf_lut[ctx->cursor_y]->len-textposition);

        // insert character
        linebuf_lut[ctx->cursor_y]->len -= 1;

        // ctx->cursor_x += 1;

        if ( ctx->cursor_x == 0 && ctx->cursor_y > 0 ) {
            ctx->cursor_x = linelen_lut[ctx->cursor_y-1]-1;
            ctx->cursor_y -= 1;
        } else if ( ctx->cursor_x > 0 ) { ctx->cursor_x = ctx->cursor_x - 1; }

        if ( ctx->cursor_x > linelen_lut[ctx->cursor_y]-1 ) {
            if ( ctx->cursor_y < ctx->win_height-2 ) {
                ctx->cursor_y += 1;
                ctx->cursor_x = 0;
            } else {
                ctx->cursor_x = linelen_lut[ctx->cursor_y]-1;
            }
        }
        if ( ctx->cursor_x > ctx->win_width-5 ) { ctx->cursor_x = ctx->win_width-5; }



    }

    // Check if keypress is a typable character
    if (keypress >= 32 && keypress <= 126) {

        // reallocate memory
        if ( linebuf_lut[ctx->cursor_y]->cap < linebuf_lut[ctx->cursor_y]->len+1 ) {
            size_t newcap = linebuf_lut[ctx->cursor_y]->cap*2;
            linebuf_lut[ctx->cursor_y]->line = realloc(linebuf_lut[ctx->cursor_y]->line, newcap);
            if ( linebuf_lut[ctx->cursor_y]->line == NULL ) {
                return ERR_MEM;
            }
            linebuf_lut[ctx->cursor_y]->cap = newcap;
        }

        // shift text
        memmove(&linebuf_lut[ctx->cursor_y]->line[textposition+1], &linebuf_lut[ctx->cursor_y]->line[textposition], linebuf_lut[ctx->cursor_y]->len-textposition);

        // insert character
        linebuf_lut[ctx->cursor_y]->line[textposition] = (char)keypress;
        linebuf_lut[ctx->cursor_y]->len += 1;

        ctx->cursor_x += 1;

        if ( ctx->cursor_x >= (ctx->win_width - (size_t)(digits+2)) ) {
            if ( ctx->cursor_y < ctx->win_height-2 ) {
                ctx->cursor_y += 1;
                ctx->cursor_x = 0;
            } else {
                ctx->cursor_x = linelen_lut[ctx->cursor_y];
            }
        }

    }

    free(linelen_lut);
    free(linebuf_lut);

    ret = viewbuf_remove_empty_lines(&fbuf->view);
    if ( ret != ERR_NONE ) {
        return ret;
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
