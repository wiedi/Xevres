/*
Xevres (based on Operservice 2) - usercommands.c
Supplies all the standard commands that users can /msg the service
(C) Michael Meier 2000-2002 - released under GPL
(C) Sebastian Wiedenroth 2004
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
#include "dlmalloc/malloc.h"
#endif

void dowhois(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int um;
  int res; userdata **a; long i; unsigned int hash; channel **d; int iamtired;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if ((res!=2) || (strcmp(tmps3,"")==0)) {
    msgtouser(unum,"Syntax: whois nickname");
    return;
  }
  toLowerCase(tmps3);
  hash=nlhash(tmps3);
  a=(userdata **)nls[hash].content;
  for (i=0;i<nls[hash].cursi;i++) {
    if (strcmp(tmps3,a[i]->nick)==0) {
      sprintf(tmps2,"WHOIS info for %s:",tmps3);
      msgtouser(unum,tmps2);
      sprintf(tmps2,"Usermodes:        %s",a[i]->umode);
      msgtouser(unum,tmps2);
      sprintf(tmps2,"Ident@Host:       %s@%s",a[i]->ident,a[i]->host);
      msgtouser(unum,tmps2);
      longtotoken(a[i]->numeric,tmps3,5);
      sprintf(tmps2,"Numeric:          %s",tmps3);
      msgtouser(unum,tmps2);
      sprintf(tmps2,"Realname:         %s",a[i]->realname);
      msgtouser(unum,tmps2);
      longtoduration(tmps3,getnettime()-a[i]->connectat);
      sprintf(tmps2,"Connected for:    %s",tmps3);
      msgtouser(unum,tmps2);
      sprintf(tmps2,"Authlevel:        %d",getauthlevel(a[i]->numeric));
      msgtouser(unum,tmps2);
      if(a[i]->authname[0]!='\0') {
        sprintf(tmps2,"Network auth:     %s",a[i]->authname);
        msgtouser(unum,tmps2);
      } else {
        msgtouser(unum,"Not authed with the network.");
      }
      if (getauthlevel(a[i]->numeric)>0) {
        getauthedas(tmps3,a[i]->numeric);
        sprintf(tmps2,"Local auth:       %s",tmps3);
        msgtouser(unum,tmps2);
      } else {
        msgtouser(unum,"Not authed with me.");
      }
      if (a[i]->chans.cursi>0) {
        if (a[i]->chans.cursi==1) {
          strcpy(tmps2,"On channel:");
        } else {
          strcpy(tmps2,"On channels:");
        }
        for (iamtired=0;iamtired<(a[i]->chans.cursi);iamtired++) {
          d=(channel **)(a[i]->chans.content);
          if ((strlen(d[iamtired]->name)+strlen(tmps2))>400) {
            strcat(tmps2," [...]"); break;
          } else {
            um=getchanmode2(d[iamtired],a[i]->numeric);
            strcat(tmps2," (");
            if (um<0) {
              strcat(tmps2,"??");
            } else {
              if (isflagset(um,um_o)) { strcat(tmps2,"@"); } else { strcat(tmps2,"_"); }
              if (isflagset(um,um_v)) { strcat(tmps2,"+"); } else { strcat(tmps2,"_"); }
            }
            strcat(tmps2,")");
            strcat(tmps2,d[iamtired]->name);
          }
        }
        msgtouser(unum,tmps2);
      } else {
        msgtouser(unum,"Not on any (nonlocal-)channels");
      }
      msgtouser(unum,"--- End of WHOIS ---");
      return;
    }
  }
  sprintf(tmps2,"User %s does not exist.",tmps3);
  msgtouser(unum,tmps2);
}

void dostatus(long unum, char *tail) {
  int usersonnet; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int authl;
  sprintf(tmps2,"Xevres version %s - compiled on %s %s",
    operservversion, __DATE__, __TIME__);
  msgtouser(unum,tmps2);
  usersonnet=countusersonthenet();
  sprintf(tmps2,"There are currently %d users on the network (%lu fakeuser%s).",usersonnet,myfakes.cursi,
    (myfakes.cursi==1) ? "" : "s");
  msgtouser(unum,tmps2);
  sprintf(tmps2,"Current network time: %lu",(unsigned long)getnettime());
  msgtouser(unum,tmps2);
  longtoduration(tmps3,(time(NULL)-starttime));
  sprintf(tmps2,"I am running for %s",tmps3);
  msgtouser(unum,tmps2);
  authl=getauthlevel(unum);
  if (authl>=999) {
    int i, j; long ss[8];
    long longestch, curch;
    channel *chan; userdata *ud;
#ifdef HAVE_MALLINFO
    struct mallinfo mi;
#endif
    j=0; longestch=0;
    for (i=0;i<SIZEOFCL;i++) {
      if (cls[i]!=NULL) {
        j++; curch=0; chan=cls[i];
        while (chan!=NULL) { chan=(void *)chan->next; curch++; }
        if (curch>longestch) { longestch=curch; }
      }
    }
    sprintf(tmps2,"Channel-Hashtable-Usage:  %d / %d (%5.1f%%) - Maxchain %ld",j,SIZEOFCL,j*100.0/SIZEOFCL,longestch);
    msgtouser(unum,tmps2);
    j=0; longestch=0;
    for (i=0;i<SIZEOFUL;i++) {
      if (uls[i]!=NULL) {
        j++; curch=0; ud=uls[i];
        while (ud!=NULL) { ud=(void *)ud->next; curch++; }
        if (curch>longestch) { longestch=curch; }
      }
    }
    sprintf(tmps2,"Userlist-Hashtable-Usage: %d / %d (%5.1f%%) - Maxchain %ld",j,SIZEOFUL,j*100.0/SIZEOFUL,longestch);
    msgtouser(unum,tmps2);
    j=0; longestch=0;
    for (i=0;i<SIZEOFNL;i++) {
      if (nls[i].cursi>0) {
        j++;
        if (nls[i].cursi>longestch) { longestch=nls[i].cursi; }
      }
    }
    sprintf(tmps2,"Nicklist-Hashtable-Usage: %d / %d (%5.1f%%) - Maxchain %ld",j,SIZEOFNL,j*100.0/SIZEOFNL,longestch);
    msgtouser(unum,tmps2);
    stringtoolstats(ss);
    sprintf(tmps2,"Smallstrings-HT-Usage:    %ld / %ld (%5.1f%%) - Maxchain %ld",ss[1],ss[0],ss[1]*100.0/ss[0],ss[5]);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"  '- Array Size: %ld , average array usage: %5.1f%% - Small = < %ld chars",
      ss[3], ss[4]/10.0, ss[7]);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"Bigstrings-HT-Usage:      %ld / %ld (%5.1f%%) - Maxchain %ld",ss[2],ss[0],ss[2]*100.0/ss[0],ss[6]);
    msgtouser(unum,tmps2);
#ifdef HAVE_MALLINFO
    mi=mallinfo();
    sprintf(tmps2,"malloc(): %d used, %d unused, %5.1f%% waste",mi.uordblks,mi.fordblks,((100.0*mi.fordblks)/(mi.fordblks+mi.uordblks)));
    msgtouser(unum,tmps2);
#else
    msgtouser(unum,"malloc(): compiled without mallinfo(), no information available :(");
#ifdef WE_R_ON_BSD
    msgtouser(unum,"(BSDs malloc routines suck real bad, that's why they don't want to give stats about them out)");
#endif
#endif
  }
}

void doopchan(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  long tmpl;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
  if (res!=3) {
    sprintf(tmps2,"Syntax: opchan channel nickname");
    msgtouser(unum,tmps2);
    return;
  }
  toLowerCase(tmps4); toLowerCase(tmps3);
  tmpl=nicktonum(tmps4);
  if (tmpl<0) {
    sprintf(tmps2,"User %s is not on the network.",tmps4);
    msgtouser(unum,tmps2);
    return;
  }
  longtotoken(tmpl,tmps2,5);
  sendtouplink("%s M %s +o %s\r\n",servernumeric,tmps3,tmps2);
  sprintf(tmps2,"Put fake mode +o %s on %s",tmps4,tmps3);
  changechanmode(tmps3,tmpl,1,um_o);
  msgtouser(unum,tmps2);
  fflush(sockout);
}

void dokickcmd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], reas[TMPSSIZE];
  long tmpl;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4,reas);
  if (res<3) {
    msgtouser(unum,"Syntax: xkick channel nickname [reason]");
    msgtouser(unum,"If you don't give a reason the default will be \"requested by (yournick)\"");
    return;
  }
  if (res==3) {
    numtonick(unum,tmps2);
    sprintf(reas,"requested by %s",tmps2);
  }
  toLowerCase(tmps4); toLowerCase(tmps3);
  tmpl=nicktonum(tmps4);
  if (tmpl<0) {
    sprintf(tmps2,"User %s is not on the network.",tmps4);
    msgtouser(unum,tmps2);
    return;
  }
  longtotoken(tmpl,tmps2,5);
  sendtouplink("%s K %s %s :%s\r\n",servernumeric,tmps3,tmps2,reas);
  sprintf(tmps2,"Put fake kick for %s on %s (Reason: %s)",tmps4,tmps3,reas);
  deluserfromchan(tmps3,tmpl);
  msgtouser(unum,tmps2);
  fflush(sockout);
}

void dojupe(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE]; long tmpl;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %ld %s",tmps2,tmps3,&tmpl,tmps4);
  if (res!=3) {
    sprintf(tmps2,"Syntax: jupe servername numeric");
    msgtouser(unum,tmps2);
    return;
  }
  if (numericinuse(tmpl)) {
    if (getsrvnum(tmps3)!=tmpl) {
      sprintf(tmps2,"Sorry, the numeric you supplied is already in use on the network.");
      msgtouser(unum,tmps2);
      return;
    }
  }
  if (getsrvnum(tmps3)>=0) {
    sendtouplink("%s SQ %s 0 :Getting juped\r\n",servernumeric,tmps3);
    delsrv(tmps3);
  }
  longtotoken(tmpl,tmps2,2);
  sendtouplink("%s S %s 2 0 %ld J10 %sAAB :Jupe - /SQUIT to remove\r\n",servernumeric,tmps3,getnettime()+84000,tmps2);
  sendtouplink("%s EB\r\n",tmps2);
  sendtouplink("%s EA\r\n",tmps2);
  newserver(tmps3,1,tmpl,0,getnettime(),tokentolong(servernumeric),1);
  sprintf(tmps4,"%s has been juped.",tmps3);
  msgtouser(unum,tmps4);
  fflush(sockout);
}

void dooperhelp(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res>1) {
    showhelp(unum,tmps3);
    return;
  }
  msgtouser(unum,"Additional commands for opers:");
  msgtouser(unum,"==============================");
  msgtouser(unum,"login username password   Login as Oper");
  msgtouser(unum,"create [desired username] Creates a new Oper useraccount (with zero rights)");
  msgtouser(unum,"password newpass newpass  Changes your password");
  msgtouser(unum,"noticeme on|off           Sets if the service sends you its replys as /msg or as /notice");
  msgtouser(unum,"status                    Prints some status information");
  msgtouser(unum,"opchan channel nickname   Ops nickname on channel");
  msgtouser(unum,"xkick chan nick [reason]  Kicks nickname from channel");
  msgtouser(unum,"whois nickname            Shows some info about this user");
  msgtouser(unum,"channel channelname       Shows info about that channel");
  msgtouser(unum,"compare chan1 chan2       Compare channels and show common users");
  msgtouser(unum,"compare nick1 nick2       Compare users and show common channels");
  msgtouser(unum,"resync channelname        Resyncs a desynched channel");
  msgtouser(unum,"semiresync channelname    Resyncs a desynched channel");
  msgtouser(unum,"deopall channelname       Deops everyone on a channel (including Q!)");
  msgtouser(unum,"regexgline RE duration    Sets a regexp gline");
  msgtouser(unum,"                          type [reason]");
  msgtouser(unum,"mfc [timespan]            Mirkforce-Check");
  msgtouser(unum,"serverlist                Lists servers");
  msgtouser(unum,"password [user] newp newp Changes password for you or another user");
  msgtouser(unum,"changelev user newlevel   Changes the authlevel of a user");
  msgtouser(unum,"deluser username          Deletes a user");
  msgtouser(unum,"listauthed                Lists who is authed right now");
  msgtouser(unum,"regexspew RE              Lists all clients matching a regexp");
  msgtouser(unum,"fakeuser nick ident       Creates a fake user (useful to block nicks)");
  msgtouser(unum,"                          host realname");
  msgtouser(unum,"fakelist                  Lists all Fakeusers");
  msgtouser(unum,"fakekill nick             Removes a fakeuser");
  msgtouser(unum,"rnc                       Lists the 20 most common realnames on the net");
  msgtouser(unum,"rngline pattern duration  Adds a GLINE on a Realname");
  msgtouser(unum,"                          bantype reason");
  msgtouser(unum,"rnungline pattern         Removes a GLINE on a Realname");
  msgtouser(unum,"rnglist                   Lists all Realname-GLINEs");
  msgtouser(unum,"rnglogsearch              Search log of activated Realname-GLINEs");
  msgtouser(unum,"rnglogpurge               Purge log of activated Realname-GLINEs");
  msgtouser(unum,"splitlist                 Gives a list of splits");
  msgtouser(unum,"splitdel pattern          Removes one or more entries from the splitlist");
  msgtouser(unum,"loadmod module            Loads a module");
  msgtouser(unum,"unloadmod module          Unloads a module");
  msgtouser(unum,"reloadmod module          Same as unloadmod followed by loadmod");
  msgtouser(unum,"lsmod                     Lists loaded modules");
  msgtouser(unum,"rehash                    Rehashes Config file");
  msgtouser(unum,"noticemask [newmask]      Shows/Sets your noticemask");
  msgtouser(unum,"die [message]             Kills the service. Does NOT automatically save before that!");
  msgtouser(unum,"chanfix channelname       Fixes a channel");
  msgtouser(unum,"showregs channelname      Shows Nicks elegible for reop");
  msgtouser(unum,"chanopstat channelname    Compares stats for current and regular ops");
  msgtouser(unum,"chanoplist chan [-all]    Lists scores");
  dyncmdhelp(unum,1,getauthlevel(unum));
}

void douserhelp(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res>1) {
    sprintf(tmps4,"user%s",tmps3);
    showhelp(unum,tmps4);
    return;
  }
  msgtouser(unum,"Xevres commands: ");
  msgtouser(unum,"================");
  msgtouser(unum,"help                      Help for a specific command"); 

  dyncmdhelp(unum,0,getauthlevel(unum));
}

void dousercmp(long unum, char *tail) {
  int c, i, j;
  int match = 0, flags1 = 0, flags2 = 0, hits = 0, nochannels = 0;
  unsigned int hash; long lnumeric1, lnumeric2; userdata **user; channel **chan;
  chanuser *chan_user; int havedisplayed=0;
  char nick1[TMPSSIZE], nick2[TMPSSIZE],
       hostmask1[TMPSSIZE], hostmask2[TMPSSIZE], f1[TMPSSIZE], f2[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  if ((c = sscanf(tail, "%*s %s %[^\n]", nick1, nick2)) < 2) {
    msgtouser(unum, "Syntax: compare nick1 nick2");
    return;
  }
  toLowerCase(nick1);
  if ((lnumeric1 = nicktonum(nick1)) < 0) {    /* check nick1 */
    newmsgtouser(unum, "User %s is not on the network.", nick1);
    return;
  }
  numtohostmask(lnumeric1, hostmask1);         /* get nick1's hostmask */
  toLowerCase(nick2);
  if ((lnumeric2 = nicktonum(nick2)) < 0) {    /* check nick2 */
    newmsgtouser(unum, "User %s is not on the network.", nick2);
    return;
  }
  numtohostmask(lnumeric2, hostmask2);         /* get nick2's hostmask */
  newmsgtouser(unum, "Comparing user <%s> with user <%s>", hostmask1, hostmask2);
  msgtouser(unum, "Common channels for both users:");
  hash = nlhash(nick1);
  user = (userdata **) nls[hash].content;
  for (i = 0; i < nls[hash].cursi; i++) {      /* run thru the list */
    if (!strcmp(nick1, user[i]->nick)) {       /* look for nick1 */
      if (user[i]->chans.cursi) {              /* found him/her, any channels? */
        chan = (channel **) user[i]->chans.content;
        for (j = 0; j < user[i]->chans.cursi; j++) {   /* walk the channel list */
          chan_user = (chanuser *) chan[j]->users.content;
          /* walk the userlist on the channel */
          for (c = 0, match = flags1 = flags2 = 0; c < chan[j]->users.cursi; c++) {
             if (lnumeric1 == chan_user[c].numeric) {  /* grab nick1's chanmodes */
               flags1 = chan_user[c].flags;
             }
             if (lnumeric2 == chan_user[c].numeric) {  /* is nick2 present on it? */
               match++; hits++;
               flags2 = chan_user[c].flags;
             }
          }
          if (match) {
            if (havedisplayed>=100) {
              newmsgtouser(unum,"More than %d matches, list truncated.",havedisplayed);
              newmsgtouser(unum, "--- End of list - %d match%s ---", hits, (1 == hits) ? "" : "es");
              return;
            }
            flags2string(flags1, f1);
            flags2string(flags2, f2);
            newmsgtouser(unum, "  %-25s (%s) %-15s (%s) %-15s", chan[j]->name, 
                    f1, nick1, f2, nick2);
            havedisplayed++;
          }
        }
      } else {
        nochannels++;
        newmsgtouser(unum, "  \"%s\" is not on any (nonlocal-)channels", nick1);
      }
      break; /* found nick1, no need to continue to walk the userlist */
    }
  }
  if (!hits && !nochannels) {
    newmsgtouser(unum, "  \"%s\" and \"%s\" do not share any channels", nick1, nick2);
  }
  newmsgtouser(unum, "--- End of list - %d match%s ---", hits, (1 == hits) ? "" : "es");
}

