/* dumb-output.c
 * $Id: dumb-output.c,v 1.2 2002/03/26 22:52:31 feedle Exp $
 *
 * Copyright 1997,1998 Alfresco Petrofsky <alfresco@petrofsky.berkeley.edu>.
 * Any use permitted provided this notice stays intact.
 */

#include "dumb_frotz.h"
#include "chibi_frotz.h"

/* h_screen_rows * h_screen_cols */
static int screen_cells;

/* The in-memory state of the screen.  */
/* Each cell contains a style in the upper byte and a char in the lower. */
typedef uint8_t cell;
static cell *screen_data;

static cell make_cell(int style, char c) {return (style << 8) | (0xff & c);}
static char cell_char(cell c) {return c & 0xff;}
static int cell_style(cell c) {return c >> 8;}


/* A cell's style is REVERSE_STYLE, normal (0), or PICTURE_STYLE.
 * PICTURE_STYLE means the character is part of an ascii image outline
 * box.  (This just buys us the ability to turn box display on and off
 * with immediate effect.  No, not very useful, but I wanted to give
 * the rv bit some company in that huge byte I allocated for it.)  */
#define PICTURE_STYLE 16

static int current_style = 0;

/* Which cells have changed (1 byte per cell).  */
/*static*/ char *screen_changes;

static int cursor_row = 0, cursor_col = 0;

/* Compression styles.  */
static enum {
  COMPRESSION_NONE, COMPRESSION_SPANS, COMPRESSION_MAX,
} compression_mode = COMPRESSION_SPANS;
static int hide_lines = 0;

/* Reverse-video display styles.  */
static enum {
  RV_NONE, RV_DOUBLESTRIKE, RV_UNDERLINE, RV_CAPS,
} rv_mode = RV_NONE;
static char rv_blank_char = ' ';

cell *dumb_row(int r) {return screen_data + r * h_screen_cols;}

char *dumb_changes_row(int r)
{
    return screen_changes + r * h_screen_cols;
}

int os_char_width (zchar c)
{
    (void) c;
    return 1;
}

int os_string_width (const zchar *s)
{
    int width = 0;
    zchar c;

    while ((c = *s++) != 0) {
    if (c == ZC_NEW_STYLE || c == ZC_NEW_FONT)
        s++;
    else
        width += os_char_width(c);
    }
    return width;
}

void os_set_cursor(int row, int col)
{
    cursor_row = row - 1; cursor_col = col - 1;
    if (cursor_row >= h_screen_rows)
    cursor_row = h_screen_rows - 1;
}

/* Set a cell and update screen_changes.  */
static void dumb_set_cell(int row, int col, cell c)
{
    dumb_changes_row(row)[col] = (c != dumb_row(row)[col]);
    dumb_row(row)[col] = c;
}

void dumb_set_picture_cell(int row, int col, char c)
{
    dumb_set_cell(row, col, make_cell(PICTURE_STYLE, c));
}

/* Copy a cell and copy its changedness state.
 * This is used for scrolling.  */
static void dumb_copy_cell(int dest_row, int dest_col,
               int src_row, int src_col)
{
    dumb_row(dest_row)[dest_col] = dumb_row(src_row)[src_col];
    dumb_changes_row(dest_row)[dest_col] = dumb_changes_row(src_row)[src_col];
}

void os_set_text_style(int x)
{
    current_style = x & REVERSE_STYLE;
}

/* put a character in the cell at the cursor and advance the cursor.  */
static void dumb_display_char(char c)
{
    dumb_set_cell(cursor_row, cursor_col, make_cell(current_style, c));
    if (++cursor_col == h_screen_cols) {
    if (cursor_row == h_screen_rows - 1)
        cursor_col--;
    else {
        cursor_row++;
        cursor_col = 0;
    }
    }
}

void dumb_display_user_input(char *s)
{
    /* copy to screen without marking it as a change.  */
    while (*s)
    dumb_row(cursor_row)[cursor_col++] = make_cell(0, *s++);
}

void dumb_discard_old_input(int num_chars)
{
  /* Weird discard stuff.  Grep spec for 'pain in my butt'.  */
  /* The old characters should be on the screen just before the cursor.
   * Erase them.  */
    cursor_col -= num_chars;

    if (cursor_col < 0)
    cursor_col = 0;
    os_erase_area(cursor_row + 1, cursor_col + 1,
    cursor_row + 1, cursor_col + num_chars, -1);
}

