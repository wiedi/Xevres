/*
Xevres (based on Operservice 2) - trusts.c
Manages Trusts for hosts or groups of hosts
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
#include "globals.h"

array deniedtrusts; /* List of denied trusts */
trustedhost * trustedhosts[SIZEOFTL];       // The trusted hosts
//array untrustedhosts[SIZEOFHL];     // normal, untrusted hosts
trustedgroup * trustedgroups[SIZEOFIDMAP];
int glineonclones=0;

//unsigned int hlhash(char *host) {
//  return (unsigned int)(crc32(host)%SIZEOFHL);
//}

unsigned int tlhash(unsigned long IPv4) {
  return (unsigned int)(IPv4%SIZEOFTL);
}

unsigned int tgshash(unsigned long ID) {
  return (unsigned int)(ID%SIZEOFIDMAP);
}

/* Returns 0 if host isn't trusted, or the ID of the trusted host group */
unsigned long istrusted(unsigned long IPv4) {
  trustedhost *a;
  a=trustedhosts[tlhash(IPv4)];
  while (a!=NULL) {
    if (IPv4==a->IPv4) { return a->id; }
    a=a->next;
  }
  return 0;
}

/* updates counters of a trusted host */
void updatetrustedhost(unsigned long IPv4, int modifier) {
  trustedhost *a;
  a=trustedhosts[tlhash(IPv4)];
  while (a!=NULL) {
    if (IPv4==a->IPv4) {
      a->lastused=getnettime();
      a->currentlyon+=modifier;
      if (a->currentlyon > a->maxused) { a->maxused=a->currentlyon; }
      return;
    }
    a=a->next;
  }
  return;
}

void addtotrustedhost(char *ident, unsigned long id) {
  trustedgroup *tg; char tmps2[TMPSSIZE]; unsigned long i; identcounter *ic;
  tg=findtrustgroupbyID(id);
  if (tg==NULL) { return; }
  tg->currentlyon++;
  tg->lastused=getnettime();
  if (tg->currentlyon>tg->maxused) { tg->maxused=tg->currentlyon; }
  if (tg->currentlyon > tg->trustedfor) {
    sprintf(tmps2,"Trustgroup %s exceeding trustlimit - %lu / %lu",tg->name,tg->currentlyon,tg->trustedfor);
    if (burstcomplete!=0) { noticeallircops(tmps2); }
    putlog("[trustexceeded] %s",tmps2);
  }
  if (strlen(ident)>USERLEN) {
    putlog("ERROR: IDENT with length >USERLEN received (%s)! Fix USERLEN!",ident);
    return;
  }
  ic=(identcounter *)(tg->identcounts.content);
  for (i=0;i<tg->identcounts.cursi;i++) {
    if (strcmp(ic[i].ident,ident)==0) {
      ic[i].currenton++;
      if ((ic[i].currenton > tg->maxperident) && (tg->maxperident!=0)) {
        sprintf(tmps2,"User %s @ Group %s has excessive connections (%lu / %lu)",ident,tg->name,ic[i].currenton,tg->maxperident);
        putlog("[excon] %s",tmps2);
        if (burstcomplete!=0) { noticeallircops(tmps2); }
      }
      return;
    }
  }
  i=array_getfreeslot(&(tg->identcounts));
  ic=(identcounter *)(tg->identcounts.content);
  ic[i].currenton=1;
  strcpy(ic[i].ident,ident);
}

void delfromtrustedhost(char *ident, unsigned long id) {
  trustedgroup *tg; unsigned long i; identcounter *ic;
  tg=findtrustgroupbyID(id);
  if (tg==NULL) { return; }
  tg->currentlyon--;
  tg->lastused=getnettime();
  if (strlen(ident)>USERLEN) { return; }
  ic=(identcounter *)(tg->identcounts.content);
  for (i=0;i<tg->identcounts.cursi;i++) {
    if (strcmp(ic[i].ident,ident)==0) {
      ic[i].currenton--;
      if (ic[i].currenton==0) {
        array_delslot(&(tg->identcounts),i);
      }
      return;
    }
  }
}

