/*
Operservice 2 - glinecommands.c
Loadable module
(C) Michael Meier 2002 - released under GPL
Parts (C) Henrik Mühe 2001
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

/* Because the name of the mod is used in several places, we define it here so
   we can easily change it. It should be all lowercase. */
#define MODNAM "glinecommands"
/* Note: This .c-file will have to be named MODNAM.c! */
#define UNGLINEMAX    500

/* Return values: <0 = Error, 0 = gline *@host, 1 = gline user@host */
int getglinemode(long numeric) {
  /*
  One user from that host         => gline *@host
  More users from that host, identd on  => gline ident@host
  More users from that host, identd off   => gline ident@host (woo?)
  */
  int i;            // Int for several loops
  userdata *udata, *tudata;
  tudata = getudptr(numeric);
  if (tudata == NULL) {
    return -1;
  }
  for (i = 0; i < SIZEOFUL; i++) {
    udata = uls[i];
    while (udata!=NULL) {
      if (strcmp(udata->host, tudata->host) == 0) {
        if (strcmp(udata->ident, tudata->ident) != 0) {
          return 1;
        }
      }
      udata=(void *)udata->next;
    }
  }
  return 0;
}

void doblockcmd (long unum, char *tail) {
  // Define :]
  userdata *tudata; // Struct for User Data as of User DB
  int res;          // Int for results
  int override;       // Mode flag
  long ldur;          // Long for the duration in secs
  char cmdname[TMPSSIZE];   // Char for the command name
  char mode[TMPSSIZE];    // Char for the mode
  char nick[TMPSSIZE];    // Char for the user's nickname
  char mask[TMPSSIZE];    // Char for the user's hostmask
  char duration[TMPSSIZE];  // Char for the duration
  char reason[TMPSSIZE];    // Char for the reason (might be too long)
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char tmpstr2[TMPSSIZE];   // Char for several purposes
  int mode_gline = 0;     // Int to show: final kind of gline
  // Simple error checking
  res = sscanf(tail, "%s %s", tmpstr, tmpstr2);
  if (res<2) { tmpstr2[0]='\0'; }
  if (tmpstr2[0] == '-') {
    // User wants to override the gline mode
    res = sscanf(tail, "%s %s %s %s %[^\n]", cmdname, mode, nick, duration, reason);
    override = 1;
  } else {
    res = sscanf(tail, "%s %s %s %[^\n]", cmdname, nick, duration, reason);
    override = 0;
  }
  if (res < 1) {
    putlog("!!! No commandname in opercommands.c !!!");
    return;
  } else if (((res!=4) && (override==0)) || ((res!=5) && (override==1))) {
    newmsgtouser(unum, "Syntax: %s [override switch] nick duration reason", cmdname);
    msgtouser(unum, " Where override switch is -u to force glineing of user@host and");
    msgtouser(unum, " -h to force *@host.");
    return;
  }
  nick[NICKLEN]='\0'; // Truncate nick
  toLowerCase(nick);
  tudata = getudptr(nicktonum(nick));
  if (tudata == NULL) {
    msgtouser(unum, "I don't know that user!");
    return;
  }
  // Find right automode IF needed (!)
  if (override != 1) {
    mode_gline = getglinemode(tudata->numeric);
  } else {
    toLowerCase(mode);
    if (strcmp("-u", mode) == 0) {
      mode_gline = 1;
    } else {
      mode_gline = 0;
    }
  }
  // find the duration (in SECONDS!)
  ldur = durationtolong(duration);
  if (ldur < 0) {
    msgtouser(unum, "The duration you've supplied is invalid.");
    return;
  }
  if (ldur>EXCESSGLINELEN) {
    msgtouser(unum, "You're creating an excessively long gline. If you do so without having");
    msgtouser(unum, "a very good reason to do so, someone will hurt you with a SCSI cable.");
  }
  /*  mode_gline == 1   => user@host
      mode_gline == 0   => *@host   */
  if (mode_gline == 1) {
    sprintf(mask, "%s@%s", tudata->ident, tudata->host);
  } else {
    sprintf(mask, "*@%s", tudata->host);
  }
  res=countusershit(mask);
  getauthedas(tmpstr,unum);
  addgline(mask, reason, tmpstr, ldur, 1);
  newmsgtouser(unum, "I've glined %s (hit %d user%s)", mask, res, (res==1) ? "" : "s");
  sprintf(tmpstr2,"GLINE %s, expires in %s, set by %s: %s (hit %d user%s)",mask,duration,tmpstr,reason,res,(res==1) ? "" : "s");
  sendtonoticemask(NM_GLINE,tmpstr2);
}