void dochancmp(long unum, char *tail) {
  int c, i, j, hits = 0;
  channel *one, *two;
  chanuser *u_one, *u_two;
  char channel1[TMPSSIZE], channel2[TMPSSIZE],
       hostmask[TMPSSIZE], nick[TMPSSIZE], f1[TMPSSIZE], f2[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  if ((c = sscanf(tail, "%*s %s %[^\n]", channel1, channel2)) < 2) {
    msgtouser(unum, "Syntax: compare #channel1 #channel2");
    msgtouser(unum, "     or compare nick1 nick2");
    return;
  }
  if ('#' != channel1[0]) {
    dousercmp(unum, tail);
    return;
  }
  toLowerCase(channel1);
  if (NULL == (one = getchanptr(channel1))) {
    newmsgtouser(unum, "The channel \"%s\" doesn't exists.", channel1);
    return;
  }
  toLowerCase(channel2);
  if (NULL == (two = getchanptr(channel2))) {
    newmsgtouser(unum, "The channel \"%s\" doesn't exists.", channel2);
    return;
  }
  newmsgtouser(unum, "Comparing \"%s\" with \"%s\"", channel1, channel2);
  msgtouser(unum, "Common users on both channels:");
  u_one = (chanuser *) one->users.content;
  u_two = (chanuser *) two->users.content;
  for (i = 0; i < one->users.cursi; i++) {
    for (j = 0; j < two->users.cursi; j++) {
      if (u_one[i].numeric == u_two[j].numeric) {
        hits++;
        numtonick(u_one[i].numeric, nick);
        numtohostmask(u_one[i].numeric, hostmask);
        flags2string(u_one[i].flags, f1);
        flags2string(u_two[j].flags, f2);
        newmsgtouser(unum, "  %-15s (%s) (%s) <%s>", nick, f1, f2, hostmask);
      }
    }
  }
  newmsgtouser(unum, "--- End of list - %d match%s ---", hits, (1 == hits) ? "" : "es");
}

void dochancmd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  channel *a; chanuser *b; int ops; int voices;
  unsigned long j; int k, l;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: channel channelname");
    return;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  sprintf(tmps2,"Information about %s",tmps3);
  msgtouser(unum,tmps2);
  strcpy(tmps2,""); ops=0;
  for (res=0;res<numcf;res++) {
    if (isflagset(a->flags,chanflags[res].v)) {
      tmps2[ops]=chanflags[res].c;
      ops++;
    }
  }
  tmps2[ops]='\0';
  sprintf(tmps4,"Chanmodes: %s",tmps2);
  msgtouser(unum,tmps4);
  if (isflagset(a->flags,cm_l)) {
    sprintf(tmps4,"Channellimit: %d",a->chanlim);
    msgtouser(unum,tmps4);
  }
  if (isflagset(a->flags,cm_k)) {
    sprintf(tmps4,"Channelkey: %s",a->chankey);
    msgtouser(unum,tmps4);
  }
  if (isflagset(a->flags,cm_F)) {
    sprintf(tmps4,"Forwardchan: %s",a->fwchan);
    msgtouser(unum,tmps4);
  }
  if (strcmp(a->topic,"")==0) {
    msgtouser(unum,"No topic set");
  } else {
    sprintf(tmps2,"Topic: %s",a->topic);
    msgtouser(unum,tmps2);
  }
  msgtouser(unum,"Users on the channel:");
  b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; ops=0; voices=0;
  for (j=0;j<a->users.cursi;j++) {
    if (strlen(tmps2)>80) {
      msgtouser(unum,tmps2);
      strcpy(tmps2,"");
    }
    numtonick(b[j].numeric,tmps4);
    l=strlen(tmps4);
    if (l>0) {
      for (k=l;k<NICKLEN;k++) {
        tmps4[k]=' ';
      }
      tmps4[NICKLEN]='\0';
    }
    if (tmps2[0]!='\0') { strcat(tmps2," "); }
    strcat(tmps2,"(");
    if (isflagset(b[j].flags,um_o)) {
      ops++;
      strcat(tmps2,"@");
    } else {
      strcat(tmps2,"_");
    }
    if (isflagset(b[j].flags,um_v)) {
      voices++;
      strcat(tmps2,"+");
    } else {
      strcat(tmps2,"_");
    }
    strcat(tmps2,")");
    if (tmps4[0]=='\0') {
      longtotoken(b[j].numeric,tmps4,5);
      strcat(tmps2,"!! DEBUG: Empty nick. Numeric: ");
      strcat(tmps2,tmps4);
      strcat(tmps2," !!");
    } else {
      strcat(tmps2,tmps4);
    }
    res++;
  }
  msgtouser(unum,tmps2);
  sprintf(tmps2,"%d Users; %d Op, %d Voice",res,ops,voices);
  msgtouser(unum,tmps2);
}

