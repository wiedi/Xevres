/*
Xevres (based on Operservice 2)
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
#include "mysql_version.h"
#include "mysql.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

/* Realname-GLINE-Functions */

void rnglcreate() {
  array_init(&rngls,sizeof(realnamegline));
}

void rngladd(char *mask, char *creator, long expires, int howtoban, char *reason) {
  unsigned long i; userdata *a; char t1[TMPSSIZE], t2[TMPSSIZE]; long tl;
  realnamegline *tmpp;
  tmpp=(realnamegline *)(rngls.content);
  for (i=0;i<rngls.cursi;i++) {
    if (strcmp(mask,tmpp[i].mask)==0) {
      array_delslot(&rngls,i); i--;
      tmpp=(realnamegline *)(rngls.content);
    }
  }
  i=array_getfreeslot(&rngls);
  tmpp=(realnamegline *)(rngls.content);
  tmpp[i].howtogline=howtoban;
  tmpp[i].expires=expires;
  tmpp[i].timesused=0;
  strcpy(tmpp[i].creator,creator);
  strcpy(tmpp[i].reason,reason);
  strcpy(tmpp[i].mask,mask);
  strcpy(t1,mask); toLowerCase(t1);
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      strcpy(t2,a->realname);
      toLowerCase(t2);
      if (match2strings(t1,t2)) {
        char gl[TMPSSIZE];
        /* Hossa, this one is gonna have to leave the net :-P */
        tl=expires-getnettime();
        if (tl>RNGTIM) { tl=RNGTIM; }
        if (howtoban==1) {
          sprintf(gl,"*@%s",a->host);
        } else {
          sprintf(gl,"%s@%s",a->ident,a->host);
        }
        sprintf(t2,"%s!%s@%s (%s) matches RealnameGLINE %s - %s glined",
          a->nick,a->ident,a->host,a->realname,mask,gl);
        putlog("%s",t2);
        sendtonoticemask(NM_RNGL,t2);
        addgline(gl,reason,"RealnameGLINE",tl,1);
        rnglinelog(getnettime(), t2);
      }
      a=(void *)a->next;
    }
  }
}

void rnglcheck(userdata *a) {
  unsigned long i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; long tl;
  char gl[TMPSSIZE]; realnamegline *tmpp;
  strcpy(tmps2,a->realname);
  toLowerCase(tmps2);
  tmpp=(realnamegline *)(rngls.content);
  for (i=0;i<rngls.cursi;i++) {
    strcpy(tmps3,tmpp[i].mask);
    toLowerCase(tmps3);
    if (match2strings(tmps3,tmps2)) {
      /* Hossa, this one is gonna have to leave the net :-P */
      tmpp[i].timesused++;
      tl=tmpp[i].expires-getnettime();
      if (tl>RNGTIM) { tl=RNGTIM; }
      if (tl>0) {
        if (tmpp[i].howtogline==1) {
          sprintf(gl,"*@%s",a->host);
        } else {
          sprintf(gl,"%s@%s",a->ident,a->host);
        }
        sprintf(tmps3,"%s!%s@%s (%s) matches RealnameGLINE %s - %s glined",
          a->nick,a->ident,a->host,a->realname,tmpp[i].mask,gl);
        putlog("%s",tmps3);
        sendtonoticemask(NM_RNGL,tmps3);
        addgline(gl,tmpp[i].reason,"RealnameGLINE",tl,1);
        rnglinelog(getnettime(), tmps3);
      }
    }
  }
}

void rngldel(char *mask) {
  unsigned long i;
  for (i=0;i<rngls.cursi;i++) {
    if (strcmp(mask,((realnamegline *)(rngls.content))[i].mask)==0) {
      array_delslot(&rngls,i); i--;
    }
  }
}

void rnglexpire() {
  unsigned long i;
  for (i=0;i<rngls.cursi;i++) {
    if (((realnamegline *)(rngls.content))[i].expires<getnettime()) {
      array_delslot(&rngls,i); i--;
    }
  }
}

