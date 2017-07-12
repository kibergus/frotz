/* stream.c - IO stream implementation
 *        Copyright (c) 1995-1997 Stefan Jokisch
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "frotz.h"

extern bool handle_hot_key (zchar);

extern bool validate_click (void);

extern void memory_open (zword, zword, bool);
extern void memory_close (void);

extern void memory_word (const zchar *);
extern void memory_new_line (void);
extern void screen_char (zchar);
extern void screen_word (const zchar *);
extern void screen_new_line (void);
extern void screen_write_input (const zchar *, zchar);
extern void screen_erase_input (const zchar *);
extern void screen_mssg_on (void);
extern void screen_mssg_off (void);

extern zchar console_read_key (zword);
extern zchar console_read_input (int, zchar *, zword, bool);

extern int direct_call (zword);


/*
 * stream_mssg_on
 *
 * Start printing a "debugging" message.
 *
 */
void stream_mssg_on (void)
{
    flush_buffer ();

    if (ostream_screen)
    screen_mssg_on ();

    message = TRUE;

}/* stream_mssg_on */


/*
 * stream_mssg_off
 *
 * Stop printing a "debugging" message.
 *
 */
void stream_mssg_off (void)
{
    flush_buffer ();

    if (ostream_screen)
    screen_mssg_off ();

    message = FALSE;

}/* stream_mssg_off */


/*
 * z_output_stream, open or close an output stream.
 *
 *        zargs[0] = stream to open (positive) or close (negative)
 *        zargs[1] = address to redirect output to (stream 3 only)
 *        zargs[2] = width of redirected output (stream 3 only, optional)
 *
 */
void z_output_stream (void)
{
    flush_buffer ();

    switch ((short) zargs[0]) {

    case  1: ostream_screen = TRUE;
         break;
    case -1: ostream_screen = FALSE;
         break;
    case  3: memory_open (zargs[1], zargs[2], zargc >= 3);
         break;
    case -3: memory_close ();
         break;

    }

}/* z_output_stream */


/*
 * stream_char
 *
 * Send a single character to the output stream.
 *
 */
void stream_char (zchar c)
{
    if (ostream_screen)
    screen_char (c);

}/* stream_char */


/*
 * stream_word
 *
 * Send a string of characters to the output streams.
 *
 */
void stream_word (const zchar *s)
{
    if (ostream_memory && !message)
        memory_word (s);
    else {
        if (ostream_screen)
            screen_word (s);
    }

}/* stream_word */


/*
 * stream_new_line
 *
 * Send a newline to the output streams.
 *
 */
void stream_new_line (void)
{
    if (ostream_memory && !message)
        memory_new_line ();
    else {
        if (ostream_screen)
            screen_new_line ();
    }

}/* stream_new_line */


/*
 * z_input_stream, select an input stream.
 *
 *        zargs[0] = input stream to be selected
 *
 */
void z_input_stream (void)
{
    flush_buffer ();
}/* z_input_stream */


/*
 * stream_read_key
 *
 * Read a single keystroke from the current input stream.
 *
 */
zchar stream_read_key ( zword timeout, zword routine, bool hot_keys )
{
    zchar key = ZC_BAD;

    flush_buffer ();

    /* Read key from current input stream */

continue_input:

    do {
        key = console_read_key (timeout);
    } while (key == ZC_BAD);

    /* Verify mouse clicks */

    if (key == ZC_SINGLE_CLICK || key == ZC_DOUBLE_CLICK)
        if (!validate_click ())
            goto continue_input;

    /* Handle timeouts */

    if (key == ZC_TIME_OUT)
        if (direct_call (routine) == 0)
            goto continue_input;

    /* Handle hot keys */

    if (hot_keys && key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX) {

        if (h_version == V4 && key == ZC_HKEY_UNDO)
            goto continue_input;
        if (!handle_hot_key (key))
            goto continue_input;

        return ZC_BAD;

    }

    /* Return key */

    return key;

}/* stream_read_key */


/*
 * stream_read_input
 *
 * Read a line of input from the current input stream.
 *
 */
zchar stream_read_input ( int max, zchar *buf,
                          zword timeout, zword routine,
                          bool hot_keys)
{
    zchar key = ZC_BAD;

    flush_buffer ();

    /* Read input line from current input stream */

continue_input:

    do {
        key = console_read_input (max, buf, timeout, key != ZC_BAD);
    } while (key == ZC_BAD);

    /* Verify mouse clicks */

    if (key == ZC_SINGLE_CLICK || key == ZC_DOUBLE_CLICK)
    if (!validate_click ())
        goto continue_input;

    /* Handle timeouts */

    if (key == ZC_TIME_OUT)
        if (direct_call (routine) == 0)
            goto continue_input;

    /* Handle hot keys */

    if (hot_keys && key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX) {

        if (!handle_hot_key (key))
            goto continue_input;

        return ZC_BAD;

    }

    /* Return terminating key */

    return key;

}/* stream_read_input */
