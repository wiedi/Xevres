/*
Operservice 2 - trustcommands.c
Loadable module
(C) Michael Meier 2002 - released under GPL
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
#include <regex.h>
#include "globals.h"
#ifndef WE_R_ON_BSD
#include <malloc.h>
#endif
#ifdef USE_DOUGLEA_MALLOC
#include "../dlmalloc/malloc.h"
#endif

/* Because the name of the mod is used in several places, we define it here so
   we can easily change it. It should be all lowercase. */
#define MODNAM "spewcommands"
/* Note: This .c-file will have to be named MODNAM.c! */
#define SHOWTRUNC     500

void dospew(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  int i; int searchtype; int j; int onlyemptychans=0;
  userdata *a; channel **d; int chanmodes; int totalchannels; array chansfound; channel **d2; int k;
  int doesmatch=0; unsigned long snip=0, snmask=0; char * mycip;
  res=sscanf(tail,"%s %[^\n]",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: spew [-nochan] host[/mask]|searchmask");
    msgtouser(unum,"anything that does not contain wildcards is treated as a host,");
    msgtouser(unum,"anything that does as a searchmask that is matched against 'realname nick!user@host'");
    msgtouser(unum,"host can be of the IP/mask notation to display a subnet.");
    msgtouser(unum,"Specifying '-nochan' will only list Users that are not on any (nonlocal-) channels");
    return;
  }
  if (!strncmp(tmps3,"-nochan ",8)) {
    memmove(tmps3,tmps3+8,TMPSSIZE-8);
    onlyemptychans=1;
  }
  searchtype=0; // type: host
  for (i=0;i<strlen(tmps3);i++) {
    if ((tmps3[i]=='*') || (tmps3[i]=='?') || (tmps3[i]=='\\')) { searchtype=1; /* searchmask */ }
    if ((searchtype==0) && (tmps3[i]=='/')) { searchtype=2; /* Subnet */ }
  }
  if (searchtype==2) { /* Subnet search - extract IP and mask */
    char dc1, dc2; // Dummys
    res=sscanf(tmps3,"%[0-9.]%c%[0-9]%c",tmps2,&dc1,tmps4,&dc2);
    if ((res!=3) || (dc1!='/')) {
      msgtouser(unum,"Not a valid subnetspecification"); return;
    }
    snip=parseipv4(tmps2);
    snmask=strtoul(tmps4,NULL,10);
    if (snmask>32) { msgtouser(unum,"a mask >32 is not possible on ipv4"); return; }
    snmask=netmasks[snmask];
  }
  unescapestring(tmps3,tmps2);
  toLowerCase(tmps2);
  strcpy(tmps3,tmps2);
  if (searchtype==1) {
    newmsgtouser(unum,"Users matching %s",tmps3);
  } else {
    newmsgtouser(unum,"Users on %s",tmps3);
  }
  array_init(&chansfound, sizeof(channel *));
  j=0; totalchannels=0; 
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    if (j>500) { break; }
    while (a!=NULL) {
      if (j>500) { break; }
      strcpy(tmps5,tmps3);
      if (searchtype==1) {
        sprintf(tmps2,"%s %s!%s@%s",a->realname,a->nick,a->ident,a->host);
        toLowerCase(tmps2);
        doesmatch=match2strings(tmps5,tmps2);
      } else if (searchtype==0) {
        strcpy(tmps2,a->host);
        toLowerCase(tmps2);
        doesmatch=match2strings(tmps5,tmps2);
      } else {
        doesmatch=((a->realip&snmask)==(snip&snmask));
      }
      if ((onlyemptychans==1) && (a->chans.cursi>0)) { doesmatch=0; }
      if (doesmatch) {
        int iamtired=0;
        j++;
        mycip=printipv4(a->realip);
        newmsgtouser(unum,"%s!%s@%s(=%s) (%s)",a->nick,a->ident,a->host,mycip,a->realname);
        free(mycip);
        if (a->chans.cursi>0) {
          if (a->chans.cursi==1) {
            strcpy(tmps4,"  On channel:");
          } else {
            strcpy(tmps4,"  On channels:");
          }
          for (iamtired=0;iamtired<(a->chans.cursi);iamtired++) {
            d=(channel **)(a->chans.content);
            totalchannels++;
            d2=(channel **)(chansfound.content);
            for(k=0;k<chansfound.cursi;k++) {
              if (d2[k]==d[iamtired]) { break; }
            }
            if (k==chansfound.cursi) {
              k=array_getfreeslot(&chansfound);
              d2=(channel **)(chansfound.content);
              d2[k]=d[iamtired];
            }
            if ((strlen(d[iamtired]->name)+strlen(tmps4))>400) {
              strcat(tmps4," [...]"); break;
            } else {
              chanmodes=getchanmode2(d[iamtired],a->numeric);
              if (chanmodes<0) {
                strcat(tmps4," ?");
              } else {
                if (isflagset(chanmodes,um_o)) { strcat(tmps4," @"); } else { strcat(tmps4,"  "); }
              }
              strcat(tmps4,d[iamtired]->name);
            }
          }
        } else {
          strcpy(tmps4,"  Not on any (nonlocal-)channels");
        }
        msgtouser(unum,tmps4);
      }
      a=(void *)a->next;
    }
  }
  if (j>500) { msgtouser(unum,"-- More than 500 matches - list truncated"); }
  newmsgtouser(unum,"-- End of list -- Found %d user%s on %d channel%s total, %ld unique",
    j,(j==1) ? "" : "s",totalchannels, (totalchannels==1)?"":"s",chansfound.cursi);
  array_free(&chansfound);
}