void trustnewclient(char *ident, unsigned long IPv4) {
  unsigned long thid;
  if (strlen(ident)>USERLEN) { return; }
  /* First check if this host is trusted */
  thid=istrusted(IPv4);
  if (thid!=0) { /* That host is trusted */
    addtotrustedhost(ident, thid);
    updatetrustedhost(IPv4,+1);
  }
}

void trustdelclient(char *ident, unsigned long IPv4) {
  unsigned long thid;
  if (strlen(ident)>USERLEN) { return; }
  /* First check if this host is trusted */
  thid=istrusted(IPv4);
  if (thid!=0) { /* That host is trusted */
    delfromtrustedhost(ident, thid);
    updatetrustedhost(IPv4,-1);
  }
}

trustedgroup * findtrustgroupbyID(unsigned long ID) {
  trustedgroup *tmp;
  tmp=trustedgroups[tgshash(ID)];
  while (tmp!=NULL) {
    if (ID==tmp->id) {
      return tmp;
    }
    tmp=tmp->next;
  }
  return NULL;
}

trustedgroup * findtrustgroupbyname(char *name) {
  trustedgroup *tmp; int i;
  for (i=0;i<SIZEOFIDMAP;i++) {
    tmp=trustedgroups[i];
    while (tmp!=NULL) {
      if (strcmp(name,tmp->name)==0) {
        return tmp;
      }
      tmp=tmp->next;
    }
  }
  return NULL;
}

unsigned long getfreetgid() {
  unsigned long id; trustedgroup *tg;
  id=0;
  do {
    id++;
    tg=findtrustgroupbyID(id);
  } while (tg!=NULL);
  return id;
}

int destroytrustgroup(unsigned long id) {
  array listofhosts; trustedhost *th; long i, j; trustedhost *th2, *th3;
  trustedgroup *tg1, *tg2, *tg3;
  /* Not only will we have to delete the trustgroup, we will also have to recount
     all individual hosts in it. So lets start with building a list of all hosts */
  array_init(&listofhosts,sizeof(unsigned long));
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    th2=NULL;
    while (th!=NULL) {
      if (th->id==id) {
        /* This host belongs to our trustgroup */
        long tmpl=array_getfreeslot(&listofhosts);
        ((unsigned long *)listofhosts.content)[tmpl]=th->IPv4;
        if (th2==NULL) {
          trustedhosts[i]=th->next;
        } else {
          th2->next=th->next;
        }
        th3=th;
        th=th->next;
        free(th3);
      } else {
        th2=th;
        th=th->next;
      }
    }
  }
  /* we now have the list in array listofhosts, and the hosts deleted from trustedhosts */
  /* Goodbye trustgroup */
  tg1=trustedgroups[tgshash(id)]; tg2=NULL;
  while (tg1!=NULL) {
    if (tg1->id==id) { /* Kill it! */
      array_free(&(tg1->identcounts));
      if (tg2==NULL) {
        trustedgroups[tgshash(id)]=tg1->next;
      } else {
        tg2->next=tg1->next;
      }
      tg3=tg1;
      tg1=tg1->next;
      free(tg3);
    } else {
      tg2=tg1;
      tg1=tg1->next;
    }
  }
  for (j=0;j<listofhosts.cursi;j++) {
    int currenton=sncget(((unsigned long *)listofhosts.content)[j],32);
    if (currenton>=mf4warn[32]) { /* Clonelimit for that host exceeded */
      if (mf4warn[32]>0) {
        char tmps2[TMPSSIZE]; char * mycip=printipv4(((unsigned long *)listofhosts.content)[j]);
        sprintf(tmps2,"[%d] clones detected from (previously trusted IP) %s",currenton,mycip);
        noticeallircops(tmps2);
        putlog("[clones] %d clones detected from (now) untrusted IP %s",currenton,mycip);
        free(mycip);
      }
    }
 }
  i=listofhosts.cursi;
  array_free(&listofhosts);
  return i;
}

/* Trusts have to be loaded BEFORE linking to the net.
   this function returns the number of hosts loaded from the database. */