void rnglload() {
  int res;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  res=mysql_query(&sqlcon,"SELECT * FROM realnameglines");
  if (res!=0) {
    putlog("!!! Failed to query realname-gline-list from database !!!");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=5) {
    putlog("!!! Realname-Gline-List in database has invalid format !!!");
    return;
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if ((strlen(myrow[0])>RNGLEN) || (strlen(myrow[2])>AUTHUSERLEN) || (strlen(myrow[4])>RNGREAS)) {
      putlog("!!! Malformed realname-gline-entry in database! !!!");
    } else {
      rngladd(myrow[0],myrow[2],strtol(myrow[3],NULL,10),strtol(myrow[1],NULL,10),myrow[4]);
    }
  }
  mysql_free_result(myres);
  rnglexpire();
}

void rnglsave() {
  char query[TMPSSIZE]; int i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  rnglexpire();
  mysql_query(&sqlcon,"DROP TABLE IF EXISTS realnameglines");
  mysql_query(&sqlcon,"CREATE TABLE realnameglines (mask VARCHAR (150) not null , howtogline INT not null , creator VARCHAR(40) , expires BIGINT , reason VARCHAR (100) not null , PRIMARY KEY (mask), INDEX (mask), UNIQUE (mask))");
  for (i=0;i<rngls.cursi;i++) {
    mysql_escape_string(tmps2,((realnamegline *)(rngls.content))[i].mask,
      strlen(((realnamegline *)(rngls.content))[i].mask));
    mysql_escape_string(tmps3,((realnamegline *)(rngls.content))[i].creator,
      strlen(((realnamegline *)(rngls.content))[i].creator));
    mysql_escape_string(tmps4,((realnamegline *)(rngls.content))[i].reason,
      strlen(((realnamegline *)(rngls.content))[i].reason));
    sprintf(query,"INSERT INTO realnameglines VALUES('%s',%d,'%s',%ld,'%s')",
      tmps2,((realnamegline *)(rngls.content))[i].howtogline,tmps3,((realnamegline *)(rngls.content))[i].expires,tmps4);
    mysql_query(&sqlcon,query);
  }
}

void dorngline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  long dur; int howto; char tmps7[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res!=5) {
    msgtouser(unum,"Syntax: rngline pattern duration bantype reason");
    msgtouser(unum,"where bantype=1 means *@host and bantype=2 means user@host");
    return;
  }
  dur=durationtolong(tmps4);
  if (dur<1) {
    msgtouser(unum,"The duration you gave is not valid");
    return;
  }
  if (strlen(tmps3)<4) {
    msgtouser(unum,"The pattern you gave is too short. Needs to be at least 4 chars (including wildcards)");
    return;
  }
  howto=strtol(tmps5,NULL,10);
  if ((howto<1) || (howto>2)) {
    msgtouser(unum,"The bantype you gave is invalid");
    return;
  }
  if (strlen(tmps6)>RNGREAS) { tmps6[RNGREAS]='\0'; msgtouser(unum,"Warning: Reason truncated"); }
  if (strlen(tmps3)>RNGLEN) {
    msgtouser(unum,"Your Banmask is too long.");
    return;
  }
  unescapestring(tmps3,tmps7);
  strcpy(tmps3,tmps7);
  getauthedas(tmps2,unum);
  rngladd(tmps3,tmps2,dur+getnettime(),howto,tmps6);
  newmsgtouser(unum,"Added Realnamegline on '%s' with banmask %s for %ld seconds (Reason: %s)",
    tmps3,(howto==1) ? "*@host" : "user@host", dur, tmps6);
}

void dornungline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: rnungline pattern");
    return;
  }
  unescapestring(tmps3,tmps2);
  strcpy(tmps3,tmps2);
  rngldel(tmps3);
  msgtouser(unum,"Done");
}

void dornglist(long unum, char *tail) {
  int i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  realnamegline *tmpp;
  rnglexpire();
  tmpp=((realnamegline *)(rngls.content));
  if (rngls.cursi==0) {
    msgtouser(unum,"Currently no Realname-GLINEs set.");
    return;
  }
  sprintf(tmps2,"%-40s %-17s %-15s %-8s %s","Pattern","expires in","by","# used","Reason");
  msgtouser(unum,tmps2);
  for (i=0;i<rngls.cursi;i++) {
    longtoduration(tmps3,tmpp[i].expires-getnettime());
    sprintf(tmps2,"%-40s %17s %-15s %8lu %s",tmpp[i].mask,tmps3,tmpp[i].creator,tmpp[i].timesused,tmpp[i].reason);
    msgtouser(unum,tmps2);
  }
  msgtouser(unum,"--- End of list ---");
}

