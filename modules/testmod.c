/*
Operservice 2 - testmod.c
Sceleton for a loadable module
(C) Michael Meier 2000-2001 - released under GPL
-----------------------------------------------------------------------------
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "globals.h"

/* General note: globals.h defines a lot of global variables, functions and
   datatypes. Use them, they make life a lot easier... */

/* Because the name of the mod is used in several places, we define it here so
   we can easily change it. It should be all lowercase. */
#define MODNAM "testmod"
/* Note: This .c-file will have to be named MODNAM.c! */

/* This is the actual command this mod provides. Yes, it IS useless
 * The parameters it gets:
 * unum - numeric of the client calling this command
 * tail - string that contains the whole command with all parameters.
 *        should not be modified - although you won't break anything if you do
 *        in the current implementation.
 */
void dotestcmd(long unum, char *tail) {
  msgtouser(unum,"test - 2 - 3 - 4");
}

/* This is called upon initialization
   The name of the function is important, it has to be MODNAM_init! */
void testmod_init() {
  /* I think we all know what printf does... */
  printf("Hooray, this is testmod.c init!\n");
  /* setmoduledesc is there to set a short description of the module that will
     be shown on "lsmod". Syntax should be pretty self explaining... */
  setmoduledesc(MODNAM,"Just a short demomodule, that doesn't really make sense...");
  /* This registers the command this module supplies, that means it tells
     O's core which command this module supplies. Because a single module can
     supply more than one command, there can be more than one of these lines.
     Don't forget to deregister all commands in the cleanup()-function though.
     Syntax: registercommand2(modulename,command,function to call,operonly,
               minlev,helptext)
     where
     modulename is of course the modulename (just use MODNAM as in the
       example to prevent mistakes)
     command is which command this module provides (all lowercase!)
     function to call is the function that gets called when a user executes
       that command. Has to be a void func that takes a long and a char * as
       parameter
     operonly can be 0 or 1. If 1, only opers can call the command.
     minlev ist the authlevel that is required to call this command (0-1000)
     helptext is the short helptext that shows up on "showcommands". It can
       contain newlines (\n) if the text is too long to fit in one line. You
       can also use "\t" as formatting help. The example below will look like
       this on showcommands:
       test          this command does nothing, it is useless
                     shit
  */
  registercommand2(MODNAM,"test", dotestcmd, 1, 10,
  "test\tthis command does nothing, it is useless\n\tshit");
}

/* This is called for cleanup (before the module gets unloaded)
   Again the name of the function has to be MODNAM_cleanup */
void testmod_cleanup() {
  printf("Hooray, this is testmod.c cleanup!\n");
  /* NEVER forget to deregister your commands! Else people will die! The
   * world as we know it will cease to exist! You'll spend eternity in hell! */
  deregistercommand("test");
}