int resynchelper(int fullresync, char * chan) {
  channel *a; chanuser *b; unsigned long j; int res;
  char tmps2[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  int doreop; userdata *ud;
  a=getchanptr(chan);
  if (a==NULL) { return -1; }
  if (fullresync==1) {
    sendtouplink("%s CM %s o\r\n",servernumeric,chan);
  }
  b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; strcpy(tmps4,"");
  for (j=0;j<a->users.cursi;j++) {
    if (res>=6) {
      sendtouplink("%s M %s %s %s\r\n",servernumeric,chan,tmps2,tmps4);
      res=0; strcpy(tmps2,""); strcpy(tmps4,"");
    }
    doreop=0;
    if (isflagset(b[j].flags,um_o)) {
    	doreop=1;
    } else {
    	ud=getudptr(b[j].numeric);
      if (ud!=NULL) {
        if (ischarinstr('k',ud->umode)) { /* channelservice, but deopped? FIX! */
          doreop=1;
        }
      }
    }
    if (doreop==1) {
      strcat(tmps2,"+o");
      longtotoken(b[j].numeric,tmps5,5);
      if (res>0) { strcat(tmps4," "); }
      strcat(tmps4,tmps5);
      res++;
    }
  }
  if (res>0) {
    sendtouplink("%s M %s %s %s\r\n",servernumeric,chan,tmps2,tmps4);
  }
  return 0;
}

void doresync(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,500)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: resync channelname");
    return;
  }
  toLowerCase(tmps3);
  if (resynchelper(1,tmps3)<0) {
  	msgtouser(unum,"Failed to resync the channel - does it even exist?");
  } else {
    msgtouser(unum,"Channel should be resynced.");
  }
  fflush(sockout);
}