/*
rnglinelog(): log activated/triggered rnglines to a searchable SQL table.
*/
void rnglinelog(unsigned long timestamp, char *event) {
  char p[TMPSSIZE]; char sqlquery[TMPSSIZE*2];
  mysql_escape_string(p, event, strlen(event));
  sprintf(sqlquery, "INSERT INTO rnglinelog VALUES (0, %lu, '%s')", timestamp, p);
  mysql_query(&sqlcon, sqlquery);
}

/*
dornglogsearch(): searches thru saved activated rnglines.
*/
void dornglogsearch(long unum, char *tail) {
  int res, rows = 0;
  char args1[TMPSSIZE], args2[TMPSSIZE], p[TMPSSIZE];
  char sqlquery[TMPSSIZE*2]; char *c; MYSQL_RES *myres;
  MYSQL_ROW myrow; time_t ts; struct tm *loctime;
  if (!checkauthlevel(unum, 990)) { return; }
  res = sscanf(tail, "%s %s", args1, args2);
  if (res != 2) {
    msgtouser(unum, "Syntax: rnglogsearch pattern");
    msgtouser(unum, "The search will be performed on the nick, hostmask and realname.");
    return;
  }
  /* replace '*' w. SQL ditto, ie: '%' */
  c = &args2[0];
  while (*c != '\0') {
    if (*c == '*') { *c = '%'; }
    c++;
  }
  mysql_escape_string(p, args2, strlen(args2));
  sprintf(sqlquery, "SELECT * FROM rnglinelog WHERE event LIKE '%s' ORDER by timestamp DESC LIMIT 0,101", p);
  res = mysql_query(&sqlcon, sqlquery);
  if (res!=0) {
    msgtouser(unum, "For some reason the database-query failed. Try again later.");
    return;
  }
  myres = mysql_store_result(&sqlcon);
  rows = 0;
  if (mysql_num_fields(myres) != 3) {
    msgtouser(unum, "Sorry, the log in the database has illegal format");
    mysql_free_result(myres);
    return;
  }
  while ((myrow = mysql_fetch_row(myres)) != NULL) {
    if (rows > 99) {
      msgtouser(unum, "--- Too many matches - List truncated");
      break;
    }
    ts = strtol(myrow[1], NULL, 10);
    loctime = localtime(&ts);
    strftime(p, 100, "%A, %d.%m.%Y %T", loctime);
    sprintf(args1, "%s; %s", p, myrow[2]);
    msgtouser(unum, args1);
    rows++;
  }
  sprintf(args1, "--- End of list - %d matches ---", rows);
  msgtouser(unum, args1);
  mysql_free_result(myres);
}

/*
dornglogpurge(): purges old entries in the rnglinelog.
*/
void dornglogpurge(long unum, char *tail) {
  int res;
  long dur;
  char args1[TMPSSIZE], args2[TMPSSIZE], p[TMPSSIZE];
  if (!checkauthlevel(unum, 998)) { return; }
  res=sscanf(tail, "%s %s %s", args1, args2, p);
  if (res != 2) {
    msgtouser(unum,"Syntax: rnglogpurge <time>");
    msgtouser(unum,"Where time is in the same format that e.g. the gline-command uses.");
    msgtouser(unum,"Example: rnglogpurge 1y  - that would purge all logentries older than 1 year");
    return;
  }
  dur = durationtolong(args2);
  if (dur < 3600) {
    msgtouser(unum,"Not a valid duration. (has to be at least 1 hour)");
    return;
  }
  dur=getnettime()-dur;
  if ((dur < 0) || (dur > getnettime())) { /* Must have overflowed */
    msgtouser(unum, "Not a valid duration. (has to be at least 1 hour)");
    return;
  }
  sprintf(p, "DELETE FROM rnglinelog WHERE timestamp < %ld", dur);
  mysql_query(&sqlcon, p);
  msgtouser(unum, "Should be done.");
}

/* End of Realname-GLINE-Functions */