void os_display_char (zchar c)
{
    if (c >= ZC_LATIN1_MIN /*&& c <= ZC_LATIN1_MAX*/) {
        dumb_display_char(c);
    } else if (c >= 32 && c <= 126) {
        dumb_display_char(c);
    } else if (c == ZC_GAP) {
        dumb_display_char(' '); dumb_display_char(' ');
    } else if (c == ZC_INDENT) {
        dumb_display_char(' '); dumb_display_char(' '); dumb_display_char(' ');
    }
  return;
}


/* Haxor your boxor? */
void os_display_string (const zchar *s)
{
    zchar c;

    while ((c = *s++) != 0) {
    if (c == ZC_NEW_FONT)
        s++;
    else if (c == ZC_NEW_STYLE)
        os_set_text_style(*s++);
    else {
        os_display_char (c);
    }
    }
}

void os_erase_area (int top, int left, int bottom, int right, int win)
{
    (void)win;
    int row, col;
    top--; left--; bottom--; right--;
    for (row = top; row <= bottom; row++) {
    for (col = left; col <= right; col++)
        dumb_set_cell(row, col, make_cell(current_style, ' '));
    }
}

void os_scroll_area (int top, int left, int bottom, int right, int units)
{
    int row, col;

    top--; left--; bottom--; right--;

    if (units > 0) {
    for (row = top; row <= bottom - units; row++) {
        for (col = left; col <= right; col++)
        dumb_copy_cell(row, col, row + units, col);
    }
    os_erase_area(bottom - units + 2, left + 1, bottom + 1, right + 1, -1 );
    } else if (units < 0) {
    for (row = bottom; row >= top - units; row--) {
        for (col = left; col <= right; col++)
        dumb_copy_cell(row, col, row + units, col);
    }
    os_erase_area(top + 1, left + 1, top - units, right + 1 , -1);
    }
}

int os_font_data(int font, int *height, int *width)
{
    if (font == TEXT_FONT) {
    *height = 1; *width = 1; return 1;
    }
    return 0;
}

void os_set_colour (int x, int y) 
{
    (void)x;
    (void)y;
}
void os_set_font (int x)
{
    (void)x;
}

/* Print a cell to stdout.  */
static void show_cell(cell cel)
{
    char c = cell_char(cel);
    switch (cell_style(cel)) {
    case 0:
        ch_fputc(c);
        break;
    case PICTURE_STYLE:
        ch_fputc(' ');
        break;
    case REVERSE_STYLE:
        if (c == ' ')
            ch_fputc(rv_blank_char);
        else
            switch (rv_mode) {
            case RV_NONE: ch_fputc(c); break;
            case RV_CAPS: ch_fputc(toupper((unsigned char)c)); break;
            case RV_UNDERLINE: ch_fputc('_'); ch_fputc('\b'); ch_fputc(c); break;
            case RV_DOUBLESTRIKE: ch_fputc(c); ch_fputc('\b'); ch_fputc(c); break;
            }
        break;
    }
}

static bool will_print_blank(cell c)
{
    return ((cell_style(c) == PICTURE_STYLE)
    || ((cell_char(c) == ' ')
     && ((cell_style(c) != REVERSE_STYLE)
    || (rv_blank_char == ' '))));
}

static void show_line_prefix(int row, char c)
{
    (void) row;
    (void) c;
}

/* Print a row to stdout.  */
static void show_row(int r)
{
    if (r == -1) {
        show_line_prefix(-1, '.');
    } else {
        int c, last;
        show_line_prefix(r, (r == cursor_row) ? ']' : ' ');
        /* Don't print spaces at end of line.  */
        /* (Saves bandwidth and printhead wear.)  */
        /* TODO: compress spaces to tabs.  */
        for (last = h_screen_cols - 1; last >= 0; last--)
            if (!will_print_blank(dumb_row(r)[last]))
                break;
        for (c = 0; c <= last; c++)
            show_cell(dumb_row(r)[c]);
    }
    ch_fputc('\r');
    ch_fputc('\n');
}

