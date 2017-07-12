/* fastmem.c - Memory related functions (fast version without virtual memory)
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

#include <stdio.h>
#include <string.h>
#include "frotz.h"
#include "game_data.h"

#include "../chibi/chibi_frotz.h"

#ifdef MSDOS_16BIT

#include <alloc.h>

#define malloc(size)        farmalloc (size)
#define realloc(size,p)        farrealloc (size,p)
#define free(size)        farfree (size)
#define memcpy(d,s,n)        _fmemcpy (d,s,n)

#else

#include <stdlib.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define far

#endif

extern void seed_random (int);
extern void restart_screen (void);
extern void refresh_text_style (void);
extern void call (zword, int, zword *, int);
extern void split_window (zword);

extern FILE *os_path_open (const char *, const char *);
extern FILE *os_load_story (void);
extern int os_storyfile_seek (FILE * fp, long offset, int whence);
extern int os_storyfile_tell (FILE * fp);

extern zword save_quetzal (FILE *, FILE *);
extern zword restore_quetzal (FILE *, FILE *);

extern void erase_window (zword);

extern void (*op0_opcodes[]) (void);
extern void (*op1_opcodes[]) (void);
extern void (*op2_opcodes[]) (void);
extern void (*var_opcodes[]) (void);

/* char save_name[MAX_FILE_NAME + 1] = DEFAULT_SAVE_NAME; */
char auxilary_name[MAX_FILE_NAME + 1] = DEFAULT_AUXILARY_NAME;

const zbyte *czmp = NULL;
#define DZMP_SIZE 40000
zbyte dzmpbuf[DZMP_SIZE];
zbyte *dzmp = NULL;
zbyte *pcp = NULL;

static FILE *story_fp = NULL;

/*
 * get_header_extension
 *
 * Read a value from the header extension (former mouse table).
 *
 */
zword get_header_extension (int entry)
{
    zword addr;
    zword val;

    if (h_extension_table == 0 || entry > hx_table_size)
        return 0;

    addr = h_extension_table + 2 * entry;
    LOW_WORD (addr, val);

    return val;

}/* get_header_extension */


/*
 * set_header_extension
 *
 * Set an entry in the header extension (former mouse table).
 *
 */
void set_header_extension (int entry, zword val)
{
    zword addr;

    if (h_extension_table == 0 || entry > hx_table_size)
        return;

    addr = h_extension_table + 2 * entry;
    SET_WORD (addr, val);

}/* set_header_extension */


/*
 * restart_header
 *
 * Set all header fields which hold information about the interpreter.
 *
 */
void restart_header (void)
{
    zword screen_x_size;
    zword screen_y_size;
    zbyte font_x_size;
    zbyte font_y_size;

    int i;

    SET_BYTE (H_CONFIG, h_config);
    SET_WORD (H_FLAGS, h_flags);

    if (h_version >= V4) {
        SET_BYTE (H_INTERPRETER_NUMBER, h_interpreter_number);
        SET_BYTE (H_INTERPRETER_VERSION, h_interpreter_version);
        SET_BYTE (H_SCREEN_ROWS, h_screen_rows);
        SET_BYTE (H_SCREEN_COLS, h_screen_cols);
    }

    /* It's less trouble to use font size 1x1 for V5 games, especially
       because of a bug in the unreleased German version of "Zork 1" */

    if (h_version != V6) {
        screen_x_size = (zword) h_screen_cols;
        screen_y_size = (zword) h_screen_rows;
        font_x_size = 1;
        font_y_size = 1;
    } else {
        screen_x_size = h_screen_width;
        screen_y_size = h_screen_height;
        font_x_size = h_font_width;
        font_y_size = h_font_height;
    }

    if (h_version >= V5) {
        SET_WORD (H_SCREEN_WIDTH, screen_x_size);
        SET_WORD (H_SCREEN_HEIGHT, screen_y_size);
        SET_BYTE (H_FONT_HEIGHT, font_y_size);
        SET_BYTE (H_FONT_WIDTH, font_x_size);
        SET_BYTE (H_DEFAULT_BACKGROUND, h_default_background);
        SET_BYTE (H_DEFAULT_FOREGROUND, h_default_foreground);
    }

    if (h_version == V6)
        for (i = 0; i < 8; i++)
            storeb ((zword) (H_USER_NAME + i), h_user_name[i]);

    SET_BYTE (H_STANDARD_HIGH, h_standard_high);
    SET_BYTE (H_STANDARD_LOW, h_standard_low);

}/* restart_header */


