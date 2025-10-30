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
        // length is the number of meaningful bytes (excluding the added NUL terminator)
        ref->len = strsize;
        ref->cap = strsize;
    }

    return ERR_NONE;

}

textErr linebuf_parse(linebuf** inst, const char* src, size_t maxlines, size_t *charcount) {
    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( src == NULL ) { return ERR_NULL; }

    size_t copyposition = 0;
    size_t lines_parsed = 0;

    linebuf* head = NULL;
    linebuf* tail = NULL;

    for ( size_t line = 0; line < maxlines; line++ ) {

        size_t copysize = 0;
        textErr lret = linesize(&src[copyposition], &copysize);
        if ( lret != ERR_NONE && lret != ERR_EOF ) { return lret; }

        // Include trailing newline in the copied line only when not at EOF
        size_t to_copy = copysize + ((lret == ERR_NONE) ? 1 : 0);

        if ( to_copy == 0 ) { break; }

        linebuf* node = NULL;
        textErr ret = linebuf_init(&node, &src[copyposition], to_copy);
        if ( ret != ERR_NONE ) { return ret; }

        // append to list
        node->prev = (struct linebuf*)tail;
        node->next = NULL;
        if ( tail != NULL ) {
            tail->next = (struct linebuf*)node;
        } else {
            head = node;
        }
        tail = node;

        copyposition += to_copy;
        lines_parsed += 1;

        if ( lret == ERR_EOF ) { break; }
    }

    if ( charcount != NULL ) {
        *charcount = copyposition;
    }

    ref = head;

    return ERR_NONE;

}

textErr viewbuf_init(viewbuf** inst, linebuf* head, size_t maxlines) {
    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref != NULL ) { return ERR_NULL; }

    ref = (viewbuf*)calloc(1, sizeof(viewbuf));
    if ( ref == NULL ) { return ERR_MEM; }

    ref->head = head;
    ref->headline = 1;
    ref->lines = 0;

    if ( head != NULL ) {
        linebuf* cur = head;
        size_t linelim = (maxlines == 0) ? SIZE_MAX : maxlines;
        for ( size_t line = 0; line < linelim && cur != NULL; line++ ) {
            ref->lines += 1;
            cur = linebuf_next(cur);
        }
    }

    return ERR_NONE;

}

textErr viewbuf_remove_empty_lines(viewbuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref == NULL ) { return ERR_NULL; }
    
    linebuf* node = ref->head;
    if ( node == NULL ) { return ERR_NONE; }

    while ( node ) {

        linebuf* next = node->next;

        if ( node->len == 0 ) {

            if ( node->prev != NULL ) {
                node->prev->next = node->next;
            }

            if ( node->next != NULL ) {
                node->next->prev = node->prev;
            }

            free(node->line);
            free(node);

        }

        node = next;

    }

    return ERR_NONE;

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

textErr filebuf_load(filebuf** inst, const char* filedata, const char* fname) {
    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }

    ref->fname = fname;

    size_t data_len = strlen(filedata);
    size_t filesize = data_len + 1; // includes terminating NUL

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

    textErr ret;

    // // add lines to window
    // size_t charcount = 0;
    // ret = linebuf_parse(&(ref->lines), fileptr, ref->viewlines, &charcount);
    // if ( ret != ERR_NONE ) { return ret; }

    // fileptr += charcount;
    // size_t usedchars = (uintptr_t)fileptr - (uintptr_t)filedata;

    // // Initialize view to parsed lines
    // ref->view->head = ref->lines;
    // ref->view->headline = 1;

    // // Count how many lines parsed to set view->lines
    // size_t parsed_lines = 0;
    // {
    //     linebuf* cur = ref->lines;
    //     while ( cur != NULL ) {
    //         parsed_lines += 1;
    //         cur = linebuf_next(cur);
    //     }
    // }
    // ref->view->lines = parsed_lines;

    // // add remaining bytes to postwindow in reverse order
    // size_t remaining = (usedchars <= data_len) ? (data_len - usedchars) : 0;
    // if ( remaining > 0 ) {

    //     for (size_t i = 0; i < remaining; i++) {
    //         ref->postwindow[i] = fileptr[filesize - 1 - i];
    //     }

    // }
    // ref->postwindow_len = remaining;
    
    for ( size_t i = 0; i < filesize; i++ ) {
        ref->postwindow[filesize-1-i] = filedata[i];
    }
    ref->postwindow_len = filesize;

    ret = filebuf_resize(inst);
    if ( ret != ERR_NONE ) {
        return ret;
    }

    return ERR_NONE;

}

textErr filebuf_consume_prewindow_line(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref->prewindow_len == 0 ) { return ERR_EOF; }

    // Find the start of the last complete line in prewindow
    size_t end = ref->prewindow_len; // exclusive
    size_t start = 0;
    for ( size_t i = end; i > 0; i-- ) {
        if ( ref->prewindow[i-1] == '\n' && (i-1) != (end-1) ) {
            start = i;
            break;
        }
    }

    size_t copycount = end - start;

    linebuf* newhead = NULL;
    textErr ret = linebuf_init(&newhead, &ref->prewindow[start], copycount);
    if ( ret != ERR_NONE ) { return ret; }

    newhead->prev = NULL;
    newhead->next = (struct linebuf*)ref->view->head;
    if ( ref->view->head != NULL ) {
        ref->view->head->prev = newhead;
    }
    ref->view->head = newhead;

    // Shrink prewindow
    ref->prewindow_len -= copycount;
    if ( ref->prewindow_len < ref->prewindow_cap ) {
        ref->prewindow[ref->prewindow_len] = '\0';
    }

    ref->view->lines += 1;

    return ERR_NONE;

}