unsigned long loadtrusts() {
  int res; MYSQL_RES *myres; MYSQL_ROW myrow; unsigned long hostcounter=0;
  int convertold=0; int convold2=0;
  res=mysql_query(&sqlcon,"SELECT * FROM trustgroups");
  if (res!=0) {
    putlog("!!! Failed to query trustgroups from database !!!");
    return 0;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=12) {
    if (mysql_num_fields(myres)!=9) {
      putlog("!!! trustgroups in database have invalid format !!!");
      return 0;
    } else { /* Convert old data */
      putlog("OLDSTYLE trustgroups format in database, trying to convert!");
      convold2=1;
    }
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if ((strlen(myrow[1])>TRUSTNAMELEN) || (strlen(myrow[6])>TRUSTCONTACTLEN) ||
        (strlen(myrow[7])>TRUSTCOMLEN) || (strlen(myrow[8])>AUTHUSERLEN)) {
      putlog("!!! Malformed trustgroups-entry in database! !!!");
    } else {
      if (strtol(myrow[0],NULL,10)==0) {
        putlog("!!! Trustgroups-ID of 0 in database! !!!");
      } else {
        trustedgroup *tg; unsigned int thehash;
        tg=malloc(sizeof(trustedgroup));
        tg->id=strtol(myrow[0],NULL,10);
        strcpy(tg->name,myrow[1]);
        tg->trustedfor=strtoul(myrow[2],NULL,10);
        tg->expires=strtoul(myrow[3],NULL,10);
        tg->maxperident=strtol(myrow[4],NULL,10);
        tg->enforceident=strtol(myrow[5],NULL,10);
        strcpy(tg->contact,myrow[6]);
        strcpy(tg->comment,myrow[7]);
        strcpy(tg->creator,myrow[8]);
        if (convold2==1) {
          tg->lastused=0; tg->maxused=0; tg->maxreset=0;
        } else {
          tg->lastused=strtoul(myrow[9],NULL,10);
          tg->maxused=strtoul(myrow[10],NULL,10);
          tg->maxreset=strtoul(myrow[11],NULL,10);
        }
        array_init(&(tg->identcounts),sizeof(identcounter));
        tg->currentlyon=0;
        thehash=tgshash(tg->id);
        tg->next=trustedgroups[thehash];
        trustedgroups[thehash]=tg;
        hostcounter++;
      }
    }
  }
  mysql_free_result(myres);
  if (convold2==1) {
    mysql_query(&sqlcon,"DROP TABLE trustgroups");
    mysql_query(&sqlcon,"CREATE TABLE trustgroups (ID BIGINT not null , name VARCHAR (70) not null , trustedfor INT not null , expires BIGINT not null , maxperident INT not null , enforceident INT not null , contact VARCHAR (150) not null , comment VARCHAR (200) not null , creator VARCHAR(40) not null , lastused BIGINT not null , maxused BIGINT not null , maxreset BIGINT not null , PRIMARY KEY (ID), UNIQUE (ID))");
  }
  hostcounter=hostcounter<<16;
  res=mysql_query(&sqlcon,"SELECT * FROM trustedhosts");
  if (res!=0) {
    putlog("!!! Failed to query trustedhosts from database !!!");
    return hostcounter;
  }
//  CREATE TABLE trustedhosts (hostname VARCHAR (100) not null , groupid BIGINT not null , PRIMARY KEY (hostname), UNIQUE (hostname))
//  CREATE TABLE trustedhosts (hostname VARCHAR (16) not null , groupid BIGINT not null , lastused BIGINT not null , maxused BIGINT not null , maxreset BIGINT not null , PRIMARY KEY (hostname), UNIQUE (hostname))
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=5) {
    if (mysql_num_fields(myres)!=2) {
      putlog("!!! trustedhosts in database have invalid format !!!");
      return hostcounter;
    } else { /* This is an oldstyle-database, we need to convert it! */
      putlog("OLDSTYLE trustedhosts format in database, trying to convert!");
      convertold=1;
    }
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if (strlen(myrow[0])>HOSTLEN) {
      putlog("!!! Malformed trustedhosts-entry in database! !!!");
    } else {
      trustedgroup *tg; unsigned int thehash; trustedhost *th;
      tg=findtrustgroupbyID(strtol(myrow[1],NULL,10));
      if (tg==NULL) {
        putlog("!!! trustedhosts: Referential integrity violated! (no group #%ld)",strtol(myrow[1],NULL,10));
      } else {
        th=malloc(sizeof(trustedhost));
        if (!isvalidipv4(myrow[0])) { // oldstyle: This is a hostNAME, try to convert it to an IP
          struct hostent * he;
          putlog("Trying to convert hostNAME to IP: %s",myrow[0]);
          he=gethostbyname(myrow[0]);
          if (he==NULL) {
            putlog("CONVERSION FAILED! - could not resolve host");
            free(th); continue;
          } else {
            if ((he->h_length!=4) || (he->h_addrtype!=AF_INET)) {
              putlog("CONVERSION FAILED! - unknown addresstype");
              free(th); continue;
            } else {
              char * mycip;
              th->IPv4=ntohl(*((uint32_t *)he->h_addr));
              mycip=printipv4(th->IPv4);
              putlog("Succeeded. %s => %s",myrow[0],mycip);
              free(mycip);
            }
          }
        } else {
          th->IPv4=parseipv4(myrow[0]);
        }
        th->id=strtol(myrow[1],NULL,10);
        if (convertold==1) {
          th->maxused=0; th->maxreset=0; th->lastused=0; th->currentlyon=0;
        } else {
          th->lastused=strtol(myrow[2],NULL,10);
          th->maxused=strtol(myrow[3],NULL,10);
          th->maxreset=strtol(myrow[4],NULL,10);
          th->currentlyon=0;
        }
        thehash=tlhash(th->IPv4);
        th->next=trustedhosts[thehash];
        trustedhosts[thehash]=th;
        hostcounter++;
      }
    }
  }
  mysql_free_result(myres);
  trustgroupexpire();
  if (convertold==1) {
    mysql_query(&sqlcon,"DROP TABLE trustedhosts");
    mysql_query(&sqlcon,"CREATE TABLE trustedhosts (hostname VARCHAR (100) not null , groupid BIGINT not null , lastused BIGINT not null , maxused BIGINT not null , maxreset BIGINT not null , PRIMARY KEY (hostname), UNIQUE (hostname))");
  }
  if ((convertold==1) || (convold2==1)) { savetrusts(); }
  recreateimpsntrusts();
  return hostcounter;
}