/*
 * init_memory
 *
 * Allocate memory and load the story file.
 *
 * Data collected from http://www.russotto.net/zplet/ivl.html
 *
 */
void init_memory (void)
{
    zword addr;
    int i;

    /* Load header into memory */
    czmp = GAME_DATA;

    /* Copy header fields to global variables */
    h_version = czmp[H_VERSION];

    if (h_version < V1 || h_version > V8)
        os_fatal ("Unknown Z-code version");

    h_config = czmp[H_CONFIG];

    if (h_version == V3 && (h_config & CONFIG_BYTE_SWAPPED))
        os_fatal ("Byte swapped story file");

    h_dynamic_size =  ((zword) czmp[H_DYNAMIC_SIZE] << 8) | czmp[H_DYNAMIC_SIZE+1];
    while(h_dynamic_size > DZMP_SIZE)
        ; // ERROR need bigger buffer

    dzmp = dzmpbuf;
    memcpy(dzmp, czmp, h_dynamic_size + 1);

    LOW_WORD (H_RELEASE, h_release);
    LOW_WORD (H_RESIDENT_SIZE, h_resident_size);
    LOW_WORD (H_START_PC, h_start_pc);
    LOW_WORD (H_DICTIONARY, h_dictionary);
    LOW_WORD (H_OBJECTS, h_objects);
    LOW_WORD (H_GLOBALS, h_globals);
    LOW_WORD (H_FLAGS, h_flags);

    for (i = 0, addr = H_SERIAL; i < 6; i++, addr++)
        LOW_BYTE (addr, h_serial[i]);

    /* Auto-detect buggy story files that need special fixes */

    story_id = UNKNOWN;

    LOW_WORD (H_ABBREVIATIONS, h_abbreviations);
    LOW_WORD (H_FILE_SIZE, h_file_size);

    /* Calculate story file size in bytes */

    if (h_file_size != 0) {

        story_size = (long) 2 * h_file_size;

        if (h_version >= V4)
            story_size *= 2;
        if (h_version >= V6)
            story_size *= 2;

    } else {                /* some old games lack the file size entry */
        story_size = GAME_DATA_SIZE;
    }

    LOW_WORD (H_CHECKSUM, h_checksum);
    LOW_WORD (H_ALPHABET, h_alphabet);
    LOW_WORD (H_FUNCTIONS_OFFSET, h_functions_offset);
    LOW_WORD (H_STRINGS_OFFSET, h_strings_offset);
    LOW_WORD (H_TERMINATING_KEYS, h_terminating_keys);
    LOW_WORD (H_EXTENSION_TABLE, h_extension_table);

    /* Adjust opcode tables */

    if (h_version <= V4) {
        op0_opcodes[0x09] = z_pop;
        op1_opcodes[0x0f] = z_not;
    } else {
        op0_opcodes[0x09] = z_catch;
        op1_opcodes[0x0f] = z_call_n;
    }

    /* Read header extension table */

    hx_table_size = get_header_extension (HX_TABLE_SIZE);
    hx_unicode_table = get_header_extension (HX_UNICODE_TABLE);

}/* init_memory */


/*
 * reset_memory
 *
 * Close the story file and deallocate memory.
 *
 */
