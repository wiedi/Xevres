/*
Operservice 2 - requestop.c
Functions for requesting op on opless channels
(C) Michael Meier 2000-2002 - released under GPL
Updated 07.04.2002 for CHANFIX
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
#ifndef WE_R_ON_BSD
#include <malloc.h>
#endif
#ifdef USE_DOUGLEA_MALLOC
#include "../dlmalloc/malloc.h"
#endif

#define MODNAM "requestop"
/* Requester must have been on the net for at least 5 minutes */
#define MUSTHAVEBEENON 300
#define LOGTOTABLE "requestoplog"

char logtypes[][120]={"",
                      "Channelservice reopped",
                      "Request granted on channel with <= 4 users",
                      "Request on channel with <= 20 users - first requester",
                      "Request on channel with <= 20 users - second requester (op granted)",
                      "Request on channel with > 20 users - first requester",
                      "Request on channel with > 20 users - second requester (forwarded to #tz)",
                      "Request that could not be handled by chanfix - Op granted",
                      ""};


typedef struct {
  char chan[CHANNELLEN+1];
  long timestamp;
} bigchanreq;

int sqlcreatedone=0;
char *chansnapshot=NULL;
int cnlen=0;

void addtochansnapshot(char *txt) {
  int i; long j;
  j=strlen(txt);
  chansnapshot=(char *)realloc((void *)chansnapshot,cnlen+j+1);
  if (chansnapshot==NULL) {
    putlog("!!! Out of memory in requestop.c/addtochansnapshot !!!");
    exit(1);
  }
  for (i=0;i<j;i++) {
    chansnapshot[i+cnlen]=txt[i];
  }
  cnlen+=j;
  chansnapshot[cnlen]='\0';
}

void clearchansnapshot() {
  free(chansnapshot);
  chansnapshot=NULL;
  cnlen=0;
}

void reqoplogentry(int type, unsigned long timestamp, char *requester, char *reqfor, char *chan, char *snapshot) {
  char erequester[TMPSSIZE], ereqfor[TMPSSIZE], echan[TMPSSIZE];
  char *esnapshot; char *sqlquer;
  if (sqlcreatedone==0) {
    sprintf(erequester,"CREATE TABLE %s (type INT not null , timestamp BIGINT not null , requester VARCHAR (250) not null , reqfor VARCHAR (250) not null , chan VARCHAR (250) not null , snapshot TEXT not null)",LOGTOTABLE);
    mysql_query(&sqlcon,erequester);
    sqlcreatedone=1;
  }
  esnapshot=(char *)malloc((strlen(snapshot)*2)+1);
  if (esnapshot==NULL) {
    putlog("!!! Out of memory in requestop.c/reqoplogentry !!!");
    exit(1);
  }
  mysql_escape_string(esnapshot,snapshot,strlen(snapshot));
  mysql_escape_string(erequester,requester,strlen(requester));
  mysql_escape_string(ereqfor,reqfor,strlen(reqfor));
  mysql_escape_string(echan,chan,strlen(chan));
  sqlquer=(char *)malloc(strlen(esnapshot)+strlen(erequester)+strlen(ereqfor)+strlen(echan)+100);
  sprintf(sqlquer,"INSERT INTO %s VALUES(%d,%ld,'%s','%s','%s','%s')",LOGTOTABLE,type,timestamp,erequester,ereqfor,echan,esnapshot);
  mysql_query(&sqlcon,sqlquer);
  free(sqlquer);
  free(esnapshot);
}

