/* hotkey.c - Hot key functions
 *    Copyright (c) 1995-1997 Stefan Jokisch
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

extern int read_number (void);

extern bool read_yes_or_no (const char *);

extern void replay_open (void);
extern void replay_close (void);
extern void record_open (void);
extern void record_close (void);

extern void seed_random (int);

/*
 * hot_key_help
 *
 * ...displays a list of all hot keys.
 *
 */
static bool hot_key_help (void) {

    print_string ("Help\n");

    print_string (
        "\n"
        "Alt-H  help\n"
        "Alt-N  new game\n"
        "Alt-X  exit game\n");

    return FALSE;

}/* hot_key_help */

/*
 * hot_key_restart
 *
 * ...allows user to start a new game.
 *
 */
static bool hot_key_restart (void)
{
    print_string ("New game\n");

    if (read_yes_or_no ("Do you wish to restart")) {

        z_restart ();
        return TRUE;

    } else return FALSE;

}/* hot_key_restart */


/*
 * hot_key_quit
 *
 * ...allows user to exit the game.
 *
 */
static bool hot_key_quit (void)
{

    print_string ("Exit game\n");

    if (read_yes_or_no ("Do you wish to quit")) {

        z_quit ();
        return TRUE;

    } else return FALSE;

}/* hot_key_quit */


/*
 * handle_hot_key
 *
 * Perform the action associated with a so-called hot key. Return
 * true to abort the current input action.
 *
 */
bool handle_hot_key (zchar key)
{
    if (cwin == 0) {

        bool aborting;

        aborting = FALSE;

        print_string ("\nHot key -- ");

        switch (key) {
            case ZC_HKEY_RESTART: aborting = hot_key_restart (); break;
            case ZC_HKEY_QUIT: aborting = hot_key_quit (); break;
            case ZC_HKEY_HELP: aborting = hot_key_help (); break;
        }

        if (aborting)
            return TRUE;

        print_string ("\nContinue input...\n");

    }

    return FALSE;

}/* handle_hot_key */
