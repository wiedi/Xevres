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

/* Fakeuser-Functions */

void fakeuseradd(char *nick, char *ident, char *host, char *realname, long numeric) {
  unsigned long i; afakeuser *tmpp;
  tmpp=((afakeuser *)(myfakes.content));
  for (i=0;i<myfakes.cursi;i++) {
    if (strcmp(nick,tmpp[i].nick)==0) {
      strcpy(tmpp[i].nick,nick);
      strcpy(tmpp[i].ident,ident);
      strcpy(tmpp[i].host,host);
      strcpy(tmpp[i].realname,realname);
      tmpp[i].numeric=numeric;
      tmpp[i].connectat=getnettime();
      return;
    }
  }
  i=array_getfreeslot(&myfakes);
  tmpp=((afakeuser *)(myfakes.content));
  strcpy(tmpp[i].nick,nick);
  strcpy(tmpp[i].ident,ident);
  strcpy(tmpp[i].host,host);
  strcpy(tmpp[i].realname,realname);
  tmpp[i].numeric=numeric;
  tmpp[i].connectat=getnettime();
}

void fakeuserdel(long numeric) {
  unsigned long i;
  for (i=0;i<myfakes.cursi;i++) {
    if (((afakeuser *)(myfakes.content))[i].numeric==numeric) {
      array_delslot(&myfakes,i);
      return;
    }
  }
}

void fakeuserkill(long numeric) {
  unsigned long i; char tmps2[TMPSSIZE]; char tnick[NICKLEN+1]; char tident[USERLEN+1];
  char thost[HOSTLEN+1]; char treal[REALLEN+1]; afakeuser *tmpp;
  tmpp=((afakeuser *)(myfakes.content));
  for (i=0;i<myfakes.cursi;i++) {
    if (tmpp[i].numeric==numeric) {
      if (tmpp[i].connectat+10>getnettime()) {
        /* Killed within 10 seconds - this is not good! */
        sprintf(tmps2,"Warning: Fakeuser %s got killed less than 10 seconds after creation. NOT recreating it.",tmpp[i].nick);
        putlog("%s",tmps2);
        fakeuserdel(numeric);
        noticeallircops(tmps2);
        return;
      } else {
        long fakenum;
        strcpy(tnick,tmpp[i].nick);
        strcpy(tident,tmpp[i].ident);
        strcpy(thost,tmpp[i].host);
        strcpy(treal,tmpp[i].realname);
        fakeuserdel(numeric);
        fakenum=getfreefakenum();
        if (fakenum!=-1) {
          fakenum+=tokentolong(servernumeric)<<SRVSHIFT;
          createfakeuser(fakenum,tnick,tident,thost,treal,1);
        } else {
          putlog("Warning: Ran out of numerics for fakeusers!");
        }
        return;
      }
    }
  }
}

void fakeuserload() {
  int res;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  res=mysql_query(&sqlcon,"SELECT * FROM fakeusers");
  if (res!=0) {
    putlog("!!! Failed to query fakeuser-list from database !!!");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=4) {
    putlog("!!! Fakeuser-List in database has invalid format !!!");
    return;
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if ((strlen(myrow[0])>NICKLEN) || (strlen(myrow[1])>USERLEN) || (strlen(myrow[2])>HOSTLEN) || (strlen(myrow[3])>REALLEN)) {
      putlog("!!! Malformed fakeuser-entry in database! !!!");
    } else {
      long fakenum; long x1; userdata *a, *b; char fooker[10]; char tmps2[TMPSSIZE];
      strcpy(tmps2,myrow[0]); toLowerCase(tmps2);
      x1=nicktonu2(tmps2);
      if (x1!=-1) {
        longtotoken(x1,fooker,5);
        deluserfromallchans(x1);
        delautheduser(x1);
        a=uls[ulhash(x1)]; b=NULL;
        while (a!=NULL) {
          if (a->numeric==x1) {
            sncdel(a->realip);
            rnldel(a->realname);
            delfromnicklist(a->nick);
            trustdelclient(a->ident,a->realip);
            if (b==NULL) {
              uls[ulhash(x1)]=(void *)a->next;
            } else {
              b->next=a->next;
            }
            array_free(&(a->chans));
            free(a);
            break;
          }
          b=a;
          a=(void *)a->next;
        }
        sendtouplink("%s D %s :that nick is reserved\r\n",servernumeric,fooker);
        fflush(sockout);
      }
      fakenum=getfreefakenum();
      if (fakenum!=-1) {
        fakenum+=tokentolong(servernumeric)<<SRVSHIFT;
        createfakeuser(fakenum,myrow[0],myrow[1],myrow[2],myrow[3],1);
      } else {
        putlog("Warning: Ran out of numerics for fakeusers!");
      }
    }
  }
  mysql_free_result(myres);
}