void dorequestop(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  int res; channel *c; chanuser *u; long i, j; int users, ops; int reqonchan;
  int wtoonchan; long wtonum; userdata *reqrec; userdata *reqforrec;
  userdata *ud; int didcsreop; char reqnick[TMPSSIZE]; char reqfornick[TMPSSIZE];
  res=sscanf(tail,"%s %s %s %s %s",tmps2,tmps3,tmps4,tmps5,tmps6);
  if ((res<2) || (res>3)) {
    msgtouser(unum,"Syntax: requestop #channel [nickname]");
    msgtouser(unum,"Requests op for nickname on #channel.");
    msgtouser(unum,"If you don't supply the nickname argument, your own nick will be used for it.");
    msgtouser(unum,"This function can be used to regain op on opless channels.");
    return;
  }
  toLowerCase(tmps3);
  if (tmps3[0]!='#') {
    msgtouser(unum,"You cannot gain ops on channels that don't start with a \"#\"");
    return;
  }
  c=getchanptr(tmps3);
  if (c==NULL) {
    msgtouser(unum,"That channel does not exist.");
    return;
  }
  if (res==3) { /* Nickname was given as arg */
    toLowerCase(tmps4);
    wtonum=nicktonu2(tmps4);
    if (wtonum<0) {
      msgtouser(unum,"That nick is not on the network");
      return;
    }
  } else {
    wtonum=unum;
  }
  if (splitservers.cursi>0) {
    msgtouser(unum,"There is currently one or more netsplit(s).");
    msgtouser(unum,"For obvious reasons, you cannot request ops during a netsplit.");
    msgtouser(unum,"Wait until it is over and then try again.");
    return;
  }
  reqrec=getudptr(unum);
  if (reqrec==NULL) { return; }
  if ((reqrec->connectat+MUSTHAVEBEENON)>getnettime()) {
    sprintf(tmps2,"You connected %ld seconds ago. To request ops, you must have been on the network for at least %d seconds.",
      getnettime()-reqrec->connectat,MUSTHAVEBEENON);
    msgtouser(unum,tmps2);
    msgtouser(unum,"Please note that this counter gets reset by nickchanges and netsplits.");
    return;
  }
  reqforrec=getudptr(wtonum);
  if (reqforrec==NULL) { return; }
  if ((reqforrec->connectat+MUSTHAVEBEENON)>getnettime()) {
    sprintf(tmps2,"The nick you requested op for connected %ld seconds ago. To request ops, it must have been on the network for at least %d seconds.",
      getnettime()-reqforrec->connectat,MUSTHAVEBEENON);
    msgtouser(unum,tmps2);
    msgtouser(unum,"Please note that this counter gets reset by nickchanges and netsplits.");
    return;
  }
  u=(chanuser *)(c->users.content);
  if (c->flags==0) {
    strcpy(tmps2,"No channelmodes\n");
  } else {
    strcpy(tmps2,"Chanmodes: ");
    j=strlen(tmps2);
    for (i=0;i<numcf;i++) {
      if (isflagset(c->flags,chanflags[i].v)) {
        tmps2[j]=chanflags[i].c;
        j++;
      }
    }
    tmps2[j]='\n';
    tmps2[j+1]='\0';
  }
  addtochansnapshot(tmps2);
  users=0; ops=0; reqonchan=0; wtoonchan=0; didcsreop=0;
  for (i=0;i<c->users.cursi;i++) {
    if (u[i].numeric==unum) { reqonchan=1; }
    if (u[i].numeric==wtonum) { wtoonchan=1; }
    strcpy(tmps6,"(");
    if (isflagset(u[i].flags,um_o)) { strcat(tmps6,"@"); } else { strcat(tmps6,"_"); }
    if (isflagset(u[i].flags,um_v)) { strcat(tmps6,"+)"); } else { strcat(tmps6,"_)"); }
    users++;
    ud=getudptr(u[i].numeric);
    if (ud!=NULL) {
      if (ischarinstr('k',ud->umode)) { /* There is a channelservice on the channel */
        if (isflagset(u[i].flags,um_o)) {
//          sprintf(tmps2,"There is a channelservice opped on that channel.");
//          msgtouser(unum,tmps2);
        } else { /* Channelservice on the chan, but not opped?! op it! */
          longtotoken(u[i].numeric,tmps2,5);
          fprintf(sockout,"%s M %s +o %s\r\n",servernumeric,tmps3,tmps2);
          changechanmod2(c,u[i].numeric,1,um_o);
          sprintf(tmps2,"Channelservice %s reopped.",ud->nick);
          msgtouser(unum,tmps2);
          didcsreop=1;
        }
      }
      strcat(tmps6,ud->nick); strcat(tmps6,"!");
      strcat(tmps6,ud->ident); strcat(tmps6,"@");
      strcat(tmps6,ud->host); strcat(tmps6," (");
      if ((strlen(tmps6)+strlen(ud->realname))<400) { strcat(tmps6,ud->realname); }
      strcat(tmps6,")"); strcat(tmps6,"\n");
      addtochansnapshot(tmps6);
    }
    if (isflagset(u[i].flags,um_o)) { ops++; /* Done after service-reopping! */ }
  }
  numtonick(unum,reqnick);
  numtonick(wtonum,reqfornick);
  if (didcsreop==1) {
    reqoplogentry(1,getnettime(),reqnick,reqfornick,tmps3,chansnapshot);
  }
  if (!reqonchan) {
    msgtouser(unum,"You are not on the channel you requested ops for.");
    clearchansnapshot();
    return;
  }
  if (!wtoonchan) {
    msgtouser(unum,"The nick you requested to be opped is not on that channel.");
    clearchansnapshot();
    return;
  }
  if (ops>0) {
    sprintf(tmps2,"There are ops on that channel. %s can only be used on channels that have lost all ops.",MODNAM);
    clearchansnapshot();
    msgtouser(unum,tmps2);
    return;
  }
  if (users>1) { /* Ask chanfix */
    int reopres;
    reopres=doreop(c);
    if (reopres==REOP_OPPED) {
      msgtouser(unum,"Chanfix found a regular Op and opped it.");
      return;
    }
    if ((reopres==REOP_NOINFO) || (reopres==REOP_NOREGOPS)) {
      msgtouser(unum,"Chanfix knows no regular Ops for this channel, so you got opped.");
      reqoplogentry(7,getnettime(),reqnick,reqfornick,tmps3,chansnapshot);
      longtotoken(wtonum,tmps2,5);
      fprintf(sockout,"%s M %s +o %s\r\n",servernumeric,tmps3,tmps2);
      changechanmod2(c,wtonum,1,um_o);
      clearchansnapshot();
      sprintf(tmps2,"Opped %s on %s.",reqfornick,tmps3);
      msgtouser(unum,tmps2);
      return;
    }
    if (reopres==REOP_NOMATCH) {
      msgtouser(unum,"Chanfix knows regular Ops on this channel, but they currently aren't there.");
      msgtouser(unum,"It will op them when they return. Until then, no Ops will be given.");
      return;
    }
    msgtouser(unum,"Chanfix does not know what to do with this channel.");
    msgtouser(unum,"This is some internal error. Try again later.");
  } else { /* WTF is he requesting Ops if he is the only user? */
    msgtouser(unum,"You are the only user on this channel, just rejoin to regain Ops.");
  }
}

