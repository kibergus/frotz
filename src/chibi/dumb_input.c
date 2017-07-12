/* dumb-input.c
 * $Id: dumb-input.c,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 * Copyright 1997,1998 Alpine Petrofsky <alpine@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */

#include "dumb_frotz.h"
#include "chibi_frotz.h"

int xgetchar(void);

static float speed = 1;

enum input_type {
    INPUT_CHAR,
    INPUT_LINE,
    INPUT_LINE_CONTINUED,
};

/* Read one line, including the newline, into s.  Safely avoids buffer
 * overruns (but that's kind of pointless because there are several
 * other places where I'm not so careful).  */
static void dumb_getline(char *s)
{
    frotz_getline(s, INPUT_BUFFER_SIZE);
}

// Translate in place all the escape characters in s.
static void translate_special_chars(char *s)
{
  char *src = s, *dest = s;
  while (*src)
    switch(*src++) {
    default: *dest++ = src[-1]; break;
    case '\n': *dest++ = ZC_RETURN; break;
    case '\\':
      switch (*src++) {
      case '\n': *dest++ = ZC_RETURN; break;
      case '\\': *dest++ = '\\'; break;
      case '?': *dest++ = ZC_BACKSPACE; break;
      case '[': *dest++ = ZC_ESCAPE; break;
      case '_': *dest++ = ZC_RETURN; break;
      case '^': *dest++ = ZC_ARROW_UP; break;
      case '.': *dest++ = ZC_ARROW_DOWN; break;
      case '<': *dest++ = ZC_ARROW_LEFT; break;
      case '>': *dest++ = ZC_ARROW_RIGHT; break;
      case 'R': *dest++ = ZC_HKEY_RECORD; break;
      case 'P': *dest++ = ZC_HKEY_PLAYBACK; break;
      case 'S': *dest++ = ZC_HKEY_SEED; break;
      case 'U': *dest++ = ZC_HKEY_UNDO; break;
      case 'N': *dest++ = ZC_HKEY_RESTART; break;
      case 'X': *dest++ = ZC_HKEY_QUIT; break;
      case 'D': *dest++ = ZC_HKEY_DEBUG; break;
      case 'H': *dest++ = ZC_HKEY_HELP; break;
      case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        *dest++ = ZC_FKEY_MIN + src[-1] - '0' - 1; break;
      case '0': *dest++ = ZC_FKEY_MIN + 9; break;
      default:
        ch_fputs("DUMB-FROTZ: unknown escape char: \n");
        ch_fputc(src[-1]);
        ch_fputs("\nEnter \\help to see the list\n");
      }
    }
  *dest = '\0';
}


/* The time in tenths of seconds that the user is ahead of z time.  */
static int time_ahead = 0;

/* Called from os_read_key and os_read_line if they have input from
 * a previous call to dumb_read_line.
 * Returns TRUE if we should timeout rather than use the read-ahead.
 * (because the user is further ahead than the timeout).  */
static bool check_timeout(int timeout)
{
    if ((timeout == 0) || (timeout > time_ahead))
        time_ahead = 0;
    else
        time_ahead -= timeout;
    return time_ahead != 0;
}

double ch_atof(const char *s)
{
    double a = 0.0;
    int e = 0;
    int c;
    while ((c = *s++) != '\0' && isdigit(c)) {
        a = a*10.0 + (c - '0');
    }
    if (c == '.') {
        while ((c = *s++) != '\0' && isdigit(c)) {
            a = a*10.0 + (c - '0');
            e = e-1;
        }
    }
    if (c == 'e' || c == 'E') {
        int sign = 1;
        int i = 0;
        c = *s++;
        if (c == '+')
            c = *s++;
        else if (c == '-') {
            c = *s++;
            sign = -1;
        }
        while (isdigit(c)) {
            i = i*10 + (c - '0');
            c = *s++;
        }
        e += i*sign;
    }
    while (e > 0) {
        a *= 10.0;
        e--;
    }
    while (e < 0) {
        a *= 0.1;
        e++;
    }
    return a;
}

/* Read a line, processing commands (lines that start with a backslash
 * (that isn't the start of a special character)), and write the
 * first non-command to s.
 * Return true if timed-out.  */
