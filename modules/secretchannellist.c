/*
Operservice 2 - secretchannelcount.c
Module for operservice 2.33 and greater (hopefully)
(C) Rasmus Have 2001 &
(C) Michael Meier 2000-2002 - released under GPL
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

/* Because the name of the mod is used in several places, we define it here so
   we can easily change it. It should be all lowercase. */
#define MODNAM "secretchannellist"

void scl(long unum, char *tail) {
  char cbuffer[513];
  int limit;
  int i;

  char commandname[TMPSSIZE], limitstr[TMPSSIZE], garbage[TMPSSIZE];
  int res;
  char *endptr;

  channel *mychannel;
  int channelsfound;

  res=sscanf(tail,"%s %s %[^\n]",commandname,limitstr,garbage);

  if (res<2) { /* We only got the commandname in the tail string */
    limit=1; /* "This wont hurt a bit..." *sting* "AAAAAAAAAUUUUUUUUUUUUUUUCCCCCCCHHHHHH!!! FFS!!!" */
  } else { /* We got another parameter, lets see if its a valid limit (eg. an int in string form). */
    limit=strtol(limitstr, &endptr, 10);
    if (endptr!=NULL && *endptr!='\0') /* if the endptr isnt NULL and its poiting at something not the end of the string, we've had an error. */
      limit=1;
  }

  sprintf(cbuffer,"Listing +s'ed and +p'ed channels with more than %i users:",limit);
  msgtouser(unum,cbuffer);

  sprintf(cbuffer,"Nr   channelname               users topic");
  msgtouser(unum,cbuffer);

  channelsfound = 0;
  for (i=0;i<SIZEOFCL;i++){ /* Run through every list in the hashed array */
    mychannel = cls[i];
    while (mychannel!=NULL){
      if ((isflagset(mychannel->flags,cm_s) || isflagset(mychannel->flags,cm_p)) && mychannel->users.cursi>=limit) {
        channelsfound++;
        if (channelsfound<1000) {
          sprintf(cbuffer,"%3i: %-25s %4ld %s",channelsfound, mychannel->name, mychannel->users.cursi,mychannel->topic);
          msgtouser(unum,cbuffer);
        } else {
          if (channelsfound==1000) { msgtouser(unum,"More than 1000 matches - list truncated"); }
        }
      }
      mychannel = (void *)mychannel->next;
    }
  }
  sprintf(cbuffer,"Done. %i channels found.",channelsfound);
  msgtouser(unum,cbuffer);
}

/* This is called upon initialization
   The name of the function is important, it has to be MODNAM_init! */
void secretchannellist_init() {
  setmoduledesc(MODNAM,"Module that supplies the scl command to count secret channels.");
  registercommand2(MODNAM,"scl", scl, 1, 900,
  "scl [limit]\tCommand to list secret channels (+p/+s) of size greater than limit.\n\tWarning: limit defaults to 1.");
}

/* This is called for cleanup (before the module gets unloaded)
   Again the name of the function has to be MODNAM_cleanup */
void secretchannellist_cleanup() {
  /* NEVER forget to deregister your commands! Else people will die! The
   * world as we know it will cease to exist! You'll spend eternity in hell! */
  deregistercommand("scl");
}