void dounglinemaskcmd (long unum, char *tail) {
  int res;          // Int for results
  int i;            // Int for a "for" loop
  int gcount;         // Int for row-counting
  char cmdname[TMPSSIZE];   // Char for the command name (^^)
  char mask[TMPSSIZE];    // Char for the MASK
  MYSQL_RES *myres; MYSQL_ROW myrow;    // MySQL stuff
  char tmpstr[TMPSSIZE];    // Char for several purposes
  char tmps4[TMPSSIZE];
  char sqlquery[TMPSSIZE];  // char for the SQL query
  // Simple error checking
  res = sscanf(tail, "%s %s", cmdname, mask);
  if (res < 1) {
    putlog("!!! No commandname in %s line %d !!!",__FILE__,__LINE__);
    return;
  } else if (res < 2) {
    sprintf(tmpstr, "Syntax: %s mask", cmdname);
    msgtouser(unum, tmpstr);
    return;
  }
  // Preparing mask for SQL
  for (i = 0; i < strlen(mask); i++) {
    if (mask[i] == '?') { mask[i] = '_'; }
    if (mask[i] == '*') { mask[i] = '%'; }
  }
  strcpy(tmpstr,mask);
  mysql_escape_string(mask,tmpstr,strlen(tmpstr));
  /*
   Now we just do some SQL stuff and finish this shit ;)
   SELECT * FROM glines WHERE hostmask LIKE 'mask'
   count & warn or remove :-)
  */
  sprintf(sqlquery, "SELECT * FROM glines WHERE hostmask LIKE '%s'", mask);
  res = mysql_query(&sqlcon, sqlquery);
  if (res != 0) {
    // Error during SQL query
    msgtouser(unum, "SQL reported an error. Aborting.");
    return;
  }
  myres = mysql_store_result(&sqlcon);
  gcount = mysql_num_rows(myres);
  if (gcount >= UNGLINEMAX) {
    newmsgtouser(unum, "Sorry, the pattern you specified would match more than %i (%i) GLINEs", UNGLINEMAX,gcount);
    msgtouser(unum, "You probably mistyped something. Removal aborted.");
    mysql_free_result(myres);
    return;
  }
  getauthedas(tmps4,unum);
  while ((myrow = mysql_fetch_row(myres))) {
    if (strlen(myrow[0])>400) { continue; }
    // Ungline
    sprintf(tmpstr,"GLINE %s removed by %s",myrow[0],tmps4);
    sendtonoticemask(NM_GLINE,tmpstr);
    newmsgtouser(unum,"Removing: %s", myrow[0]);
    remgline(myrow[0], 1);
  }
  mysql_free_result(myres);
  sprintf(tmpstr, "Removed %i glines.", gcount);
  msgtouser(unum, tmpstr);
}