static bool dumb_read_line(char *s, char *prompt, bool show_cursor,
                           int timeout, enum input_type type,
                           zchar *continued_line_chars)
{
  time_t start_time;

  if (timeout) {
    if (time_ahead >= timeout) {
      time_ahead -= timeout;
      return TRUE;
    }
    timeout -= time_ahead;
    start_time = time(0);
  }
  time_ahead = 0;

  dumb_show_screen(show_cursor);

  for (;;) {
    char *command;
    if (prompt)
      ch_fputs(prompt);
    else
      dumb_show_prompt(show_cursor, (timeout ? "tTD" : ")>}")[type]);
    // Prompt only shows up after user input if we don't flush stdout

    dumb_getline(s);
    if ((s[0] != '\\') || ((s[1] != '\0') && !islower((unsigned char)s[1]))) {
      // Is not a command line.
      translate_special_chars(s);
      if (timeout) {
        int elapsed = (time(0) - start_time) * 10 * speed;
        if (elapsed > timeout) {
          time_ahead = elapsed - timeout;
          return TRUE;
        }
      }
      return FALSE;
    }
    // Commands.

    // Remove the \ and the terminating newline.
    command = s + 1;
    command[strlen(command) - 1] = '\0';

    if (!strcmp(command, "t")) {
      if (timeout) {
        time_ahead = 0;
        s[0] = '\0';
        return TRUE;
      }
    } else if (*command == 'w') {
      if (timeout) {
        int elapsed = atoi(&command[1]);
        time_t now = time(0);
        if (elapsed == 0)
          elapsed = (now - start_time) * 10 * speed;
        if (elapsed >= timeout) {
          time_ahead = elapsed - timeout;
          s[0] = '\0';
          return TRUE;
        }
        timeout -= elapsed;
        start_time = now;
      }
    } else if (!strcmp(command, "d")) {
      if (type != INPUT_LINE_CONTINUED)
        ch_fputs("DUMB-FROTZ: No input to discard\n");
      else {
        dumb_discard_old_input(strlen((char*)continued_line_chars));
        continued_line_chars[0] = '\0';
        type = INPUT_LINE;
      }
    } else if (!strcmp(command, "s")) {
        dumb_dump_screen();
    } else {
      ch_fputs("DUMB-FROTZ: unknown command: ");
      ch_fputs(s);
    }
  }
}

// Read a line that is not part of z-machine input (more prompts and
// filename requests).
static void dumb_read_misc_line(char *s, char *prompt)
{
  dumb_read_line(s, prompt, 0, 0, 0, 0);
  // Remove terminating newline
  s[strlen(s) - 1] = '\0';
}

// For allowing the user to input in a single line keys to be returned
// for several consecutive calls to read_char, with no screen update
// in between.  Useful for traversing menus.
static char read_key_buffer[INPUT_BUFFER_SIZE];

// Similar.  Useful for using function key abbreviations.
static char read_line_buffer[INPUT_BUFFER_SIZE];

zchar os_read_key (int timeout, bool show_cursor)
{
  char c;
  int timed_out;

  // Discard any keys read for line input.
  read_line_buffer[0] = '\0';

  if (read_key_buffer[0] == '\0') {
    timed_out = dumb_read_line(read_key_buffer, NULL, show_cursor, timeout,
                               INPUT_CHAR, NULL);
    // An empty input line is reported as a single CR.
    // If there's anything else in the line, we report only the line's
    // contents and not the terminating CR.
    if (strlen(read_key_buffer) > 1)
      read_key_buffer[strlen(read_key_buffer) - 1] = '\0';
  } else
    timed_out = check_timeout(timeout);

  if (timed_out)
    return ZC_TIME_OUT;

  c = read_key_buffer[0];
  memmove(read_key_buffer, read_key_buffer + 1, strlen(read_key_buffer));

  // TODO: error messages for invalid special chars.

  return c;
}

zchar os_read_line (int max, zchar *buf, int timeout, int width, int continued)
{
  char *p;
  int terminator;
  static bool timed_out_last_time;
  int timed_out;

  // Discard any keys read for single key input.
  read_key_buffer[0] = '\0';

  // After timing out, discard any further input unless we're continuing.
  if (timed_out_last_time && !continued)
    read_line_buffer[0] = '\0';

  if (read_line_buffer[0] == '\0') {
    timed_out = dumb_read_line(read_line_buffer, NULL, TRUE, timeout,
                               buf[0] ? INPUT_LINE_CONTINUED : INPUT_LINE,
                               buf);
  } else {
    timed_out = check_timeout(timeout);
  }

  if (timed_out) {
    timed_out_last_time = TRUE;
    return ZC_TIME_OUT;
  }

  // find the terminating character.
  for (p = read_line_buffer;; p++) {
    if (is_terminator(*p)) {
      terminator = *p;
      *p++ = '\0';
      break;
    }
  }

  // TODO: Truncate to width and max.
  (void)max;
  (void)width;

  // copy to screen
  dumb_display_user_input(read_line_buffer);

  // copy to the buffer and save the rest for next time.
  strcat((char*)buf, read_line_buffer);
  p = read_line_buffer + strlen(read_line_buffer) + 1;
  memmove(read_line_buffer, p, strlen(p) + 1);

  // If there was just a newline after the terminating character,
  // don't save it.
  if ((read_line_buffer[0] == '\r') && (read_line_buffer[1] == '\0'))
    read_line_buffer[0] = '\0';

  timed_out_last_time = FALSE;
  return terminator;
}

int os_read_file_name (char *file_name, const char *default_name, int flag)
{
  (void) file_name;
  (void) default_name;
  (void) flag;
  return FALSE;
}

void os_more_prompt (void)
{
  if (do_more_prompts) {
//    char buf[INPUT_BUFFER_SIZE];
//    dumb_read_misc_line(buf, "");
  } else {
    dumb_elide_more_prompt();
  }
}

void dumb_init_input(void)
{
  if ((h_version >= V4) && (speed != 0))
    h_config |= CONFIG_TIMEDINPUT;

  if (h_version >= V5)
    h_flags &= ~(MOUSE_FLAG | MENU_FLAG);
}

zword os_read_mouse(void)
{
    // NOT IMPLEMENTED
    return 0;
}
