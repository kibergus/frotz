/* dumb-pic.c
 * $Id: dumb-pic.c,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 *
 * Copyright 1997,1998 Alcibiades Petrofsky
 * <alcibiades@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */
#include "dumb_frotz.h"

void dumb_init_pictures (char *filename)
{
    (void) filename;
}

bool os_picture_data(int num, int *height, int *width)
{
    (void) num;
    (void) height;
    (void) width;
    return FALSE;
}

void os_draw_picture (int num, int row, int col)
{
    (void) num;
    (void) row;
    (void) col;
}

int os_peek_colour (void) {return BLACK_COLOUR; }
