/*
Operservice 2 - raw.c
Loadable module
(C) Sebastian Wiedenroth 2004 - released under GPL

------[ What it does ]-----

It is just a better version of the standart "raw" command.
Now you can enter real raw data (including the numeric) to send to the network.
Example:

<Wiedi> fakeadd ruby ruby licht-und-seelenschein-security-systems.net Ruby Blumenkraft
<O> Fake user created
<Wiedi> fakelist
<O> Numeric    nick!ident@host (Realname) 
<O> ACAAL      ruby!ruby@licht-und-seelenschein-security-systems.net (Ruby Blumenkraft)
<O> --- End of list ---
<Wiedi> realraw ACAAL J #grün 235235235
--> ruby (ruby@licht-und-seelenschein-security-systems.net) has joined #grün
<O> Raw done.
<Wiedi> realraw ACAAL L #grün ewigeblumenkraft
<-- ruby (ruby@licht-und-seelenschein-security-systems.net) has left #grün (ewigeblumenkraft)
<O> Raw done.

Have fun with it, you can do a lot of stupid things with it (-;
Wiedi

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
#include <string.h>
#include <time.h>
#include "globals.h"
#define MODNAM      "raw"
void dorrawcmd(long unum, char *tail) {
  int res; 
  char cmdname[TMPSSIZE]; 
  char raw[TMPSSIZE];     
  char tmpstr[TMPSSIZE];  
  res = sscanf(tail, "%s %[^\n]", cmdname, raw);
  if (res < 2) {
    sprintf(tmpstr, "Syntax: %s numeric commands", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  raw[450]='\0';
  fprintf(sockout, "%s\r\n", raw);
  fflush(sockout);
  msgtouser(unum, "RealRaw done. Rock-n-Roll baby");
}
void raw_init() {
  setmoduledesc(MODNAM, "Here you can do REAL RAW commands w0000t");
  registercommand2(MODNAM, "realraw", dorrawcmd, 1, 999,
          "realraw mode commands\tCommand to send real raw server messages");
}
void raw_cleanup() {
  deregistercommand("realraw");
}