void reset_memory (void)
{
    story_fp = NULL;

    czmp = NULL;
}/* reset_memory */


/*
 * storeb
 *
 * Write a byte value to the dynamic Z-machine memory.
 *
 */
void storeb (zword addr, zbyte value)
{
    if (addr >= h_dynamic_size)
        runtime_error (ERR_STORE_RANGE);

    if (addr == H_FLAGS + 1) {        /* flags register is modified */

        h_flags &= ~(SCRIPTING_FLAG | FIXED_FONT_FLAG);
        h_flags |= value & (SCRIPTING_FLAG | FIXED_FONT_FLAG);

        refresh_text_style ();

    }

    SET_BYTE (addr, value);

}/* storeb */


/*
 * storew
 *
 * Write a word value to the dynamic Z-machine memory.
 *
 */
void storew (zword addr, zword value)
{
    storeb ((zword) (addr + 0), hi (value));
    storeb ((zword) (addr + 1), lo (value));

}/* storew */

void frotzResetState(void);

/*
 * z_restart, re-load dynamic area, clear the stack and set the PC.
 *
 *         no zargs used
 *
 */
void z_restart (void)
{
    static bool first_restart = TRUE;

    flush_buffer ();

    os_restart_game (RESTART_BEGIN);

    seed_random (0);
    frotzResetState();

    if (!first_restart) {
        memcpy(dzmp, czmp, h_dynamic_size + 1);
    } else {
        first_restart = FALSE;
    }

    restart_header ();
    restart_screen ();

    sp = fp = stack + STACK_SIZE;
    frame_count = 0;

    if (h_version != V6) {

        long pc = (long) h_start_pc;
        SET_PC (pc);

    } else call (h_start_pc, 0, NULL, 0);

    os_restart_game (RESTART_END);

}/* z_restart */


/*
 * get_default_name
 *
 * Read a default file name from the memory of the Z-machine and
 * copy it to a string.
 *
 */
static void get_default_name (char *default_name, zword addr)
{
    if (addr != 0) {

        zbyte len;
        int i;

        LOW_BYTE (addr, len);
        addr++;

        for (i = 0; i < len; i++) {

            zbyte c;

            LOW_BYTE (addr, c);
            addr++;

            if (c >= 'A' && c <= 'Z')
                c += 'a' - 'A';

            default_name[i] = c;

        }

        default_name[i] = 0;

        if (strchr (default_name, '.') == NULL)
            strcpy (default_name + i, ".AUX");

    } else strcpy (default_name, f_setup.aux_name);

}/* get_default_name */

extern int frotzShouldRestore;
extern int frotzShouldSave;

/*
 * z_restore, restore [a part of] a Z-machine state from disk
 *
 *        zargs[0] = address of area to restore (optional)
 *        zargs[1] = number of bytes to restore
 *        zargs[2] = address of suggested file name
 *
 */
void z_restore (void)
{
    frotzShouldRestore = 1;

    zword success = 1;
    store (success);
}/* z_restore */

void z_restore_undo (void)
{
    store ((zword) -1);
}/* z_restore_undo */

void z_save_undo (void)
{
    store ((zword)-1);

}/* z_save_undo */

/*
 * z_save, save [a part of] the Z-machine state to disk.
 *
 *        zargs[0] = address of memory area to save (optional)
 *        zargs[1] = number of bytes to save
 *        zargs[2] = address of suggested file name
 *
 */
void z_save (void)
{
    frotzShouldSave = 1;

    zword success = 1;
    store (success);
}/* z_save */


/*
 * z_verify, check the story file integrity.
 *
 *        no zargs used
 *
 */
void z_verify (void)
{
    zword checksum = 0;
    long i;

    /* Sum all bytes in story file except header bytes */
    for (i = 64; i < story_size; i++)
        checksum += GAME_DATA[i];

    /* Branch if the checksums are equal */

    branch (checksum == h_checksum);

}/* z_verify */
