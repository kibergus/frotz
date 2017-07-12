/* dumb-init.c
 * $Id: dumb-init.c,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 *
 * Copyright 1997,1998 Alva Petrofsky <alva@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */

#include "dumb_frotz.h"
#include "chibi_frotz.h"

f_setup_t f_setup;

/* A unix-like getopt, but with the names changed to avoid any problems.  */
static int user_screen_width = 75;
static int user_screen_height = 24;
static int user_interpreter_number = -1;
extern int user_random_seed;
static int user_tandy_bit = 0;
static char *graphics_filename = NULL;

bool do_more_prompts;

int os_process_arguments(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    do_more_prompts = TRUE;
    return 0;
}

void os_init_screen(void)
{
    if (h_version == V3 && user_tandy_bit)
        h_config |= CONFIG_TANDY;

    h_flags &= ~UNDO_FLAG;

    h_screen_rows = user_screen_height;
    h_screen_cols = user_screen_width;

    if (user_interpreter_number > 0)
        h_interpreter_number = user_interpreter_number;
    else {
        /* Use ms-dos for v6 (because that's what most people have the
        * graphics files for), but don't use it for v5 (or Beyond Zork
        * will try to use funky characters).  */
        h_interpreter_number = h_version == 6 ? INTERP_MSDOS : INTERP_DEC_20;
    }
    h_interpreter_version = 'F';

    dumb_init_input();
    dumb_init_output();
    dumb_init_pictures(graphics_filename);
}

void os_restart_game (int stage)
{
    (void)stage;
}


void os_init_setup(void)
{
    f_setup.attribute_assignment = 0;
    f_setup.attribute_testing = 0;
    f_setup.context_lines = 0;
    f_setup.object_locating = 0;
    f_setup.object_movement = 0;
    f_setup.left_margin = 0;
    f_setup.right_margin = 0;
    f_setup.ignore_errors = 0;
    f_setup.piracy = 0;
    f_setup.expand_abbreviations = 0;
    f_setup.script_cols = 80;
    f_setup.sound = 1;
    f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;
    f_setup.restore_mode = 0;
}