void dosemiresync(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,500)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: semiresync channelname");
    return;
  }
  toLowerCase(tmps3);
  if (resynchelper(0,tmps3)<0) {
  	msgtouser(unum,"Failed to resync the channel - does it even exist?");
  } else {
    msgtouser(unum,"Channel should be semiresynced.");
  }
  fflush(sockout);
}

void dodeopall(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  channel *a;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: deopall channelname");
    return;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  deopall(a);
  msgtouser(unum,"deopall should be done.");
}

void domfc(long unum, char *tail) {
  msgtouser(unum,"Defunct.");
}

void dosettimecmd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,500)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if ((res==2) && (strcmp(tmps3,"force")==0)) {
    msgtouser(unum,"Sending settime (forced)");
    timestdif=0;  /* _WE_ are authoritative with respect to time. */
    sendtouplink("%s SE %ld\r\n",servernumeric,(long int)getnettime());
    fflush(sockout);
  } else {
    if (waitingforping==0) {
      msgtouser(unum,"Sending pings to all servers. If the networklag is below a certain value, i will then do SETTIME.");
      msgtouser(unum,"If you want to force a settime even if the net is lagged, use 'settime force'.");
      pingallservers();
      waitingforping=getnettime(); lastsettime=getnettime()-3600;
    } else {
      msgtouser(unum,"There is already a settime-request pending.");
    }
  }
}

int serverlistsorthelper1(const void *a, const void *b) {
  // numeric sort
  unsigned long *c, *d;
  c=(unsigned long *)a;
  d=(unsigned long *)b;
  return (*c - *d);
}

int serverlistsorthelper2(const void *a, const void *b) {
  // reverse-hostname sort
  unsigned long *c, *d; serverdata *s1, *s2;
  char nick1[SRVLEN+1], nick2[SRVLEN+1];
  c=(unsigned long *)a;
  d=(unsigned long *)b;
  s1=getsdptr(*c);
  s2=getsdptr(*d);
  if ((s1==NULL) || (s2==NULL)) { return 0; }
  // compare s1->nick , s2->nick
  mystrncpy(nick1,s1->nick,SRVLEN);
  strreverse(nick1);
  mystrncpy(nick2,s2->nick,SRVLEN);
  strreverse(nick2);
  return (strcmp(nick1,nick2));
}