/* Print the part of the cursor row before the cursor.  */
void dumb_show_prompt(bool show_cursor, char line_type)
{
    int i;
    show_line_prefix(show_cursor ? cursor_row : -1, line_type);
    if (show_cursor) {
    for (i = 0; i < cursor_col; i++)
        show_cell(dumb_row(cursor_row)[i]);
    }
}

static void mark_all_unchanged(void)
{
    memset(screen_changes, 0, screen_cells);
}

/* Check if a cell is a blank or will display as one.
 * (Used to help decide if contents are worth printing.)  */
static bool is_blank(cell c)
{
    return ((cell_char(c) == ' ')
        || ((cell_style(c) == PICTURE_STYLE)));
}

/* Show the current screen contents, or what's changed since the last
 * call.
 *
 * If compressing, and show_cursor is true, and the cursor is past the
 * last nonblank character on the last line that would be shown, then
 * don't show that line (because it will be redundant with the prompt
 * line just below it).  */
void dumb_show_screen(bool show_cursor)
{
    int r, c, first, last;
    char changed_rows[0x100];

    /* Easy case */
    if (compression_mode == COMPRESSION_NONE) {
        for (r = hide_lines; r < h_screen_rows; r++)
            show_row(r);
        mark_all_unchanged();
        return;
    }

    /* Check which rows changed, and where the first and last change is.  */
    first = last = -1;
    memset(changed_rows, 0, h_screen_rows);
    for (r = hide_lines; r < h_screen_rows; r++) {
        for (c = 0; c < h_screen_cols; c++)
            if (dumb_changes_row(r)[c] && !is_blank(dumb_row(r)[c]))
                break;
        changed_rows[r] = (c != h_screen_cols);
        if (changed_rows[r]) {
            first = (first != -1) ? first : r;
            last = r;
        }
    }

    if (first == -1)
    return;

    /* The show_cursor rule described above */
    if (show_cursor && (cursor_row == last)) {
        for (c = cursor_col; c < h_screen_cols; c++)
            if (!is_blank(dumb_row(last)[c]))
                break;
        if (c == h_screen_cols)
            last--;
    }

    /* Display the appropriate rows.  */
    if (compression_mode == COMPRESSION_MAX) {
        for (r = first; r <= last; r++)
            if (changed_rows[r])
                show_row(r);
    } else {
        /* COMPRESSION_SPANS */
        for (r = first; r <= last; r++) {
            if (changed_rows[r] || changed_rows[r + 1]) {
                show_row(r);
            } else {
                while (!changed_rows[r + 1])
                    r++;
                show_row(-1);
            }
        }
        if (show_cursor && (cursor_row > last + 1))
            show_row((cursor_row == last + 2) ? (last + 1) : -1);
    }

    mark_all_unchanged();
}

/* Unconditionally show whole screen.  For \s user command.  */
void dumb_dump_screen(void)
{
    int r;
    for (r = 0; r < h_screen_height; r++)
    show_row(r);
}

/* Called when it's time for a more prompt but user has them turned off.  */
void dumb_elide_more_prompt(void)
{
    dumb_show_screen(FALSE);
    if (compression_mode == COMPRESSION_SPANS && hide_lines == 0) {
    show_row(-1);
    }
}

void os_reset_screen(void)
{
    dumb_show_screen(FALSE);
}

void os_beep (int volume)
{
    (void) volume;
    ch_fputc('\a'); /* so much for dumb.  */
}


/* To make the common code happy */

void os_prepare_sample (int a)
{
(void)a;
}
void os_finish_with_sample () {}
void os_start_sample (int a, int b, int c, zword d)
{
(void)a;
(void)b;
(void)c;
(void)d;
}
void os_stop_sample () {}

void dumb_init_output(void)
{
    if (h_version == V3) {
    h_config |= CONFIG_SPLITSCREEN;
    h_flags &= ~OLD_SOUND_FLAG;
    }

    if (h_version >= V5) {
    h_flags &= ~SOUND_FLAG;
    }

    h_screen_height = h_screen_rows;
    h_screen_width = h_screen_cols;
    screen_cells = h_screen_rows * h_screen_cols;

    h_font_width = 1; h_font_height = 1;

    screen_data = ch_malloc(screen_cells * sizeof(cell));
    screen_changes = ch_malloc(screen_cells);
    os_erase_area(1, 1, h_screen_rows, h_screen_cols, -2);
    memset(screen_changes, 0, screen_cells);
}