void doglist(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE*2]; int i;
  MYSQL_RES *myres; MYSQL_ROW myrow; int dofullsearch=0; int docountonly=0;
  purgeglines();
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res<2) {
    msgtouser(unum,"Syntax: glist [-f|-c|-fc] pattern");
    msgtouser(unum,"Where -f also matches pattern in \"creator\" or \"reason\".");
    msgtouser(unum,"-c just prints the number of glines that match the pattern.");
    return;
  }
  if (res==3) {
    toLowerCase(tmps3);
    if ((strlen(tmps3)<2) || (tmps3[0]!='-')) {
      msgtouser(unum,"Invalid parameter");
      return;
    }
    for (i=1;tmps3[i]!='\0';i++) {
    	if (tmps3[i]=='f') { dofullsearch=1; continue; }
    	if (tmps3[i]=='c') { docountonly=1; continue; }
      msgtouser(unum,"Invalid parameter");
      return;
    }
    strcpy(tmps3,tmps4);
  }
  for (i=0;tmps3[i]!='\0';i++) {
    if (tmps3[i]=='*') { tmps3[i]='%'; }
  }
  mysql_escape_string(tmps4,tmps3,strlen(tmps3));
  if (dofullsearch==1) {
    sprintf(tmps2,"FROM glines WHERE hostmask LIKE '%s' OR reason LIKE '%s' OR creator LIKE '%s'",tmps4,tmps4,tmps4);
  } else {
    sprintf(tmps2,"FROM glines WHERE hostmask LIKE '%s'",tmps4);
  }
  if (docountonly==1) {
  	strcpy(tmps5,"SELECT COUNT(*) ");
  } else {
  	strcpy(tmps5,"SELECT * ");
  }
  strcat(tmps5,tmps2);
  res=mysql_query(&sqlcon,tmps5);
  if (res!=0) {
    putlog("!!! Failed to query gline-list from database !!!");
    msgtouser(unum,"!!! Failed to query gline-list from database !!!");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (docountonly==1) {
  	myrow=mysql_fetch_row(myres);
  	if ((myrow==NULL) || (mysql_num_fields(myres)!=1)) {
  		msgtouser(unum,"Query failed. (database error)"); return;
  	}
  	newmsgtouser(unum,"%s GLINEs match %s.",myrow[0],tmps3);
  	mysql_free_result(myres);
  	return;
  }
  if (mysql_num_fields(myres)!=4) {
    mysql_free_result(myres);
    putlog("!!! Failed to query gline-list from database !!!");
    msgtouser(unum,"!!! GList in database has invalid format !!!");
    return;
  }
  newmsgtouser(unum,"GLINEs matching %s:",tmps3);
  newmsgtouser(unum,"%-50s %-17s by %s: %s","Hostmask","expires in","whom","reason");
  i=0;
  while ((myrow=mysql_fetch_row(myres))) {
    longtoduration(tmps5,strtol(myrow[2],NULL,10)-getnettime());
    sprintf(tmps2,"%-50s %-17s by %s: %s",myrow[0],tmps5,myrow[1],myrow[3]);
    msgtouser(unum,tmps2);
    i++;
  }
  sprintf(tmps2,"--- End of list - %d matches ---",i);
  msgtouser(unum,tmps2);
  mysql_free_result(myres);
}

void dogline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE]; int r2;
  res=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5);
  if (res<3) {
    msgtouser(unum,"Syntax: gline hostmask duration [reason]");
    msgtouser(unum,"Note: O does do some sanity checks to the glines, and tries to count the users hit by it,");
    msgtouser(unum,"      but it does not alter the gline in any way before it is sent to the net. That means");
    msgtouser(unum,"      you can use any feature the ircd offers, even if O does not understand it.");
    return;
  }
  if (res==3) {
    numtonick(unum,tmps2);
    sprintf(tmps5,"requested by %s",tmps2);
  }
  r2=durationtolong(tmps4);
  if (r2<=0) {
    msgtouser(unum,"The duration you gave is invalid.");
    return;
  }
  if (r2>EXCESSGLINELEN) {
    msgtouser(unum, "You're creating an excessively long gline. If you do so without having");
    msgtouser(unum, "a very good reason to do so, someone will hurt you with a SCSI cable.");
  }
  if (tmps3[0]!='#') {
  	int h; int lastwaswildcard=0; int hadat=0;
  	for (h=0;tmps3[h]!='\0';h++) {
  		if (tmps3[h]=='@') { hadat=1; }
  		if (tmps3[h]=='*') {
  			lastwaswildcard=1;
  		} else {
  			if ((lastwaswildcard==1) && (hadat==1)) {
  				if ((tmps3[h]>='0') && (tmps3[h]<='9')) {
  					msgtouser(unum, "REJECTED: That mask is confusing (wildcard before number) and could fuck up on certain ircd versions.");
  					return;
  				}
  			}
  			lastwaswildcard=0;
  		}
  	}
    if (!ischarinstr('@',tmps3)) {
      msgtouser(unum,"Please use GLINEs of the form user@host or nick!user@host");
      if (getauthlevel(unum)<999) { return; }
    }
  }
  res=countusershit(tmps3);
  if (res>GLINEMAXHIT) {
    newmsgtouser(unum,"That gline would hit more than %d (%d) Users/Channels. You probably mistyped something.",GLINEMAXHIT,res);
    if ((res<(GLINEMAXHIT*10)) && (getauthlevel(unum)>=999)) {
      msgtouser(unum,"However, your authlevel is >=999, so I hope you know what you're doing... Executing command.");
    } else {
      return;
    }
  }
  getauthedas(tmps6,unum);
  sprintf(tmps2,"GLINE %s, expires in %s, set by %s: %s (hit %d %s%s)",tmps3,tmps4,tmps6,tmps5,res,(tmps3[0]=='#') ? "channel" : "user",(res==1) ? "" : "s");
  sendtonoticemask(NM_GLINE,tmps2);
  purgeglines();
  addgline(tmps3,tmps5,tmps6,r2,1);
  sprintf(tmps2,"Added GLINE %s, expires in %d seconds, hit %d %s%s.",tmps3,r2,res,(tmps3[0]=='#') ? "channel" : "user",(res==1) ? "" : "s");
  msgtouser(unum,tmps2);
}

