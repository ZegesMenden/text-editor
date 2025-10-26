#include "textMan.h"

static textErr linesize(const char* textbuff, size_t* len) {

    if ( textbuff == NULL || len == NULL ) { return ERR_NULL; }

    const char* ptr = textbuff;
    
    *len = 0;
    while ( *ptr != '\n' ) {
        if (*ptr == '\0') { return ERR_EOF; }

        (*len) += 1;
        ptr++;

    }

    return ERR_NONE;

}

textErr linebuf_init(linebuf** inst, const char* src, size_t strsize) {
    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref != NULL ) { return ERR_NULL; }

    ref = (linebuf*)calloc(1, sizeof(linebuf));
    if ( ref == NULL ) { return ERR_MEM; }

    if ( src != NULL ) {
        ref->line = (char*)malloc(strsize+1);
        if ( ref->line == NULL ) { return ERR_MEM; }
        memcpy(ref->line, src, strsize);
        ref->line[strsize] = '\0';
        ref->len = strsize+1;
    }

    return ERR_NONE;

}

textErr linebuf_parse(linebuf** inst, const char* src, size_t maxlines, size_t *charcount) {
    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( src == NULL ) { return ERR_NULL; }

    size_t copyposition = 0;

    linebuf prehead = {
        .prev=NULL,
        .next=NULL,
        .line=NULL,
        .len=0
    };

    linebuf* cur = &prehead;
    linebuf* prev = NULL;

    for ( size_t line = 0; line < maxlines; line++ ) {

        size_t copysize = 0;
        size_t last_line = 0;
        textErr ret = linesize(&src[copyposition], &copysize);
        if ( ret == ERR_EOF ) { 
            printf("eof!\n");
            last_line = 1; 
        }
        else if ( ret != ERR_NONE ) { return ret; }

        copysize += 1;
        if ( cur->next == NULL ) {
            ret = linebuf_init((linebuf**)&(cur->next), &src[copyposition], copysize);
            if ( ret != ERR_NONE ) { return ret; }
        }

        copyposition += copysize;

        cur->prev = (struct linebuf*)prev;
        prev = cur;
        linebuf_next(cur)->prev = (struct linebuf*)cur;

        cur = linebuf_next(cur);

        if ( last_line ) { break; }

    }

    if ( charcount != NULL ) {
        *charcount = copyposition;
    }

    ref = (linebuf*)prehead.next;

    return ERR_NONE;

}

textErr viewbuf_init(viewbuf** inst, linebuf* head, size_t maxlines) {
    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref != NULL ) { return ERR_NULL; }

    ref = (viewbuf*)calloc(1, sizeof(viewbuf));
    if ( ref == NULL ) { return ERR_MEM; }

    if ( head != NULL ) { 

        linebuf* cur = head;
        ref->lines = 1;

        size_t linelim = maxlines == 0 ? SIZE_MAX : maxlines;

        for ( size_t line = 0; line < linelim; line++ ) {

            ref->lines += 1;
            cur = linebuf_next(cur);

            if ( cur == NULL ) { break; }

        }

    }

    return ERR_NONE;

}

textErr viewbuf_resize(viewbuf** inst, size_t newlines) {

    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }    

}

textErr filebuf_init(filebuf** inst, size_t viewlines) {
    #define ref (*inst)
    
    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref != NULL ) { return ERR_NULL; }

    ref = (filebuf*)calloc(1, sizeof(filebuf));
    if ( ref == NULL ) { return ERR_MEM; }

    textErr ret = viewbuf_init(&(ref->view), NULL, 0);
    if ( ret != ERR_NONE ) { return ret; }

    ref->viewlines = viewlines;

    return ERR_NONE;

}

textErr filebuf_load(filebuf** inst, const char* filedata, const char* fname, size_t initline) {
    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }

    ref->fname = fname;

    size_t filesize = strlen(filedata)+1;

    if ( ref == NULL || filedata == NULL || filesize == 1 ) { return ERR_NULL; }

    if ( ref->prewindow != NULL || ref->postwindow != NULL || ref->view == NULL ) { return ERR_NULL; }
    if ( ref->view->head != NULL ) { return ERR_NULL; }

    ref->prewindow = (char*)calloc(sizeof(char), filesize);
    if ( ref->prewindow == NULL ) { return ERR_MEM; }
    
    ref->postwindow = (char*)calloc(sizeof(char), filesize);
    if ( ref->postwindow == NULL ) { return ERR_MEM; }

    ref->prewindow_cap = filesize * sizeof(char);
    ref->postwindow_cap = filesize * sizeof(char);

    const char* fileptr = filedata;

    // add lines to prewindow
    for ( int i = 0; i < initline; i++ ) {

        // linesize+1 so the newline is captured
        size_t copy_size = 0;
        textErr ret = linesize(fileptr, &copy_size);
        if ( ret != ERR_NONE && ret != ERR_EOF ) { return ret; }
        copy_size += 1;
        memcpy(&ref->prewindow[ref->prewindow_len], fileptr, copy_size);
        fileptr += copy_size;

        ref->prewindow_len += copy_size;

    }

    textErr ret;

    // add lines to window
    size_t charcount = 0;
    ret = linebuf_parse(&(ref->lines), fileptr, ref->viewlines, &charcount);
    if ( ret != ERR_NONE ) { return ret; }

    fileptr += charcount;
    size_t usedchars = (uintptr_t)fileptr - (uintptr_t)filedata;

    // add lines to postwindow
    for ( size_t i = 0; i < (filesize-usedchars); i++ ) {
        ref->postwindow[i] = filedata[filesize-(i+1)];
    }

    ref->postwindow_len = (filesize-usedchars);
    
    return ERR_NONE;

}

textErr filebuf_scroll_up(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref->prewindow_len == 0 ) { return ERR_NONE; }

    size_t copycount = 0;
    for (int i = ref->prewindow_len; i > 0; i--) {
        copycount += 1;
        if ( i == '\n' ) { break; }
    }

    linebuf* tmp = ref->view->head;

    linebuf* newhead;
    textErr ret = linebuf_init(&newhead, &ref->prewindow[ref->prewindow_len-copycount], copycount);
    if ( ret != ERR_NONE ) { return ret; }

    newhead->next = ref->view->head;
    ((linebuf*)(ref->view->head))->prev = tmp;
    newhead->prev = NULL;
    ref->view->head = newhead;

    ref->prewindow[ref->prewindow_len-copycount] = '\0';
    

}