void fakeusersave() {
  char query[TMPSSIZE]; int i; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  char tmps4[TMPSSIZE], tmps5[TMPSSIZE]; afakeuser *tmpp;
  tmpp=((afakeuser *)(myfakes.content));
  mysql_query(&sqlcon,"DROP TABLE IF EXISTS fakeusers");
  mysql_query(&sqlcon,"CREATE TABLE fakeusers (nick VARCHAR (40) not null , ident VARCHAR (40) not null , host VARCHAR (200) not null , realname VARCHAR (200) not null , PRIMARY KEY (nick), INDEX (nick), UNIQUE (nick))");
  for (i=0;i<myfakes.cursi;i++) {
    if ((tmpp[i].numeric % SRVNUMMULT)<10) {
      /* this one is not automatically added! */
      continue;
    }
    mysql_escape_string(tmps2,tmpp[i].nick,strlen(tmpp[i].nick));
    mysql_escape_string(tmps3,tmpp[i].ident,strlen(tmpp[i].ident));
    mysql_escape_string(tmps4,tmpp[i].host,strlen(tmpp[i].host));
    mysql_escape_string(tmps5,tmpp[i].realname,strlen(tmpp[i].realname));
    sprintf(query,"INSERT INTO fakeusers VALUES('%s','%s','%s','%s')",
      tmps2,tmps3,tmps4,tmps5);
    mysql_query(&sqlcon,query);
  }
}

int clinuminuse(long senu, long clnu) {
  userdata *a; int i;
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      if (a->numeric==(senu<<SRVSHIFT)+clnu) { return 1; }
      a=(void *)a->next;
    }
  }
  return 0;
}

long getfreefakenum() {
  long a;
  a=lastfakenum;
  do {
    if (!clinuminuse(tokentolong(servernumeric),a)) { lastfakenum=a; return a; }
    a++;
    if (a>4000) { a=10; }
  } while (a!=lastfakenum);
  return -1;
}

void createfakeuser(long num, char *nick, char *ident, char *host, char *realname, int sendtonet) {
  userdata *a;
  if (strlen(nick)>NICKLEN) { return; }
  if (strlen(ident)>USERLEN) { return; }
  if (strlen(host)>HOSTLEN) { return; }
  if (strlen(realname)>REALLEN) { return; }
  a=(userdata *)malloc(sizeof(userdata));
  array_init(&(a->chans),sizeof(channel *));
  a->next=NULL;
  a->hopsaway=0;
  a->connectat=getnettime()-120;
  a->realip=iptolong(127,0,(num%SRVNUMMULT)/255,((num%SRVNUMMULT)%255)+1);
  a->numeric=num;
  strcpy(a->nick,nick);
  toLowerCase(a->nick);
  strcpy(a->ident,ident);
  a->host=getastring(host);
  strcpy(a->umode,"");
  a->realname=getastring(realname);
  addnicktoul(a);
  addtonicklist(a);
  sncadd(a->realip,NULL);
  rnladd(realname);
  trustnewclient(a->ident,a->realip);
  if (sendtonet!=0) {
    char myip[500], fakenum[500];
    longtotoken(a->realip,myip,6);
    longtotoken(num,fakenum,5);
    sendtouplink("%s N %s 0 %ld %s %s %s %s :%s\r\n",servernumeric,nick,getnettime()-120,ident,host,myip,fakenum,realname);
    fflush(sockout);
  }
  strcpy(a->authname,"");
  setmd5(a);
  fakeuseradd(nick,ident,host,realname,num);
}

