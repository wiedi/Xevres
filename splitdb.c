/*
Xevres (based on Operservice 2)
(C) Michael Meier 2000-2001 - released under GPL
-----------------------------------------------------------------------------
splitdb.c - this keeps a list of split servers
--------------------------------------------------------
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
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

void addsplit(char *name) {
  splitserv *x; long i; char tmpn[SRVLEN+1];
  strncpy(tmpn,name,SRVLEN);
  tmpn[SRVLEN]='\0';
  toLowerCase(tmpn);
  x=(splitserv *)(splitservers.content);
  for (i=0;i<splitservers.cursi;i++) {
    if (strcmp(x[i].name,tmpn)==0) {
      x[i].state=0; /* 0 means completly out of the net */
      return;
    }
  }
  i=array_getfreeslot(&splitservers);
  x=(splitserv *)(splitservers.content);
  x[i].time=getnettime();
  x[i].state=0;
  strcpy(x[i].name,tmpn);
}

void changesplitstatus(char *name, int newstatus) {
  splitserv *x; long i; char tmpn[SRVLEN+1];
  strncpy(tmpn,name,SRVLEN);
  tmpn[SRVLEN]='\0';
  toLowerCase(tmpn);
  addsplit(name); /* That way the following loop should always find the chan :) */
  x=(splitserv *)(splitservers.content);
  for (i=0;i<splitservers.cursi;i++) {
    if (strcmp(x[i].name,tmpn)==0) {
      x[i].state=newstatus; /* 0 means completly out of the net, 1 is relinking */
      return;
    }
  }
}

int getsplitstatus(char *name) {
  splitserv *x; long i; char tmpn[SRVLEN+1];
  strncpy(tmpn,name,SRVLEN);
  tmpn[SRVLEN]='\0';
  toLowerCase(tmpn);
  x=(splitserv *)(splitservers.content);
  for (i=0;i<splitservers.cursi;i++) {
    if (strcmp(x[i].name,tmpn)==0) {
      return x[i].state;
    }
  }
  return -1;
}

void delsplit(char *name) {
  splitserv *x; long i; char tmpn[SRVLEN+1];
  strncpy(tmpn,name,SRVLEN);
  tmpn[SRVLEN]='\0';
  toLowerCase(tmpn);
  x=(splitserv *)(splitservers.content);
  for (i=0;i<splitservers.cursi;i++) {
    if (strcmp(x[i].name,tmpn)==0) {
      array_delslot(&splitservers,i);
      return;
    }
  }
}

void clearsplits() {
  while (splitservers.cursi>0) {
    array_delslot(&splitservers,0);
  }
}

/* Removes servers that have state "RELINK" for more than 30 minutes from the splitlist */
void splitpurge() {
  splitserv *x; long i;
  for (i=0;i<splitservers.cursi;i++) {
    x=(splitserv *)(splitservers.content);
    if ((x[i].state==1) && (x[i].time+(30*60)<getnettime())) {
      array_delslot(&splitservers,i);
      i--;
      x=(splitserv *)(splitservers.content);
    }
  }
}

void dosplitlist(long unum, char *tail) {
  splitserv *x; long i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  splitpurge();
  if (splitservers.cursi<=0) {
    msgtouser(unum,"There currently aren't any registered splits.");
    return;
  }
  sprintf(tmps2,"%-50s %-6s %20s","Servername","Status","Split for");
  msgtouser(unum,tmps2);
  x=(splitserv *)(splitservers.content);
  for (i=0;i<splitservers.cursi;i++) {
    longtoduration(tmps3,getnettime()-x[i].time);
    sprintf(tmps2,"%-50s %-6s %20s",x[i].name,(x[i].state==0) ? "M.I.A." : "RELINK",tmps3);
    msgtouser(unum,tmps2);
  }
  msgtouser(unum,"--- End of list ---");
}

void dosplitdel(long unum, char *tail) {
  splitserv *x; long i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; long j=0;
  int res;
  splitpurge();
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: splitdel pattern");
    return;
  }
  if (strcmp(tmps3,"*")==0) {
    clearsplits();
    msgtouser(unum,"Splitlist cleared.");
    return;
  }
  for (i=0;i<splitservers.cursi;i++) {
    x=(splitserv *)(splitservers.content);
    if (match2strings(tmps3,x[i].name)) {
      delsplit(x[i].name);
      i=0; j++;
    }
  }
  sprintf(tmps2,"%ld entries removed from Splitlist",j);
  msgtouser(unum,tmps2);
}