void doserverlist(long unum, char *tail) {
  serverdata *s; int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], sort[TMPSSIZE], pattern[TMPSSIZE]; int sc;
  long usersonthissrv; long totalusers=0;  int sortswitch = 0;
  array listofids; long i; long j;   unsigned long showid;
  int sortmode = 0;  // Default Sortmode (0)unsorted (2)numeric (3)host

  res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
  if (res>3) {
    msgtouser(unum,"Syntax: serverlist [sort] [pattern]");    
    msgtouser(unum,"  Sort: -n numeric -h host -u unsorted (default)");
    return;
  }
  if (res<2) { tmps3[0]='\0'; }
  if (tmps3[0]=='-') {
    // user wants special sort order
    res=sscanf(tail,"%s %s %s",tmps2,sort,pattern);
    sortswitch = 1;
  } else {
    res=sscanf(tail,"%s %s",tmps2,pattern);      
    sortswitch = 0;
  }
  if ((res==2) && (sortswitch==1)) { /* Sortswitch but no pattern */
    strcpy(pattern,"*");
  }
  if (res==1) { strcpy(pattern,"*"); }
  toLowerCase(sort);
  toLowerCase(pattern);
  // Check what sort mode..
  if (sortswitch > 0) {
    sortmode = 1;
    if (strcmp("-u", sort) == 0) { sortmode = 0; }
    if (strcmp("-n", sort) == 0) { sortmode = 2; }
    if (strcmp("-h", sort) == 0) { sortmode = 3; }
  }
  // Error Check, wrong sort mode selected
  if (sortmode==1) {
    msgtouser(unum,"Syntax: serverlist [sort] [pattern]");
    msgtouser(unum,"  Sort: -n numeric -h host -u unsorted (default)");
    return;
  }
  // Build Sort Array
  array_init(&listofids,sizeof(unsigned long));
  s=sls;
  while (s!=NULL) {
    strcpy(tmps4,s->nick); toLowerCase(tmps4);
    if (match2strings(pattern,tmps4)) {
      j=array_getfreeslot(&listofids);
      ((unsigned long *)listofids.content)[j]=s->numeric;
    }
    s=(void *)s->next;
  }
  // No Matches found
  if (listofids.cursi==0) {
    msgtouser(unum,"No matches.");
    array_free(&listofids);
    return;
  }
  // Sort..
  if (sortmode == 2) { qsort(listofids.content, listofids.cursi, sizeof(unsigned long), serverlistsorthelper1); }
  if (sortmode == 3) { qsort(listofids.content, listofids.cursi, sizeof(unsigned long), serverlistsorthelper2); }
  // Print results..
  sprintf(tmps2,"%3s %-32s %-18s %-5s %-18s %4s %5s %s","Num","Hostname","Uptime","MaxCl","Connected for","Ping","Clie","Ports");
  msgtouser(unum,tmps2);
  sc=0;
  for (i=0;i<listofids.cursi;i++) {
    sc++;
    showid=((unsigned long *)(listofids.content))[i];
    // Display server..
    s=getsdptr(showid);
    if (s==NULL) { continue; }
    longtoduration(tmps4,getnettime()-s->connectat);
    longtoduration(tmps3,getnettime()-s->createdat);
    if ((getnettime()-s->createdat)>durationtolong("3y")) {
      strcpy(tmps3,"N/A");
    }
    usersonthissrv=countusersonsrv(s->numeric);
    totalusers+=usersonthissrv;
    if ((s->pingreply-s->pingsent)<0) {
      sprintf(tmps2,"%3ld %-32s %18s %5ld %18s  N/A %5ld %s",s->numeric,s->nick,tmps3,s->maxclients,tmps4,usersonthissrv,s->ports);
    } else {
      sprintf(tmps2,"%3ld %-32s %18s %5ld %18s %4ld %5ld %s",s->numeric,s->nick,tmps3,s->maxclients,tmps4,s->pingreply-s->pingsent,usersonthissrv,s->ports);
    }
    msgtouser(unum,tmps2);
  }
  array_free(&listofids);
  if (match2strings(pattern,myname)) {
    longtoduration(tmps3,getnettime()-starttime);
    usersonthissrv=countusersonsrv(tokentolong(servernumeric));
    totalusers+=usersonthissrv;
    sprintf(tmps2,"%3ld %-32s %18s %5d %18s %4d %5ld %s",
      tokentolong(servernumeric),myname,tmps3,4095,tmps3,0,usersonthissrv,"none");
    msgtouser(unum,tmps2);
    sc++;
  }
  sprintf(tmps2,"--- End of list. %ld users and %d servers on the net.",totalusers,sc);
  msgtouser(unum,tmps2);
}

void dohello(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  authdata ad;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) { numtonick(unum,tmps3); }
  tmps3[32]='\0';
  getuserdata(&ad,tmps3);
  if (ad.authlevel<0) {
    msgtouser(unum,"Error accessing the database. Try again later.");
    return;
  }
  if (ad.authlevel>0) {
    sprintf(tmps2,"Sorry, Username %s is already in use.",tmps3);
    msgtouser(unum,tmps2);
    msgtouser(unum,"You'll have to use another Username. Try 'hello otherusername'");
    return;
  }
  // That user does not exist yet, so lets create it!
  strcpy(ad.username,tmps3);
  createrandompw(ad.password,8);
  sprintf(tmps2,"A random password has been created for you: %s",ad.password);
  msgtouser(unum,tmps2);
  msgtouser(unum,"Please change this password as soon as possible.");
  if (countusersinul()==0) {
    ad.authlevel=1000;  // This is the first user on the service, he gets assigned level 1000
    msgtouser(unum,"You are the first user on the service. You have been assigned an authlevel of 1000.");
    strcpy(tmps2,ad.password);      // We need to encrypt this users PW
    encryptpwd(ad.password,tmps2);  // because he has authlevel >=100 !
  } else {
    ad.authlevel=1;
  }
  ad.lastauthed=getnettime();
  ad.wantsnotice=0;
  if (isircop(unum)) { ad.noticemask=NOTICEMASKDEFAULT; } else { ad.noticemask=0; }
  res=updateuserinul(ad);
  if (res!=1) {
    sprintf(tmps2,"Oooops... Database-Query failed. try again later.");
    msgtouser(unum,tmps2);
  } else {
    sprintf(tmps2,"You have been successfully added with username %s",ad.username);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"You can now auth: /msg %s login %s yourpassword",mynick,ad.username);
    msgtouser(unum,tmps2);
  }
}

void doauth(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE]; authdata ad;
  char lohlp[TMPSSIZE];
  res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
  if (res!=3) {
    msgtouser(unum,"Syntax: login username password");
    msgtouser(unum,"Note that password is case sensitive, username isn't.");
    return;
  }
  numtohostmask(unum,lohlp);
  tmps3[32]='\0'; tmps4[32]='\0';
  getuserdata(&ad,tmps3);
  if (ad.authlevel<0) {
    msgtouser(unum,"Sorry, Query to database failed. Please try again later.");
    putlog("%s !AUTH %s! - failed (DB)",lohlp,tmps3);
    return;
  }
  if (ad.authlevel==0) {
    msgtouser(unum,"That user does not exist.");
    putlog("%s !AUTH %s! - failed (no such user)",lohlp,tmps3);
    return;
  }
  if (ad.authlevel>=100) {
    if (checkpwd(ad.password,tmps4)!=1) {
      msgtouser(unum,"Password incorrect.");
      putlog("%s !AUTH %s! - failed (wrong PW)",lohlp,tmps3);
      return;
    }
  } else {
    if (strcmp(ad.password,tmps4)!=0) {
      msgtouser(unum,"Password incorrect.");
      putlog("%s !AUTH %s! - failed (wrong PW)",lohlp,tmps3);
      return;
    }
  }
  addautheduser(unum,ad);
  ad.lastauthed=getnettime();
  updateuserinul(ad);
  autheduser *au;
  au=getalentry(unum);
  newmsgtouser(unum,"Login successful.");
  putlog("%s !AUTH! %s - succeeded",lohlp,tmps3);
  if (ad.authlevel>=100) {
    newmsgtouser(unum,"Your authlevel is %d",ad.authlevel);
  }
}