void doreqopsearchlog(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], escsstr[TMPSSIZE];
  int extsearch=0; char *query; char *c; MYSQL_RES *myres; MYSQL_ROW myrow; int i;
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res!=2) {
    if ((res!=3) || (strcmp(tmps4,"-f")!=0)) {
      msgtouser(unum,"Syntax: reqopsearchlog pattern [-f]");
      msgtouser(unum,"Example: reqopsearchlog #ftp    or    reqopsearchlog *speednet*");
      msgtouser(unum,"The search will be performed on the nick- and channelfield of the log.");
      msgtouser(unum,"If you give the -f parameter, the channelsnapshot will be searched too. (*S-L-O-W*)");
      return;
    }
  }
  if (res==3) {
    if (strcmp(tmps4,"-f")==0) {
      extsearch=1;
    } else {
      msgtouser(unum,"invalid parameter");
      return;
    }
  }
  query=(char *)malloc(strlen(tmps3)+2000);
  c=&tmps3[0];
  while (*c!='\0') { if (*c=='*') { *c='%'; } c++; }
  mysql_escape_string(escsstr,tmps3,strlen(tmps3));
  sprintf(query,"SELECT * FROM %s WHERE requester LIKE '%s' OR reqfor LIKE '%s' OR chan LIKE '%s'",LOGTOTABLE,escsstr,escsstr,escsstr);
  if (extsearch==1) {
    sprintf(tmps2," OR snapshot LIKE '%s'",escsstr);
    strcat(query,tmps2);
  }
  strcat(query," ORDER BY timestamp DESC");
  res=mysql_query(&sqlcon,query);
  free(query);
  if (res!=0) {
    msgtouser(unum,"For some reason the database-query failed. Try again later.");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=6) {
    msgtouser(unum,"Sorry, the log in the database has illegal format");
    mysql_free_result(myres);
    return;
  }
  i=0;
  while ((myrow=mysql_fetch_row(myres))!=NULL) {
    time_t ts; struct tm *loctime; char tmps5[TMPSSIZE]; char *d; long lt;
    if (i>0) { msgtouser(unum,"----------"); }
    ts=strtol(myrow[1],NULL,10);
    loctime=localtime(&ts);
    strftime(tmps5,100,"%A, %d.%m.%Y %T",loctime);
    lt=strtol(myrow[0],NULL,10);
    if ((lt<1) || (lt>5)) {
      sprintf(tmps2,"%-20s Unknown (%s)","Type:",myrow[0]);
    } else {
      sprintf(tmps2,"%-20s %s (%s)","Type:",logtypes[lt],myrow[0]);
    }
    msgtouser(unum,tmps2);
    sprintf(tmps2,"%-20s %s","Channel:",myrow[4]);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"%-20s %s","Date:",tmps5);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"%-20s %s","Requested by:",myrow[2]);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"%-20s %s","Requested for:",myrow[3]);
    msgtouser(unum,tmps2);
    msgtouser(unum,"Snapshot of channel at the time of the request:");
    c=&myrow[5][0]; d=&tmps2[0];
    while (*c!='\0') {
      if (*c=='\n') {
        *d='\0';
        msgtouser(unum,tmps2);
        d=&tmps2[0];
      } else {
        *d=*c; d++;
      }
      c++;
    }
    *d='\0';
    if (strlen(tmps2)>0) { msgtouser(unum,tmps2); }
    i++;
    if (i>100) { msgtouser(unum,"--- Too many matches - List truncated"); break; }
  }
  sprintf(tmps2,"--- End of list - %d matches ---",i);
  msgtouser(unum,tmps2);
  mysql_free_result(myres);
}