textErr filebuf_consume_postwindow_line(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref->postwindow_len == 0 ) { return ERR_EOF; }

    size_t copycount = 0;
    for ( size_t i = ref->postwindow_len - 1; i > 0; i-- ) {
        copycount += 1;
        if ( ref->postwindow[i] == '\n' ) { break; }
    }

    linebuf* newtail = NULL;
    textErr ret = linebuf_init(&newtail, NULL, 0);
    if ( ret != ERR_NONE ) { return ret; }

    if ( ref->view->head == NULL ) {
        ref->view->head = newtail;
        newtail->prev = NULL;
        newtail->next = NULL;
    } else {

        linebuf* tail = ref->view->head;
        while (tail && tail->next) {
            tail = (linebuf*)tail->next;
        }

        newtail->prev = tail;
        newtail->next = NULL;
        tail->next = newtail;

        newtail->line = (char*)malloc(copycount+1);
        newtail->len = copycount;
        newtail->cap = copycount;

    }

    newtail->line = calloc(copycount, 1);
    if ( newtail->line == NULL ) {
        return ERR_MEM;
    }

    newtail->len = copycount;
    newtail->cap = copycount;
    
    for ( int i = 0; i < copycount; i++ ) {
        newtail->line[i] = ref->postwindow[ref->postwindow_len-1-i];
    }
    ref->postwindow_len -= copycount;

    ref->view->lines += 1;

    return ERR_NONE;

}

textErr filebuf_return_prewindow_line(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref->view->head == NULL ) { return ERR_NONE; }

    linebuf* oldhead = ref->view->head;
    ref->view->head = (linebuf*)oldhead->next;
    if ( ref->view->head != NULL ) {
        ((linebuf*)(ref->view->head))->prev = NULL;
    }

    size_t linelen = oldhead->len;
    memmove(&ref->prewindow[ref->prewindow_len], oldhead->line, linelen);
    ref->prewindow_len += linelen;

    free(oldhead->line);
    free(oldhead);

    ref->view->lines -= 1;

    return ERR_NONE;

}

textErr filebuf_return_postwindow_line(filebuf** inst) {
    
    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }

    if ( ref->view->head == NULL ) { return ERR_NONE; }

    linebuf* oldhead = ref->view->head;
    ref->view->head = (linebuf*)oldhead->next;
    if ( ref->view->head != NULL ) {
        ((linebuf*)(ref->view->head))->prev = NULL;
    }

    size_t linelen = oldhead->len;
    for (size_t i = 0; i < linelen; i++) {
        ref->postwindow[i] = oldhead->line[linelen - 1 - i];
    }
    // memmove(&ref->postwindow[linelen], &ref->postwindow[0], ref->postwindow_len);
    // memcpy(&ref->postwindow[0], oldhead->line, linelen);
    ref->postwindow_len += linelen;

    free(oldhead->line);
    free(oldhead);

    ref->view->lines -= 1;

    return ERR_NONE;

}

textErr filebuf_resize(filebuf** inst) {

    #define ref (*inst)

    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref->view == NULL ) { return ERR_NULL; }
    if ( ref->viewlines == 0 ) { return ERR_NULL; }

    // do nothing if we are already at the correct size
    if ( ref->viewlines == ref->view->lines ) { return ERR_NONE; }

    while ( ref->viewlines > ref->view->lines ) {
        textErr ret = filebuf_consume_postwindow_line(inst);
        if ( ret != ERR_NONE ) { return ret; }
    }

    while ( ref->viewlines < ref->view->lines ) {
        textErr ret = filebuf_return_postwindow_line(inst);
        if ( ret != ERR_NONE ) { return ret; }
    }

    return ERR_NONE;

}

textErr filebuf_scroll_down(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref->view->head == NULL ) { return ERR_NULL; }

    textErr ret = filebuf_consume_postwindow_line(inst);
    if ( ret != ERR_NONE ) { return ret; }

    ret = filebuf_return_prewindow_line(inst);
    if ( ret != ERR_NONE ) { return ret; }

    ref->view->headline += 1;

    return ERR_NONE;

}

textErr filebuf_scroll_up(filebuf** inst) {

    #define ref (*inst)
    if ( inst == NULL ) { return ERR_NULL; }
    if ( ref->view->head == NULL ) { return ERR_NULL; }

    textErr ret = filebuf_consume_prewindow_line(inst);
    if ( ret != ERR_NONE ) { return ret; }

    ret = filebuf_return_postwindow_line(inst);
    if ( ret != ERR_NONE ) { return ret; }

    ref->view->headline -= 1;

    return ERR_NONE;

}