void dochangepwd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  authdata ad; char lohlp[TMPSSIZE];
  res=sscanf(tail,"%s %s %s %s %s",tmps2,tmps3,tmps4,tmps5,tmps6);
  if ((res!=3) && (res!=4)) {
    msgtouser(unum,"Syntax: password [user] newpass newpass");
    return;
  }
  tmps3[32]='\0'; tmps4[32]='\0'; tmps5[32]='\0'; //Truncate everything to 32 chars
  numtohostmask(unum,lohlp);
  if (res==4) {
    if (!isircop(unum)) {
      msgtouser(unum,"Only ircops can change the passwords of other users");
      putlog("%s !PASSWORD %s! - failed (no IRCOP)",lohlp,tmps3);
      return;
    }
    getuserdata(&ad,tmps3);
    if (ad.authlevel>=getauthlevel(unum)) {
      msgtouser(unum,"You cannot change the password of a user that has a higher or equal authlevel");
      putlog("%s !PASSWORD %s! - failed (authlevel too low)",lohlp,tmps3);
      return;
    }
    if (strcmp(tmps4,tmps5)!=0) {
      msgtouser(unum,"Passwords do not match");
      return;
    }
    strcpy(tmps6,tmps3);
    strcpy(tmps2,tmps4);
  } else {
    if (strcmp(tmps3,tmps4)!=0) {
      msgtouser(unum,"Passwords do not match");
      return;
    }
    getauthedas(tmps6,unum);
    strcpy(tmps2,tmps4);
  }
  getuserdata(&ad,tmps6);
  if (ad.authlevel==0) {
    msgtouser(unum,"That user does not exist.");
    putlog("%s !PASSWORD %s! - failed (nosuchuser)",lohlp,tmps6);
    return;
  }
  if (ad.authlevel<0) {
    msgtouser(unum,"There was an error accessing the databse.");
    putlog("%s !PASSWORD %s! - failed (DB-Err)",lohlp,tmps6);
    return;
  }
  if (ad.authlevel>=100) {
    strcpy(tmps5,tmps2);
    encryptpwd(tmps2,tmps5);
  }
  strcpy(ad.password,tmps2);
  if (updateuserinul(ad)) {
    msgtouser(unum,"Password changed.");
    putlog("%s !PASSWORD %s! - succeeded",lohlp,tmps6);
  } else {
    msgtouser(unum,"Something went wrong when accessing the database. Password could not be changed.");
    putlog("%s !PASSWORD %s! - failed (DB-Err #2)",lohlp,tmps6);
  }
}

void dochangelev(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE]; int newlev;
  authdata ad;
  res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
  if (res!=3) {
    msgtouser(unum,"Syntax: changelev user newlevel");
    return;
  }
  newlev=atoi(tmps4);
  getuserdata(&ad,tmps3);
  if (ad.authlevel<0) {
    msgtouser(unum,"Ooooops... Error accessing database.");
    return;
  }
  if (ad.authlevel==0) {
    msgtouser(unum,"That user does not exist.");
    return;
  }
  if (ad.authlevel>=getauthlevel(unum)) {
    msgtouser(unum,"You cannot change the authlevel of a user that has a higher or equal authlevel.");
    return;
  }
  if (newlev>=getauthlevel(unum)) {
    msgtouser(unum,"You cannot give anyone an authlevel that is higher or equal than your own!");
    return;
  }
  if ((newlev<100) && (ad.authlevel>=100)) {
    msgtouser(unum,"*MEEP* You cannot change the level of a user from 100+ to something <100");
    msgtouser(unum,"You'll have to delete that user and recreate it.");
    msgtouser(unum,"The reason for this is, that for users with level >= 100 only a password-hash is stored, not the password itself, and there is no way to get the password back.");
    return;
  }
  if ((ad.authlevel<100) && (newlev>=100)) {
    strcpy(tmps2,ad.password);
    encryptpwd(ad.password,tmps2);
  }
  ad.authlevel=newlev;
  if (updateuserinul(ad)) {
    msgtouser(unum,"Userlevel changed. Note that this change will not take effect until the user reauths.");
  } else {
    msgtouser(unum,"Couldn't change the entry in the database. Try again later.");
  }
}

void dodelusercmd(long unum, char *tail) {
  int res; authdata ad; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res!=2) {
    msgtouser(unum,"Syntax: deluser username");
    return;
  }
  getuserdata(&ad,tmps3);
  if (ad.authlevel==0) {
    msgtouser(unum,"That user does not exist");
    return;
  }
  if (ad.authlevel<0) {
    msgtouser(unum,"Couldn't read database - try again later.");
    return;
  }
  if (ad.authlevel>=getauthlevel(unum)) {
    msgtouser(unum,"You cannot delete an user with a higher or equal authlevel");
    return;
  }
  mysql_escape_string(tmps2,ad.username,strlen(ad.username));
  sprintf(tmps4,"DELETE FROM users WHERE username='%s'",tmps2);
  res=mysql_query(&sqlcon,tmps4);
  if (res==0) {
    msgtouser(unum,"User deleted.");
  } else {
    msgtouser(unum,"Query to database failed.");
  }
}

void donoticeset(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; authdata ad; autheduser *au;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: noticeme on|off");
    return;
  }
  if (getauthlevel(unum)<1) {
    msgtouser(unum,"This command only makes sense when you are authed");
    return;
  }
  toLowerCase(tmps3);
  if ((strcmp(tmps3,"on")!=0) && (strcmp(tmps3,"off")!=0)) {
    msgtouser(unum,"Syntax: noticeme on|off");
    return;
  }
  getauthedas(tmps2,unum);
  getuserdata(&ad,tmps2);
  if (ad.authlevel<1) {
    msgtouser(unum,"That user does not exist in the database or database is down.");
    return;
  }
  if (strcmp(tmps3,"on")==0) { ad.wantsnotice=1; }
  if (strcmp(tmps3,"off")==0) { ad.wantsnotice=0; }
  updateuserinul(ad);
  au=getalentry(unum);
  au->wantsnotice=ad.wantsnotice;
  msgtouser(unum,"OK.");
}

void dolistauthed(long unum, char *tail) {
  autheduser *a; char tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  a=als;
  sprintf(tmps4,"%-50s %-20s %-4s","Who","Authed as","Lev");
  msgtouser(unum,tmps4);
  while (a!=NULL) {
    numtohostmask(a->numeric,tmps3);
    sprintf(tmps4,"%-50s %-20s %4d",tmps3,a->authedas,a->authlevel);
    msgtouser(unum,tmps4);
    a=(void *)a->next;
  }
  msgtouser(unum,"End of authlist");
}

void dosave(long unum, char *tail) {
  if (!checkauthlevel(unum,900)) { return; }
  saveall();
  msgtouser(unum,"Done. Tried to save everything.");
}