void dofakelist(long unum, char *tail) {
  unsigned long i; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  afakeuser *tmpp;
  if (myfakes.cursi==0) {
    msgtouser(unum,"Currently no Fakeusers.");
    return;
  }
  tmpp=((afakeuser *)(myfakes.content));
  sprintf(tmps2,"%-10s %-70s","Numeric","nick!ident@host (Realname)");
  msgtouser(unum,tmps2);
  for (i=0;i<myfakes.cursi;i++) {
    longtotoken(tmpp[i].numeric,tmps2,5);
    sprintf(tmps3,"%s!%s@%s (%s)",tmpp[i].nick,tmpp[i].ident,tmpp[i].host,tmpp[i].realname);
    sprintf(tmps4,"%-10s %-70s",tmps2,tmps3);
    msgtouser(unum,tmps4);
  }
  msgtouser(unum,"--- End of list ---");
}

void dofakekill(long unum, char *tail) {
  unsigned long i; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int res;
  afakeuser *tmpp;
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res!=2) {
    msgtouser(unum,"Syntax: fakekill nickname");
    return;
  }
  toLowerCase(tmps3);
  tmpp=((afakeuser *)(myfakes.content));
  for (i=0;i<myfakes.cursi;i++) {
    strcpy(tmps2,tmpp[i].nick);
    toLowerCase(tmps2);
    if (strcmp(tmps2,tmps3)==0) {
      userdata *a, *b;
      if ((tmpp[i].numeric % SRVNUMMULT)<10) {
        msgtouser(unum,"eeeeer... wait a minute, you cannot kill my primary nickname!");
        return;
      }
      longtotoken(tmpp[i].numeric,tmps2,5);
      sendtouplink("%s Q :\r\n",tmps2);
      fflush(sockout);
      a=uls[ulhash(tmpp[i].numeric)]; b=NULL;
      while (a!=NULL) {
        if (a->numeric==tmpp[i].numeric) {
          sncdel(a->realip);
          rnldel(a->realname);
          delfromnicklist(a->nick);
          trustdelclient(a->ident,a->realip);
          if (b==NULL) {
            uls[ulhash(tmpp[i].numeric)]=(void *)a->next;
          } else {
            b->next=a->next;
          }
          freeastring(a->host);
          freeastring(a->realname);
          free(a);
          break;
        }
        b=a;
        a=(void *)a->next;
      }
      fakeuserdel(tmpp[i].numeric);
      msgtouser(unum,"Done.");
      return;
    }
  }
  msgtouser(unum,"No such fakeuser");
}

void docreatefakeuser(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  long x1; char nicklow[TMPSSIZE];
  if (!checkauthlevel(unum,900)) { return; }
  res=sscanf(tail,"%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res!=5) {
    msgtouser(unum,"Syntax: fakeuser nick ident host realname");
    return;
  }
  strcpy(nicklow,tmps3),
  toLowerCase(nicklow);
  if (strlen(nicklow)>NICKLEN) { msgtouser(unum,"That nick is too long!"); return; }
  if (strlen(tmps4)>USERLEN) { msgtouser(unum,"That ident is too long!"); return; }
  if (strlen(tmps5)>HOSTLEN) { msgtouser(unum,"That Hostname is too long!"); return; }
  if (strlen(tmps6)>REALLEN) { msgtouser(unum,"That Realname is too long!"); return; }
  x1=nicktonu2(nicklow);
  if (x1!=-1) {
    char fooker[100]; userdata *a, *b;
    if ((x1>>SRVSHIFT)==tokentolong(servernumeric)) {
      msgtouser(unum,"That fakeuser already exists!");
      return;
    }
    longtotoken(x1,fooker,5);
    msgtouser(unum,"Killing user that currently has this nick...");
    deluserfromallchans(x1);
    delautheduser(x1);
    a=uls[ulhash(x1)]; b=NULL;
    while (a!=NULL) {
      if (a->numeric==x1) {
        sncdel(a->realip);
        rnldel(a->realname);
        delfromnicklist(a->nick);
        trustdelclient(a->ident,a->realip);
        if (b==NULL) {
          uls[ulhash(x1)]=(void *)a->next;
        } else {
          b->next=a->next;
        }
        free(a);
        break;
      }
      b=a;
      a=(void *)a->next;
    }
    sendtouplink("%s D %s :that nick is reserved\r\n",servernumeric,fooker);
    fflush(sockout);
  }
  createfakeuser((tokentolong(servernumeric)<<SRVSHIFT)+getfreefakenum(),tmps3,tmps4,tmps5,tmps6,1);
  msgtouser(unum,"Fake user created");
}

