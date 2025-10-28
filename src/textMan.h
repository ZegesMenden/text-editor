#ifndef TEXTMAN_H
#define TEXTMAN_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "textErr.h"

typedef struct linebuf {

    struct linebuf* prev;
    struct linebuf* next;

    char* line;
    size_t len;

} linebuf;

// viewbuf stores some number of lines.
// the head is not guaranteed to be the root of the linked list, 
// and the tail is not guaranteed to be the end. this struct is 
// an abstraction for processing chunks of text (ie. the text in the viewport) 
typedef struct {

    struct linebuf *head;
    size_t headline;
    size_t lines;

} viewbuf;

typedef struct {

    char* prewindow;
    char* postwindow; 

    size_t prewindow_len;
    size_t postwindow_len;

    size_t prewindow_cap;
    size_t postwindow_cap;

    size_t viewlines;

    linebuf* lines;
    viewbuf* view;

    const char* fname;

} filebuf;

#define linebuf_next(lb) ((linebuf*)lb->next)
#define linebuf_prev(lb) ((linebuf*)lb->prev)

textErr linebuf_init(linebuf** inst, const char* src, size_t strsize);
textErr linebuf_parse(linebuf** inst, const char* src, size_t maxlines, size_t *charcount);

textErr viewbuf_init(viewbuf** inst, linebuf* head, size_t maxlines);

textErr filebuf_init(filebuf** inst, size_t viewlines);
textErr filebuf_load(filebuf** inst, const char* filedata, const char* fname);
textErr filebuf_resize(filebuf** inst);

textErr filebuf_scroll_down(filebuf** inst);
textErr filebuf_scroll_up(filebuf** inst);

#endif /* TEXTMAN_H */
