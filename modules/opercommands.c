/*
Operservice 2 - opercommands.c
Loadable module
(C) Michael Meier & Henrik Mühe 2000-2002 - released under GPL
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

/*
 First, the name of this mod.
 Second, the amount of items to show before truncating
*/

#define MODNAM      "opercommands"

void doobroadcastcmd(long unum, char *tail) {
  int res;          // Int for results
  long msgcount;       // Int to count users the text was send to
  char text[TMPSSIZE];    // Char for the text (might be too long)
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char tmpstr[TMPSSIZE];    // Char for several purposes
  // Simple error checking
  res = sscanf(tail, "%s %[^\n]", cmdname, text);
  if (res < 2) {
    sprintf(tmpstr, "Syntax: %s text", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  // Preparing
  sprintf(tmpstr, "(oBroadcast) %s", text);
  msgcount = 0;
  tmpstr[350]='\0';  // Truncate to 350 chars
  msgcount=noticeallircops(tmpstr);
  // Message sent successfully
  sprintf(tmpstr, "Your message was sent to %ld operators.", msgcount);
  msgtouser(unum, tmpstr);
}

void dombroadcastcmd(long unum, char *tail) {
  userdata *udata;      // Struct for User Data as of User DB
  int res;          // Int for results
  int msgcount;       // Int to count users the text was send to
  int hcount;         // Int for a "for" loop
  char text[TMPSSIZE];    // Char for the text (might be too long)
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char tmpstr2[TMPSSIZE];   // Char for several purposes
  char mask[TMPSSIZE];    // Char for the mask we wanna broadcast to
  char target[10];      // String'y'fied numeric
  // Simple error checking
  res = sscanf(tail, "%s %s %[^\n]", cmdname, mask, text);
  if (res < 3) {
    sprintf(tmpstr, "Syntax: %s mask text", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  // Preparing
  sprintf(tmpstr, "(mBroadcast) %s", text);
  msgcount = 0;
  // Searching for users with the right mask
  for (hcount = 0; hcount < SIZEOFUL; hcount++) {
    udata = uls[hcount];
    while (udata != NULL) {
      // Check whether user is or is not matched by the mask
      // Checking for deaf-mode to prevent O from getting into
      // a message loop when PrivMSGing another service.

      sprintf(tmpstr2, "%s!%s@%s", udata->nick, udata->ident, udata->host);
      if ((match2strings(mask, tmpstr2)) && (ischarinstr('d', udata->umode) != 1)) {
        /* Two ideas, one is using the build in function, the other one
        is generally noticeing the message, second one seems to be better.
        First:
        msgtouser(udata->numeric, tmpstr);
        Second:
        */
        longtotoken(udata->numeric, target, 5);
        fprintf(sockout, "%sAAB O %s :%s\r\n", servernumeric, target, tmpstr);
          fflush(sockout);
        msgcount++;
      }
      udata = (void *)udata->next;
    }
  }
  // Message sent successfully
  sprintf(tmpstr, "Your message was sent to %i users.", msgcount);
  msgtouser(unum, tmpstr);
}


void dokillcmd(long unum, char *tail) {
  // Define :]
  userdata *udata;      // Struct for User Data as of User DB
  int res;          // Int for results
  long numeric;       // Long for the user's numeric
  char reason[TMPSSIZE];    // Char for the reason (might be too long)
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char nickname[NICKLEN + 1]; // Char for the user's nickname
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char target[10];      // String'y'fied numeric
  // Simple error checking
  res = sscanf(tail, "%s %s %[^\n]", cmdname, tmpstr, reason);
  mystrncpy(nickname,tmpstr,NICKLEN);
  toLowerCase(nickname);
  if (res < 2) {
    sprintf(tmpstr, "Syntax: %s nick [reason]", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  if (res == 2) {
    // No reason given
    udata = getudptr(unum);
    if (udata==NULL) { return; /* This should not happen! */ }
    sprintf(reason, "Requested by %s", udata->nick);
  }
  // Find the user *we* want to kill
  numeric = nicktonum(nickname);
  if (numeric < 0) {
    msgtouser(unum, "I don't know that user!");
    return;
  }
  longtotoken(numeric, target, 5);
  // Work
  fprintf(sockout, "%s D %s :%s %s\r\n", servernumeric, target, myname, reason);
  fflush(sockout);
  deluserfromallchans(numeric);
  delautheduser(numeric);
  if (!killuser(numeric)) {
    fakeuserkill(numeric);
    putlog("!!! KILL failed on dokillcmd: %s !!!", lastline);
    msgtouser(unum, "KILL was sent, but User could not be removed from my internal data. This is NOT good.");
  }
  msgtouser(unum, "KILL sent.");
}

void dorawnumericcmd(long unum, char *tail) {
  // Define :]
  int res;          // Int for results
  long numeric;       // Long for numeric
  char token[10];       // Char for tokenized numeric
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char nick[NICKLEN + 1];   // Char for the user's nickname
  char tmpstr[TMPSSIZE];    // Char for several purposes
  // Simple error checking
  res = sscanf(tail, "%s %s", cmdname, tmpstr);
  mystrncpy(nick,tmpstr,NICKLEN);
  toLowerCase(nick);
  if (res < 2) {
    newmsgtouser(unum, "Syntax: %s nick", cmdname);
    return;
  }
  // Get the user's numeric and tokenize it
  numeric = nicktonum(nick);
  if (numeric < 0) {
    msgtouser(unum, "I don't know that user!");
    return;
  }
  longtotoken(numeric, token, 5);
  sprintf(tmpstr, "Data for %s: Numeric: %ld / Tokenized numeric: %s", nick, numeric, token);
  msgtouser(unum, tmpstr);
}

void dorawcmd(long unum, char *tail) {
  int res;          // Int for results
  char cmdname[TMPSSIZE];   // Char for the command name
  char raw[TMPSSIZE];     // Char for the user's nickname
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char mode[TMPSSIZE];    // Char for several purposes
  // Simple error checking
  res = sscanf(tail, "%s %s %[^\n]", cmdname, mode, raw);
  if (res < 3) {
    sprintf(tmpstr, "Syntax: %s mode commands", cmdname);
    msgtouser(unum, tmpstr);
    msgtouser(unum, "where mode is either 'server' or 'client',");
    return;
  }
  toLowerCase(mode);
  if (strcmp(mode, "server") == 0) {
    strcpy(tmpstr, servernumeric);
  } else if (strcmp(mode, "client") == 0) {
    sprintf(tmpstr, "%sAAB", servernumeric);
  } else {
    msgtouser(unum, "There are two modes (first argument), server and client, use one of them");
    return;
  }
  /*
   We DO NOT perform ANY error checks here. If the user does
   send a BAD commandline (really bad I mean) servers might
   crash etc.
   Use this with CARE!
  */
  raw[450]='\0'; // At least truncate the command
  fprintf(sockout, "%s %s\r\n", tmpstr, raw);
  fflush(sockout);
  msgtouser(unum, "Raw done.");
}

void domkpasswdcmd (long unum, char *tail) {
  int res;          // Int for results
  char cmdname[TMPSSIZE];   // Char for the command name
  char plain[TMPSSIZE];   // Char for the plaintext password
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char tmps2[TMPSSIZE];
  // Simple error checking
  res = sscanf(tail, "%s %s", cmdname, plain);
  if (res < 2) {
    sprintf(tmpstr, "Syntax: %s password", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  encryptpwd(tmps2,plain);
  sprintf(tmpstr, "Plaintext: %s => Crypted: %s", plain, tmps2);
  msgtouser(unum, tmpstr);
}

void dobroadcast(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  res=sscanf(tail,"%s %[^\n]",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: broadcast message");
    return;
  }
  sendtouplink("%sAAB O $* :(Broadcast) %s\r\n",servernumeric,tmps3);
  msgtouser(unum,"Message broadcasted.");
}

void dosbroadcast(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  serverdata *s; unsigned long nsrv, nus;
  res=sscanf(tail,"%s %s %[^\n]",tmps2,tmps3,tmps4);
  if (res!=3) {
    msgtouser(unum,"Syntax: sbroadcast targetserver/servermask message");
    return;
  }
  s=sls; nsrv=0; nus=0;
  toLowerCase(tmps3);
  if (strlen(tmps3)<5) {
    msgtouser(unum,"Invalid targetservermask"); return;
  }
  while (s!=NULL) {
    strcpy(tmps2,s->nick); toLowerCase(tmps2);
    if (match2strings(tmps3,tmps2)) {
      nus+=countusersonsrv(s->numeric);
      if (nus>0) {
        nsrv++;
        sendtouplink("%sAAB O $%s :(sBroadcast) %s\r\n",servernumeric,s->nick,tmps4);
      }
    }
    s=(void *)s->next;
  }
  if (nsrv==0) {
    msgtouser(unum,"No Server matching that mask on the network.");
  } else {
    newmsgtouser(unum,"Message sent to %lu user%s on %lu server%s.",
      nus,(nus==1) ? "" : "s", nsrv, (nsrv==1) ? "" : "s");
  }
}

void opercommands_init() {
  setmoduledesc(MODNAM, "Mod containing some helpful commands for opers");
  registercommand2(MODNAM, "obroadcast", doobroadcastcmd, 1, 0,
          "obroadcast text\tUsed to send a message to all oper'ed users");
  registercommand2(MODNAM, "mbroadcast", dombroadcastcmd, 1, 900,
          "mbroadcast text\tUsed to send a message to all users matching a certain mask");
  registercommand2(MODNAM, "kill", dokillcmd, 1, 900,
          "kill nick [reason]\tCommand to kill a certain user");
  registercommand2(MODNAM, "raw", dorawcmd, 1, 999,
          "raw mode commands\tCommand to send raw server messages");
  registercommand2(MODNAM, "rawnumeric", dorawnumericcmd, 1, 999,
          "rawnumeric nick\tCommand to get the tokenized numeric of a user");
  registercommand2(MODNAM, "mkpasswd", domkpasswdcmd, 1, 0,
          "mkpasswd password\tCommand to encrypt a plaintext password");
  registercommand2(MODNAM, "broadcast", dobroadcast, 1, 900,
          "broadcast text\tSends a message to all users on the network");
  registercommand2(MODNAM, "sbroadcast", dosbroadcast, 1, 900,
          "sbroadcast server text\tSends a message to all users on specific server(s)");
}

void opercommands_cleanup() {
  deregistercommand("obroadcast");
  deregistercommand("mbroadcast");
  deregistercommand("kill");
  deregistercommand("raw");
  deregistercommand("rawnumeric");
  deregistercommand("mkpasswd");
  deregistercommand("broadcast");
  deregistercommand("sbroadcast");
}