int regexcountusershit(int type, char *re) {
  int i, c, total; array alreadyhadthose; int j;
  userdata *user; int isma;
  regex_t preg;
  char pattern[TMPSSIZE];
  if (regcomp(&preg, re, REG_EXTENDED|REG_NOSUB)) { return(99999); }
  total=0; c=0;
  array_init(&alreadyhadthose,sizeof(unsigned long));
  for (i = 0; i < SIZEOFUL; i++) {
    user = uls[i];
    while (user != NULL) {
      sprintf(pattern, "%s!%s@%s (%s)", user->nick, user->ident, user->host, user->realname);
      if (!regexec(&preg, pattern, (size_t) 0, NULL, 0)) {
      	isma=0;
      	for (j=0;j<alreadyhadthose.cursi;j++) {
      		if (((unsigned long *)alreadyhadthose.content)[j]==user->realip) {
      			isma=1; break;
      		}
      	}
      	if (isma==0) {
          total+=sncget(user->realip, 32);
          j=array_getfreeslot(&alreadyhadthose);
          ((unsigned long *)alreadyhadthose.content)[j]=user->realip;
        }
      }
      user = (void *)user->next;
    }
  }
  array_free(&alreadyhadthose);
  regfree(&preg);
  return(total);
}

void doregexgline(long unum, char *tail) {
  int i, c, dur, type, total; array alreadyhadthose; int j;
  userdata *user; int isma; regex_t preg;
  char re[TMPSSIZE], pattern[TMPSSIZE], hostmask[TMPSSIZE], buf[TMPSSIZE];
  char howlong[TMPSSIZE], who[TMPSSIZE], why[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  c = sscanf(tail, "%*s %s %s %s %[^\n]", re, howlong, pattern, why);
  if (c < 3) {
    numtonick(unum, who);
    msgtouser(unum, "Syntax: regexgline RE duration type [reason]");
    msgtouser(unum, "        RE       - a POSIX 1003.2 regular expression, eg ^[A-Za-z]{7}[0-9]{2}");
    msgtouser(unum, "        duration - Xs|M|h|w|m|y, eg 3h");
    msgtouser(unum, "        type     - 1 means glining user@host");
    msgtouser(unum, "                 - 2 means glining *@host");
    newmsgtouser(unum, "        reason   - reason (defaults to \"requested by %s\")", who);
    return;
  }
  if (3 == c) {	/* reason not supplied */
    numtonick(unum, buf);
    sprintf(why, "requested by %s", buf);
  }
  if ((dur = durationtolong(howlong)) <= 0) {
    msgtouser(unum, "The duration you gave is invalid.");
    return;
  }
  if (!strncmp(pattern, "1", 1)) {
    type = 1;
  } else if (!strncmp(pattern, "2", 1)) {
    type = 2;
  } else {
    msgtouser(unum, "invalid type. valid types are either 1 or 2.");
    return;
  }
  if (regcomp(&preg, re, REG_EXTENDED|REG_NOSUB)) {
    msgtouser(unum, "Invalid regexp");
    return;
  }
  if ((total = regexcountusershit(type, re)) > GLINEMAXHIT) {
    newmsgtouser(unum, "That gline would hit more than %d (%d) Users. You probably misstyped something.", GLINEMAXHIT, total);
    if ((total<(GLINEMAXHIT*10)) && (getauthlevel(unum)>=999)) {
      msgtouser(unum,"However, your authlevel is >=999, so I hope you know what you're doing... Executing command.");
    } else {
      regfree(&preg);
      return;
    }
  }
  if (total==0) { newmsgtouser(unum, "REGEXGLINE %s didn't match any users.", re); return; }
  numtonick(unum, who);
  c = 0;
  array_init(&alreadyhadthose,sizeof(unsigned long));
  for (i = 0; i < SIZEOFUL; i++) {
    user = uls[i];
    while (user != NULL) {
      sprintf(pattern, "%s!%s@%s (%s)", user->nick, user->ident, user->host, user->realname);
      if (!regexec(&preg, pattern, (size_t) 0, NULL, 0)) {
      	isma=0;
      	for (j=0;j<alreadyhadthose.cursi;j++) {
      		if (((unsigned long *)alreadyhadthose.content)[j]==user->realip) {
      			isma=1; break;
      		}
      	}
      	if (isma==0) {
        	char *mycip;
        	mycip=printipv4(user->realip);
          c++;
          if (1 == type) {
            sprintf(hostmask, "%s@%s", user->ident, mycip);
          } else {
            sprintf(hostmask, "*@%s", mycip);
          }
          free(mycip);
          addgline(hostmask, why, who, dur, 1);
          sprintf(buf, "GLINE %s, expires in %s, set by %s: %s", hostmask, howlong, who, why);
          sendtonoticemask(NM_GLINE, buf);
          newmsgtouser(unum, "Added %s", buf);
          j=array_getfreeslot(&alreadyhadthose);
          ((unsigned long *)alreadyhadthose.content)[j]=user->realip;
        }
      }
      user = (void *)user->next;
    }
  }
  array_free(&alreadyhadthose);
  if (c) {
    newmsgtouser(unum, "REGEXGLINE %s, expires in %s, set by %s: %s (produced %d glines/hit %d use%s)", re, howlong, who, why, c, total, (1 == total) ? "r" : "rs" );
  } else {
    newmsgtouser(unum, "REGEXGLINE %s didn't match any users.", re);
  }
  regfree(&preg);
  purgeglines();
}

void doregexspew(long unum, char *tail) {
  int i, j, c, chanmodes;
  userdata *user;
  channel **chan;
  regex_t preg;
  char re[TMPSSIZE], hostmask[TMPSSIZE], buf[TMPSSIZE];
  if (!checkauthlevel(unum,600)) { return; }
  c = sscanf(tail, "%*s %[^\n]", re);
  if (c != 1) {
    msgtouser(unum, "Syntax: regexspew RE");
    msgtouser(unum, "        RE - a POSIX 1003.2 regular expression, eg ^[A-Za-z]{7}[0-9]{2}");
    return;
  }
  if (regcomp(&preg, re, REG_EXTENDED|REG_NOSUB)) {
    msgtouser(unum, "Invalid regexp"); return;
  }
  sprintf(buf, "Users matching RE %s:", re);
  msgtouser(unum, buf);
  c = 0;
  for (i = 0; (i < SIZEOFUL && c <= 500); i++) {
    user = uls[i];
    while (user != NULL && c <= 500) {
      sprintf(hostmask, "%s!%s@%s (%s)", user->nick, user->ident, user->host, user->realname);
      if (!regexec(&preg, hostmask, (size_t) 0, NULL, 0)) {
        c++;
        msgtouser(unum, hostmask);
        if (user->chans.cursi > 0) {
          sprintf(buf, "  On channel%s", (1 == user->chans.cursi) ? ":" : "s:");

          for (j = 0; j < (user->chans.cursi); j++) {
            chan = (channel **)(user->chans.content);
            if ((strlen(chan[j]->name) + strlen(buf)) > 400) {
              strcat(buf," [...]"); 
              break;
            } else {
              chanmodes = getchanmode2(chan[j], user->numeric);
              if (chanmodes < 0) {
                strcat(buf, " ?");
              } else {
                if (isflagset(chanmodes, um_o)) {
                  strcat(buf, " @");
                } else {
                  strcat(buf, "  ");
                }
              }
              strcat(buf, chan[j]->name);
            }
          }
        } else {
          strcpy(buf,"  Not on any (nonlocal-)channels");
        }
        msgtouser(unum, buf);
      }
      user = (void *)user->next;
    }
  }
  if (c > 500) { msgtouser(unum,"-- More than 500 matches - list truncated"); }
  sprintf(buf, "-- End of list -- Found %d user%s", c , (1 == c) ? "" : "s");
  msgtouser(unum, buf);
  regfree(&preg);
}

int rnlcomp(rnlentry *a, rnlentry *b) {
  if (a->n==b->n) { return 0; }
  if (a->n>b->n) { return -1; } else { return 1; }
}

void rnlsort(array *myrnl) {
  if (myrnl->cursi==0) { return; }
  qsort(myrnl->content,myrnl->cursi,sizeof(rnlentry),(void *)rnlcomp);
}

void dornc(long unum, char *tail) {
  char tmps2[TMPSSIZE]; int i=0; rnlentry *tmpp; int j, k;
  array myrnl; rnlentry *tmp2; long l;
  msgtouser(unum,"Top 20 Realnames on the net:");
  array_init(&myrnl,sizeof(rnlentry));
  for (j=0;j<256;j++) {
    tmp2=(rnlentry *)(mainrnl[j].content);
    for (k=0;k<mainrnl[j].cursi;k++) {
      l=array_getfreeslot(&myrnl);
      tmpp=(rnlentry *)(myrnl.content);
      tmpp[l]=tmp2[k];
    }
  }
  rnlsort(&myrnl);
  tmpp=(rnlentry *)(myrnl.content);
  while ((i<20) && (i<myrnl.cursi)) {
    sprintf(tmps2,"(%2d) %3d %-50s",i+1,tmpp[i].n,tmpp[i].r);
    msgtouser(unum,tmps2);
    i++;
  }
  array_free(&myrnl);
}

void dointtobase64(long unum, char *tail) {
  unsigned long a; int res; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  res=sscanf(tail,"%s %lu",tmps2,&a);
  if (res!=2) {
    msgtouser(unum,"Syntax: inttobase64 integer");
    return;
  }
  longtotoken(a,tmps2,10);
  sprintf(tmps3,"%lu (10) == %s (BASE64)",a,tmps2);
  msgtouser(unum,tmps3);
}

void dobase64toint(long unum, char *tail) {
  unsigned long a; int res; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: base64toint base64encodednumber");
    return;
  }
  a=tokentolong(tmps3);
  sprintf(tmps2,"%s (BASE64) == %lu (10)",tmps3,a);
  msgtouser(unum,tmps2);
}