void doungline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: ungline hostmask");
    return;
  }
  getauthedas(tmps4,unum);
  sprintf(tmps2,"GLINE %s removed by %s",tmps3,tmps4);
  sendtonoticemask(NM_GLINE,tmps2);
  remgline(tmps3,1);
  msgtouser(unum,"Done.");
}

void dosyncglines(long unum, char *tail) {
  int res; int i=0;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  purgeglines();
  res=mysql_query(&sqlcon,"SELECT * FROM glines");
  if (res!=0) {
    putlog("!!! Failed to query gline-list from database !!!");
    msgtouser(unum,"!!! Failed to query gline-list from database !!!");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=4) {
    putlog("!!! Failed to query gline-list from database !!!");
    msgtouser(unum,"!!! GList in database has invalid format !!!");
    return;
  }
  while ((myrow=mysql_fetch_row(myres))) {
    sendtouplink("%s GL * +%s %ld :%s\r\n",servernumeric,myrow[0],atol(myrow[2])-getnettime(),myrow[3]);
    i++;
  }
  fflush(sockout);
  newmsgtouser(unum,"%d GLINEs resynched.",i);
  mysql_free_result(myres);
}

void doclonegline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res==2) {
  	toLowerCase(tmps3);
    if (strcmp(tmps3,"0")==0) { glineonclones=0; }
    if (strcmp(tmps3,"1")==0) { glineonclones=1; }
    if (strcmp(tmps3,"2")==0) { glineonclones=2; }
    if (strcmp(tmps3,"on")==0) { glineonclones=1; }
    if (strcmp(tmps3,"off")==0) { glineonclones=0; }
  }
  newmsgtouser(unum,"Auto-GLINE on clones is now %s%s",(glineonclones>0) ? "on" : "off",
    (glineonclones==2) ? " (GLINEing whole subnets too)" : "");
}

void glinecommands_init(void) {
  setmoduledesc(MODNAM,"This mod provides *gline*-commands");
  registercommand2(MODNAM, "block", doblockcmd, 1, 900,
          "block [switch] nick\tCommand to gline the host/user by giving the users nickname\n        duration reason");
  registercommand2(MODNAM, "unglinemask", dounglinemaskcmd, 1, 900,
          "unglinemask pattern\tCommand to remove glines on a cartain mask");
  registercommand2(MODNAM, "glist", doglist, 1, 0,
          "glist [-f] [pattern]\tLists glines");
  registercommand2(MODNAM, "gline", dogline, 1, 900,
          "gline hostm dur [reas]\tSets a gline");
  registercommand2(MODNAM, "ungline", doungline, 1, 900,
          "ungline hostmask\tRemoves a gline");
  registercommand2(MODNAM, "syncglines", dosyncglines, 1, 900,
          "syncglines\tSynchs glines on all servers");
  registercommand2(MODNAM, "clonegline", doclonegline, 1, 900,
          "clonegline [0|1]\tTurns Auto-GLINEing of clones off or on");
}

/* This is called for cleanup (before the module gets unloaded)
   Again the name of the function has to be MODNAM_cleanup */
void glinecommands_cleanup(void) {
  deregistercommand("block");
  deregistercommand("unglinemask");
  deregistercommand("glist");
  deregistercommand("gline");
  deregistercommand("ungline");
  deregistercommand("syncglines");
  deregistercommand("clonegline");
}