void createfakeuser2(char *nick, char *ident, char *host, char *realname) {
 long x1;
 char nicklow[TMPSSIZE];
 strcpy(nicklow,nick),
 toLowerCase(nicklow);
 x1=nicktonu2(nicklow);
 if (x1!=-1) {
  char fooker[100]; userdata *a, *b;
  if ((x1>>SRVSHIFT)==tokentolong(servernumeric)) {
   return;
  }
  longtotoken(x1,fooker,5);
  deluserfromallchans(x1);
  delautheduser(x1);
  a=uls[ulhash(x1)]; b=NULL;
  while (a!=NULL) {
   if (a->numeric==x1) {
    sncdel(a->realip);
    rnldel(a->realname);
    delfromnicklist(a->nick);
    trustdelclient(a->ident,a->realip);
    if (b==NULL) {
     uls[ulhash(x1)]=(void *)a->next;
    } else {
     b->next=a->next;
    }
    free(a);
    break;
   }
   b=a;
   a=(void *)a->next;
  }
  sendtouplink("%s D %s :that nick is reserved\r\n",servernumeric,fooker);
  fflush(sockout);
 }
 createfakeuser((tokentolong(servernumeric)<<SRVSHIFT)+getfreefakenum(),nick,ident,host,realname,1);
}

void fakekill2(char *nick, char *qmsg) {
 unsigned long i; 
 static char tmps2[TMPSSIZE]; 
 afakeuser *tmpp;
 tmpp=((afakeuser *)(myfakes.content));
 for (i=0;i<myfakes.cursi;i++) {
  strcpy(tmps2,tmpp[i].nick);
  toLowerCase(tmps2);
  if (strcmp(tmps2,nick)==0) {
   userdata *a, *b;
   if ((tmpp[i].numeric % SRVNUMMULT)<10) {
    return;
   }
   longtotoken(tmpp[i].numeric,tmps2,5);
   sendtouplink("%s Q :%s\r\n",tmps2,qmsg);
   fflush(sockout);
   /* del fake from chans! */
   deluserfromallchans(tmpp[i].numeric);
   a=uls[ulhash(tmpp[i].numeric)]; b=NULL;
   while (a!=NULL) {
    if (a->numeric==tmpp[i].numeric) {
     sncdel(a->realip);
     rnldel(a->realname);
     delfromnicklist(a->nick);
     trustdelclient(a->ident,a->realip);
     if (b==NULL) {
      uls[ulhash(tmpp[i].numeric)]=(void *)a->next;
     } else {
      b->next=a->next;
     }
     freeastring(a->host);
     freeastring(a->realname);
     free(a);
     break;
    }
    b=a;
    a=(void *)a->next;
   }
   fakeuserdel(tmpp[i].numeric);
   return;
  }
 }
}

long fake2long(char *nick) {
 unsigned long i; 
 static char tmps2[TMPSSIZE]; 
 afakeuser *tmpp;
 tmpp=((afakeuser *)(myfakes.content));
 for (i=0;i<myfakes.cursi;i++) {
  strcpy(tmps2,tmpp[i].nick);
  toLowerCase(tmps2);
  if (strcmp(tmps2,nick)==0) {
   return tmpp[i].numeric;
  }
 }
 return 0;
}   

/* End of Fakeuser-Functions */