void savetrusts() {
  char sqlquery[5000]; long i; trustedgroup *tg; trustedhost *th;
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  char *mycip;
  /* No need to write expired data, so expire old shit first */
  trustgroupexpire();
  /* Clear the tables */
  mysql_query(&sqlcon,"DELETE FROM trustgroups");
  mysql_query(&sqlcon,"DELETE FROM trustedhosts");
  for (i=0;i<SIZEOFIDMAP;i++) {
    tg=trustedgroups[i];
    while (tg!=NULL) {
      mysql_escape_string(tmps2,tg->name,strlen(tg->name));
      mysql_escape_string(tmps3,tg->contact,strlen(tg->contact));
      mysql_escape_string(tmps4,tg->comment,strlen(tg->comment));
      mysql_escape_string(tmps5,tg->creator,strlen(tg->creator));
      sprintf(sqlquery,"INSERT INTO trustgroups VALUES(%lu, '%s', %lu, %lu, %lu, %d, '%s', '%s', '%s', %lu, %lu, %lu)",
        tg->id,tmps2,tg->trustedfor,tg->expires,tg->maxperident,tg->enforceident,tmps3,tmps4,tmps5,tg->lastused,tg->maxused,tg->maxreset);
      mysql_query(&sqlcon,sqlquery);
      tg=tg->next;
    }
  }
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      mycip=printipv4(th->IPv4);
      mysql_escape_string(tmps2,mycip,strlen(mycip));
      free(mycip);
      sprintf(sqlquery,"INSERT INTO trustedhosts VALUES('%s', %lu, %lu, %lu, %lu)",tmps2,th->id,th->lastused,th->maxused,th->maxreset);
      mysql_query(&sqlcon,sqlquery);
      th=th->next;
    }
  }
}