void dospewchancmd(long unum, char *tail) {
  channel *tchan;       // Struct for channel data
  chanuser *cuser;      // Struct for one user of a channel
  userdata *udata;      // Struct for User Data as of User DB
  int res;          // Int for results
  int i, y;         // Int for loops
  int ccount;         // Int for couting the amount of channels found
  int gotchanserv;      // Int for flag usage
  int morechanservs;
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char mask[TMPSSIZE];    // Char for the user's nickname
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char tmpstr2[TMPSSIZE];   // Char for several purposes
  char chanservs[TMPSSIZE]; // Char for the namelist of all chanservs
  // Simple error checking
  res = sscanf(tail, "%s %s", cmdname, mask);
  if (res < 2) {
    newmsgtouser(unum, "Syntax: %s mask", cmdname);
    return;
  }
  toLowerCase(mask);
  ccount = 0;
  for (i = 0; i < SIZEOFCL; i++) {
    tchan = cls[i];
    while (tchan != NULL) {
      if (match2strings(mask, tchan->name)) {
        gotchanserv = 0; morechanservs=0;
        chanservs[0] = '\0';
        cuser = (chanuser *)(tchan->users.content);
        for (y=0; y < tchan->users.cursi; y++) {
            udata = getudptr(cuser[y].numeric);
            if (udata != NULL) {
            if (ischarinstr('k', udata->umode)) {
              // Channelservice on channel
              gotchanserv++;
              if (strlen(chanservs) < 300) {
                if (strlen(chanservs) > 0) { strcat(chanservs, ", "); }
                strcat(chanservs, udata->nick);
              } else {
                morechanservs++;
              }
            }
          }
        }
        if (morechanservs > 0) {
          // 300 letters exceeded, adding ...
          sprintf(tmpstr2,", and %d more",morechanservs);
          strcat(chanservs, tmpstr2);
        }
        sprintf(tmpstr2, "%d", gotchanserv);
        sprintf(tmpstr, "%-30s %5ld %-8s%s%s%-11s%s%s%s",
             tchan->name,
             tchan->users.cursi,
             (tchan->users.cursi > 1) ? "users" : "user",
             (gotchanserv == 0) ? "" : "- found ",
             (gotchanserv == 0) ? "" : tmpstr2,
             (gotchanserv == 0) ? "" : ((gotchanserv > 1) ? " chanservs" : " chanserv"),
             (gotchanserv == 0) ? "" : "(",
             (gotchanserv == 0) ? "" : chanservs,
             (gotchanserv == 0) ? "" : ")");
        if (ccount<SHOWTRUNC) { msgtouser(unum, tmpstr); }
        ccount++;
      }
      tchan = (void *) tchan->next;
    }
  }
  if (ccount >= SHOWTRUNC) {
    sprintf(tmpstr, "Found %i channels matching pattern %s. (only first %i shown)", ccount, mask, SHOWTRUNC);
  } else {
    sprintf(tmpstr, "Found %i channels matching pattern %s.", ccount, mask);
  }
  msgtouser(unum, tmpstr);
}

void dosubnetlist(long unum, char *tail) {
  msgtouser(unum,"This command is OBSOLETE. Please use spew for this purpose.");
  msgtouser(unum,"e.g. to do what subnetlist 11.22.33 did, you can use: spew 11.22.33.0/24");
}

void docountusers(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) { strcpy(tmps3,"*"); }
  res=countusershit(tmps3);
  sprintf(tmps2,"%d user%s match %s",res,(res==1) ? "" : "s",tmps3);
  msgtouser(unum,tmps2);
}

void spewcommands_init() {
  setmoduledesc(MODNAM, "Mod containing some commands for searching user/channellists");
  registercommand2(MODNAM, "spew", dospew, 1, 600,
          "spew host|searchmask\tLists all clients on a certain host or matching a searchmask");
  registercommand2(MODNAM, "spewchan", dospewchancmd, 1, 600,
          "spewchan mask \tCommand to search channels by pattern");
  registercommand2(MODNAM, "subnetlist", dosubnetlist, 1, 0,
          "subnetlist X.Y.Z\tLists all users on a subnet");
  registercommand2(MODNAM, "countusers", docountusers, 1,0,
          "countusers [pattern]\tCounts how many Users on the net match pattern");
}

void spewcommands_cleanup() {
  deregistercommand("spew");
  deregistercommand("spewchan");
  deregistercommand("subnetlist");
  deregistercommand("countusers");
}