void doreqoppurge(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE]; long dur;
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res!=2) {
    msgtouser(unum,"Syntax: reqoppurge time");
    msgtouser(unum,"Where time is in the same format that e.g. the gline-command uses.");
    msgtouser(unum,"Example: reqoppurge 1y  - that would purge all logentries older than 1 year");
    return;
  }
  dur=durationtolong(tmps3);
  if (dur<3600) {
    msgtouser(unum,"Not a valid duration. (has to be at least 1 hour)");
    return;
  }
  dur=getnettime()-dur;
  if ((dur<0) || (dur>getnettime())) { /* Must have overflowed */
    msgtouser(unum,"Not a valid duration. (has to be at least 1 hour)");
    return;
  }
  sprintf(tmps4,"DELETE FROM %s WHERE timestamp<%ld",LOGTOTABLE,dur);
  mysql_query(&sqlcon,tmps4);
  msgtouser(unum,"Should be done.");
}

/* This is called upon initialization */
void requestop_init() {
  setmoduledesc(MODNAM,"Functionality to request op on opless channels");
  registercommand2(MODNAM,"requestop", dorequestop, 0, 0,
  "requestop\tRequests op on an opless channel");
  registercommand2(MODNAM,"reqopsearchlog", doreqopsearchlog, 1, 990,
  "reqopsearchlog pattern\tSearches the logs for things matching pattern");
  registercommand2(MODNAM,"reqoppurge", doreqoppurge, 1, 998,
  "reqoppurge time\tRemoves all logentries that are older than time");
  sqlcreatedone=0;
}

/* This is called for cleanup (before the module gets unloaded) */
void requestop_cleanup() {
  deregistercommand("requestop");
  deregistercommand("reqopsearchlog");
  deregistercommand("reqoppurge");
}