void trustgroupexpire() {
  long i; trustedgroup *tg; unsigned long j; char tmps2[TMPSSIZE];
  for (i=0;i<SIZEOFIDMAP;i++) {
    tg=trustedgroups[i];
    while (tg!=NULL) {
      if (tg->expires<getnettime()) {
        if (burstcomplete==1) {
          sprintf(tmps2,"Trust expired for trustgroup %s",tg->name);
          sendtonoticemask(NM_TRUSTS,tmps2);
        }
        j=tg->id;
        tg=tg->next;
        destroytrustgroup(j);
      } else {
        tg=tg->next;
      }
    }
  }
}

trustdeny * gettrustdeny(unsigned long IP) {
  trustdeny *td; long i;
  td=(trustdeny *)deniedtrusts.content;
  for (i=0;i<deniedtrusts.cursi;i++) {
    if ((td[i].v4net&netmasks[td[i].v4mask])==(IP&netmasks[td[i].v4mask])) {
      return &td[i];
    }
  }
  return NULL;    
}

/* this function returns the number of deniedtrusts loaded from the database. */
unsigned long loadtrustdeny() {
  int res; MYSQL_RES *myres; MYSQL_ROW myrow;
  res=mysql_query(&sqlcon,"SELECT * FROM trustdenys");
  if (res!=0) {
    putlog("!!! Failed to query trustdenys from database !!!");
    return 0;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=7) {
    if (mysql_num_fields(myres)==6) {
      putlog("!!! _OLDSTYLE_ trustdeny table found in database! There is no way to convert this, please delete this table from the database and restart Operservice, otherwise trustdenys just WON'T WORK !!!");
    } else {
      putlog("!!! trustdenys in database have invalid format !!!");
    }
    mysql_free_result(myres);
    return 0;
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if ((strlen(myrow[0])>39) || (strlen(myrow[2])>AUTHUSERLEN) ||
        (strlen(myrow[3])>RNGREAS)) {
      putlog("!!! Malformed trustdeny-entry in database! !!!");
    } else {
      /* FIX ME (needs more sanity checks) */
      long i; trustdeny * td;
      i=array_getfreeslot(&deniedtrusts);
      td=(trustdeny *)deniedtrusts.content;
      td[i].v4net=parseipv4(myrow[0]);
      td[i].v4mask=strtoul(myrow[1],NULL,10);
      mystrncpy(td[i].creator,myrow[2],AUTHUSERLEN);
      mystrncpy(td[i].reason,myrow[3],RNGREAS);
      td[i].expires=strtoul(myrow[4],NULL,10);
      td[i].created=strtoul(myrow[5],NULL,10);
      if (strtol(myrow[6],NULL,10)==1) {
        td[i].type=TRUSTDENY_DENY;
      } else {
        td[i].type=TRUSTDENY_WARN;
      }
    }
  }
  mysql_free_result(myres);
  trustdenyexpire();
  return 0;
}

void savetrustdeny() {
  char sqlquery[4*TMPSSIZE+500]; long i; trustdeny *td;
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE]; char * mycip;
  /* No need to write expired data, so expire old shit first */
  trustdenyexpire();
  /* Clear the tables */
  mysql_query(&sqlcon,"DELETE FROM trustdenys");
  td=(trustdeny *)deniedtrusts.content;
  for (i=0;i<deniedtrusts.cursi;i++) {
    mycip=printipv4(td[i].v4net);
    mysql_escape_string(tmps2,mycip,strlen(mycip));
    free(mycip);
    mysql_escape_string(tmps3,td[i].creator,strlen(td[i].creator));
    mysql_escape_string(tmps4,td[i].reason,strlen(td[i].reason));
    sprintf(sqlquery,"INSERT INTO trustdenys VALUES('%s','%u','%s','%s','%lu','%lu','%d')",
      tmps2,td[i].v4mask,tmps3,tmps4,td[i].expires,td[i].created,td[i].type);
    mysql_query(&sqlcon,sqlquery);
  }
}

void trustdenyexpire() {
  long i; trustdeny * td;
  for (i=0;i<deniedtrusts.cursi;i++) {
    td=(trustdeny *)deniedtrusts.content;
    if ((td[i].expires!=0) && (td[i].expires<getnettime())) {
      array_delslot(&deniedtrusts,i);
      i--;
    }
  }
}