void dodie(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,998)) { return; }
  res=sscanf(tail,"%s %[^\n]",tmps2,tmps3);
  if (res<2) { numtonick(unum,tmps2); sprintf(tmps3,"Terminated by %s",tmps2); }
  msgtouser(unum,"Terminating...");
  sendtouplink("%s SQ %s :%s\r\n",servernumeric,myname,tmps3);
  fflush(sockout);
}

void donoticemask(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; authdata ad; unsigned long ul;
  if (getauthlevel(unum)<1) {
    msgtouser(unum,"This command only makes sense when you are authed.");
    return;
  }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) { /* Show noticemask */
    if (!isircop(unum)) {
      msgtouser(unum,"Warning: You are not opered, so whatever your noticemask is set to, it will be 0 as long as you're not opered.");
    }
    ul=getnoticemask(unum);
    sprintf(tmps2,"Your current noticemask is %lu",ul);
    msgtouser(unum,tmps2);
  } else { /* Set new mask */
    getauthedas(tmps2,unum);
    getuserdata(&ad,tmps2);
    if (ad.authlevel<1) {
      msgtouser(unum,"That user does not exist in the database or database is down.");
      return;
    }
    ul=strtoul(tmps3,NULL,10);
    ad.noticemask=ul;
    updateuserinul(ad);
    setnoticemask(unum,ul);
    sprintf(tmps2,"Noticemask set to %lu",ul);
    msgtouser(unum,tmps2);
  }
}

void dochanfix(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  channel *a;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: chanfix channelname");
    return;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  deopall(a);
  res=doreop(a);
  switch(res) {
    case REOP_OPPED:
      msgtouser(unum,"One or more users reopped.");
      break;
    
    case REOP_NOINFO:
      msgtouser(unum,"Nothing known about that channel.");
      break;
      
    case REOP_NOREGOPS:
      msgtouser(unum,"Channel has no candidates for reopping.");
      break;
      
    case REOP_NOMATCH:
      msgtouser(unum,"No reop candidates on channel; all modes cleared.");
      break;
      
    default:
      msgtouser(unum,"!!! Something strange happened doing reop.");
      break;
  }
}

void doshowregs(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  channel *a;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: showregs channelname");
    return;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  showregs(unum, a);
}

void dochanopstat(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  channel *a;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: chanopstat channelname");
    return;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  showchansummary(unum, a);
}

void dochanoplist(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  channel *a; int all=0;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res<2) {
    msgtouser(unum,"Syntax: chanoplist channelname [-all]");
    return;
  }
  if (res==3 && !strcmp(tmps4,"-all")) {
    all=1;
  }
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  showchan(unum, a, all);
}

void doctcp(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  char target[100];
  longtotoken(unum,target,5);
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<1) { return; }
  if (strlen(tmps2)<3) { return; }
  if (strcmp(&tmps2[1],"VERSION\001")==0) { /* CTCP Version Request */
    putlog("Sending CTCP VERSION Reply");
    sendtouplink("%sAAB O %s :\001VERSION eggdrop v1.6.11\001\r\n",servernumeric, target);
    fflush(sockout);
  }
  if (strcmp(&tmps2[1],"PING")==0) { /* CTCP PING */
    if (res<2) { return; }
    tmps3[50]=0;
    sendtouplink("%sAAB O %s :\001PING %s\r\n",
      servernumeric, target, tmps3);
    fflush(sockout);
  }
  if (strcmp(&tmps2[1],"GENDER\001")==0) { /* GENDER */
    putlog("Sending CTCP Gender Reply");
    sendtouplink("%sAAB O %s :\001GENDER the uberservice has no gender\001\r\n",
      servernumeric, target);
    fflush(sockout);
  }

}

void dorehash(long unum, char *tail) {
  int b;
  if (!checkauthlevel(unum,999)) { return; }
  msgtouser(unum,"Rehashing...");
  b=loadconfig(configfilename,1);
  if (b<0) {
    newmsgtouser(unum,"Error, return code: %d",b);
  } else {
    newmsgtouser(unum,"Seems to have succeeded.");
  }
}
