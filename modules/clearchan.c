/*
Operservice 2 - clearchan.c
Supplies a function to clear a channel by kick, kill or gline
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

#define MODNAM "clearchan"

typedef struct {
  long num;
  char ident[USERLEN+1];
  char host[HOSTLEN+1];
} clearhelp;

void doclearchan(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  int res; channel *c; array chancopy; chanuser *u; long i; char tmps7[TMPSSIZE];
  long j; clearhelp *ch; userdata *ud;
  res=sscanf(tail,"%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res<3) {
    msgtouser(unum,"Syntax: clearchan #channel type [duration] [reason]");
    msgtouser(unum,"where type 1 means kicking");
    msgtouser(unum,"           2 means killing");
    msgtouser(unum,"           3 means glining user@host");
    msgtouser(unum,"           4 means glining *@host");
    msgtouser(unum,"all users on the channel.");
    msgtouser(unum,"The duration parameter is only valid for type 3/4, default is 30 minutes.");
    msgtouser(unum,"The default reason is \"clearing channel\".");
    return;
  }
  toLowerCase(tmps3);
  c=getchanptr(tmps3);
  if (c==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  array_init(&chancopy,sizeof(clearhelp)); /* We copy the important data for */
  u=(chanuser *)c->users.content;          /* the chan so that we don't have */
  for (i=0;i<c->users.cursi;i++) {         /* to worry about NULL pointers   */
    ud=getudptr(u[i].numeric);             /* later */
    if (ud!=NULL) {
      if (!ischarinstr('o',ud->umode)) { /* Not an oper -> fuck him */
        if ((ud->numeric>>SRVSHIFT)!=tokentolong(servernumeric)) { /* One of our fakeusers */
          j=array_getfreeslot(&chancopy);
          ch=(clearhelp *)chancopy.content;
          ch[j].num=ud->numeric;
          strcpy(ch[j].host,ud->host);
          strcpy(ch[j].ident,ud->ident);
        }
      }
    }
  }
  if (res<4) { strcpy(tmps5,""); }
  if (res<5) { strcpy(tmps6,""); }
  if (strlen(tmps5)==0) { strcpy(tmps5,"clearing channel"); }
  if (strcmp(tmps4,"1")==0) {
    /* clear by kicking */
    if ((strlen(tmps5)>0) && (strlen(tmps6)>0)) { strcat(tmps5," "); strcat(tmps5,tmps6); }
    ch=(clearhelp *)chancopy.content;
    for (i=0;i<chancopy.cursi;i++) {
      longtotoken(ch[i].num,tmps2,5);
      fprintf(sockout,"%s K %s %s :%s %s\r\n",servernumeric,tmps3,tmps2,myname,tmps5);
      delchanfromuser(ch[i].num,tmps3);
      deluserfromchan(tmps3,ch[i].num);
    }
    sprintf(tmps2,"%ld users kicked from %s.",chancopy.cursi,tmps3);
    msgtouser(unum,tmps2);
  } else if (strcmp(tmps4,"2")==0) {
    /* clear by killing */
    if ((strlen(tmps5)>0) && (strlen(tmps6)>0)) { strcat(tmps5," "); strcat(tmps5,tmps6); }
    ch=(clearhelp *)chancopy.content;
    for (i=0;i<chancopy.cursi;i++) {
      longtotoken(ch[i].num,tmps2,5);
      fprintf(sockout,"%s D %s :%s %s\r\n",servernumeric,tmps2,myname,tmps5);
      deluserfromallchans(ch[i].num);
      killuser(ch[i].num);
    }
    sprintf(tmps2,"%ld users killed for being in %s.",chancopy.cursi,tmps3);
    msgtouser(unum,tmps2);
  } else if ((strcmp(tmps4,"3")==0) || (strcmp(tmps4,"4")==0)) {
    /* clear by glining */
    long dur;
    dur=durationtolong(tmps5);
    if (dur<1) { /* not a valid duration - so it's part of the reason */
      dur=durationtolong("30m");
      if (strlen(tmps6)>0) { strcat(tmps5," "); strcat(tmps5,tmps6); }
    } else {
      if (dur>EXCESSGLINELEN) {
        msgtouser(unum, "You're creating an excessively long gline. If you do so without having");
        msgtouser(unum, "a very good reason to do so, someone will hurt you with a SCSI cable.");
      }
      strcpy(tmps5,tmps6);
    }
    getauthedas(tmps6,unum);
    ch=(clearhelp *)chancopy.content;
    for (i=0;i<chancopy.cursi;i++) {
      if (strcmp(tmps4,"3")==0) {
        sprintf(tmps2,"%s@%s",ch[i].ident,ch[i].host);
      } else {
        sprintf(tmps2,"*@%s",ch[i].host);
      }
      addgline(tmps2,tmps5,tmps6,dur,1);
      sprintf(tmps7,"GLINE %s, expires in %lds, set by %s: %s (clearchan)",tmps2,dur,tmps6,tmps5);
      sendtonoticemask(NM_GLINE,tmps7);
    }
    sprintf(tmps2,"%ld users glined for being in %s. (may have hurt innocent people too)",chancopy.cursi,tmps3);
    msgtouser(unum,tmps2);
    sprintf(tmps7,"CLEARCHAN totals: %ld users glined for being in %s. (may have hurt innocent people too)",chancopy.cursi,tmps3);
    sendtonoticemask(NM_GLINE,tmps7);
  } else {
    msgtouser(unum,"type needs to be 1, 2, 3 or 4");
  }
  array_free(&chancopy);
}

/* This is called upon initialization */
void clearchan_init() {
  setmoduledesc(MODNAM,"Supplies \"clearchan\" to clear chans either by kick, gline or kills");
  registercommand2(MODNAM,"clearchan", doclearchan, 1, 900,
  "clearchan chan how\tclears a channel, i.e. removes all users on it either by\n\tkick, gline or kill");
}

/* This is called for cleanup (before the module gets unloaded) */
void clearchan_cleanup() {
  deregistercommand("clearchan");
}
