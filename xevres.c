/*
Xevres (based on Operservice 2)
(C) Michael Meier 2000-2001 - released under GPL
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
#include "mysql_version.h"
#include "mysql.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

char dummy[20];
serverdata *sls;
userdata * uls[SIZEOFUL]; /* Userlist */
channel * cls[SIZEOFCL];  /* Channellist */
array nls[SIZEOFNL];      /* Nicklist */
array reg[SIZEOFCL];      /* Registered channel list */
autheduser *als;  /* List of authed users */
flags chanflags[]={
  {'p',cm_p},
  {'s',cm_s},
  {'m',cm_m},
  {'n',cm_n},
  {'t',cm_t},
  {'i',cm_i},
  {'l',cm_l},
  {'k',cm_k},
  {'c',cm_c},
  {'C',cm_C},
  {'D',cm_D},
  {'r',cm_r},
  {'u',cm_u},
  {'O',cm_O},
  {'F',cm_F}
};
int numcf; /* Number of Chanflags */
flags userflags[]={
  {'o',um_o},
  {'v',um_v}
};
int numuf; /* Number of Userflags (Flags of a user on a channel) */
FILE *sockin, *sockout;
char lastline[520]; /* Last line received from the ircserver */
const char tokens[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
const unsigned int reversetok[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,
  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62, 0,  63, 0,  0,  0,  26, 27, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
const char crytok[]="ABCDEFGHIJKLMNOPQ/RSTUVWXYZabcdefgh.ijklmnopqrstuvwxyz0123456789";
const char upwdchars[]="ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789.!";
char tmps1[TMPSSIZE];
char command[TMPSSIZE];
int weareinvisible;
char conpass[100]="yeahsure";  // These are compile-time defaults only, you
char mynick[NICKLEN+1]="X";          // should not change them, use a configfile
char myname[100]="xevres.xchannel.org";  // instead.
char conto[100]="hub2.xchannel.org";
unsigned int conport=4400;
char sqluser[100]="xevres";
char sqlpass[100]="yeahsure";
char sqlhost[100]="localhost";
char sqldb[100]="xevres";
int sqlport=MYSQL_PORT;
int stayhere=0;
char servernumeric[3];
char uplnum[3]="";
char lastburstchan[520]="";
int lastburstflags=0;
long starttime;  // Timestamp, contains starttime of the service
int burstcomplete=0;  // Is set to 1 once the netburst completed
int uplinkup=0; /* is set to 1 if modules can send data */
char logfile[FILENAME_MAX]="xevres.log";
char helppath[300]="help/";
long timestdif=0;   // Timestamp-difference
MYSQL sqlcon;
array mainrnl[256];
long snlstartsize=1000;
array rngls;
long mf4warn[33]= { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1 };
const unsigned int mf4warnsteps[33] = { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
                                        10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
                                        10, 10, 10, 10, 10, 10, 10, 10, 5, 5,
                                        5, 5, 1 };
long lastsettime=0;
long waitingforping=0;
long lasthourly=0;
long laststatswrite=0;
long lastmin=0;
long lastfakenum=10;
long lastscan=0;
long resyncglinesat=0;
char sendglinesto[20480];
char *noreplytoping=NULL;
array myfakes; /* List of my fakeusers */
char *nicktomsg=NULL;
char params[MAXPARAMS][TMPSSIZE];     // The last line received from the
                                      // server, split at the spaces.
int paramcount;
char sender[TMPSSIZE];
array splitservers;
char modpath[FILENAME_MAX]="/home/fox/O/modules/"; // Path where loadable modules reside.
char modend[10]=".so"; // What filesuffix modules have
array modulelist;      // List of all loaded modules
array commandlist;     // List of all loaded commands
array fakecmdlist;     // List of all loaded commands for fake users
array serverhandlerlist[SIZEOFSHL]; // List of all serverhandlers
array internaleventlist[SIZEOFSHL]; // List of all internalevents
array fchanmsglist[SIZEOFSHL]; // List of all chanmsg handlers
int glineautosync=1;
volatile int igotsighup=0;
char *configfilename;
subnetchelp * ipv4snc[SIZEOFSNC];  /* Hashtable for subnetcounters */
int ipv4sncstartmask=19;  /* Subnetcount starts with this mask */
subnetchelp * impv4snt[SIZEOFSNC]; /* IMPlicit SubNet-Trusts - calculated from trustdata */
unsigned long ipv4usercount; /* Number of users on 0.0.0.0/0 */
unsigned long netmasks[33] = { 0x00000000, 0x80000000, 0xC0000000, 0xE0000000,
                               0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000,
                               0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
                               0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
                               0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000,
                               0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
                               0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
                               0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE,
                               0xFFFFFFFF
                             };
char logtimestampformat[TMPSSIZE/2] = "%d%m%y.%H%M%S";   /* strftime format for Log timestamps */

/* End of global variable definitions */

/* Realname-List Functions */

int rnladd(char *realname) {
  unsigned long i; rnlentry *tmpp; unsigned char a;
  a=(unsigned char)realname[0];
  tmpp=(rnlentry *)(mainrnl[a].content);
  for (i=0;i<mainrnl[a].cursi;i++) {
    if (strcmp(realname,tmpp[i].r)==0) {
      tmpp[i].n++;
      return (tmpp[i].n);
    }
  }
  // That realname is not yet in our list. Add it.
  i=array_getfreeslot(&(mainrnl[a]));
  tmpp=(rnlentry *)(mainrnl[a].content);
  tmpp[i].n=1;
  tmpp[i].r=getastring(realname);
  return 1;
}

void rnldel(char *realname) {
  unsigned long i; rnlentry *tmpp; unsigned char a;
  a=(unsigned char)realname[0];
  tmpp=(rnlentry *)(mainrnl[a].content);
  for (i=0;i<mainrnl[a].cursi;i++) {
    if (strcmp(realname,tmpp[i].r)==0) {
      tmpp[i].n--;
      if (tmpp[i].n<=0) {
        // Nobody on the net has this realname anymore, so we have to kill it from our list
        freeastring(tmpp[i].r);
        array_delslot(&(mainrnl[a]),i); i--;
        tmpp=(rnlentry *)(mainrnl[a].content);
      }
      return;
    }
  }
}

/* End of Realname-List functions */

void sighuphandler(int signo) {
  /* Of course this is not a clean way to do this, i should rather use a
     semaphore to prevent race conditions. However, this is "good enough".
     The chances something happens here are minimal, and it would only result
     in a weird logentry normally. */
  igotsighup=1;
}

/* Userlist functions */

void getuserdata(authdata *retad, char *username) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int res;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  retad->authlevel=-1;
  mysql_escape_string(tmps2,username,strlen(username));
  sprintf(tmps3,"SELECT * FROM users WHERE username='%s'",tmps2);
  res=mysql_query(&sqlcon,tmps3);
  if (res!=0) {
    putlog("!!! Failed to query user from database !!!");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=6) {
    putlog("!!! Userentry in database has invalid format !!!");
    return;
  }
  retad->authlevel=0;
  myrow=mysql_fetch_row(myres);
  if (myrow==NULL) { mysql_free_result(myres); return; }
  if (strlen(myrow[0])>AUTHUSERLEN) {
    putlog("!!! Authdata in Database contains too long Username: %s !!!",myrow[0]);
    retad->authlevel=-1; return;
  }
  strcpy(retad->username,myrow[0]);
  if (strlen(myrow[1])>AUTHPASSLEN) {
    putlog("!!! Authdata in Database contains too long Password: %s !!!",myrow[1]);
    retad->authlevel=-1; return;
  }
  strcpy(retad->password,myrow[1]);
  retad->authlevel=atoi(myrow[2]);
  retad->lastauthed=atol(myrow[3]);
  retad->wantsnotice=atoi(myrow[4]);
  retad->noticemask=strtoul(myrow[5],NULL,10);
  mysql_free_result(myres);
}

long countusersinul() {
  int res; MYSQL_RES *myres; long b;
  res=mysql_query(&sqlcon,"SELECT * FROM users");
  if (res!=0) {
    putlog("!!! Failed to query users from database !!!");
    return -1;
  }
  myres=mysql_store_result(&sqlcon);
  b=mysql_num_rows(myres);
  mysql_free_result(myres);
  return b;
}

void createrandompw(char *a, int n) {
  int b;
  for (b=0;b<n;b++) {
    a[b]=upwdchars[rand()%(sizeof(upwdchars)-1)];
  }
  a[n]='\0';
}

int updateuserinul(authdata ad) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], query[5000];
  int res;
  mysql_escape_string(tmps2,ad.username,strlen(ad.username));
  mysql_escape_string(tmps3,ad.password,strlen(ad.password));
  sprintf(query,"DELETE FROM users WHERE username='%s'",tmps2);
  mysql_query(&sqlcon,query);
  sprintf(query,"INSERT INTO users VALUES('%s','%s',%d,%ld,%d,%lu)",tmps2,tmps3,ad.authlevel,ad.lastauthed,ad.wantsnotice,ad.noticemask);
  res=mysql_query(&sqlcon,query);
  return (res==0);
}

autheduser * getalentry(long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      return a;
    }
    a=a->next;
  }
  return NULL;
}

void newalentry(long unum) {
  autheduser *a, *b;
  a=als;
  b=(autheduser *) malloc(sizeof(autheduser));
  b->numeric=unum; b->next=NULL;
  if (a==NULL) { als=b; return; }
  while (a!=NULL) {
    if (a->next==NULL) {
      a->next=b; return;
    }
    a=a->next;
  }
  putlog("!!! This mark in newalentry should never be reached %s line %d !!!",
    __FILE__ , __LINE__);
}

void addautheduser(long unum, authdata ad) {
  autheduser *a;
  a=getalentry(unum);
  if (a==NULL) { newalentry(unum); a=getalentry(unum); }
  strcpy(a->authedas,ad.username);
  a->authlevel=ad.authlevel;
  a->wantsnotice=ad.wantsnotice;
  a->noticemask=ad.noticemask;
}

void delautheduser(long unum) {
  autheduser *a, *b, *c;
  a=als; b=NULL;
  while (a!=NULL) {
    if (a->numeric==unum) {
      c=a;
      if (b!=NULL) {
        b->next=a->next;
      } else {
        als=a->next;
      }
      free(c);
      return;
    }
    b=a;
    a=a->next;
  }
}

/* End of Userlist functions */

int opensql(char *host, char *username, char *password, char *database, int port) {
  mysql_init(&sqlcon);
  if (!mysql_real_connect(&sqlcon, host,username,password,database,port,NULL,0)) {
    printf("Error connecting to mysql db.\n");
    return -1;
  }
  return 0;
}

int tcpopen(char *host, unsigned int port) {
  int unit; struct sockaddr_in sin; struct hostent *hp;
  if ((hp=gethostbyname(host)) == NULL) {
    fprintf(stderr,"Oops ... unknown host \"%s\"\n",host);
    return(-1);
  }
  memset((char *)&sin, 0, sizeof(sin));
  if (sizeof(sin.sin_addr)<hp->h_length) {
    fprintf(stderr,"Ooops - host \"%s\" resolves to non IPv4-address\n",host);
    return (-1);
  }
  memcpy((char *)&sin.sin_addr,hp->h_addr,hp->h_length);
  sin.sin_family=hp->h_addrtype;
  sin.sin_port=htons(port);
  if ((unit=socket(PF_INET,SOCK_STREAM,0)) < 0) {
    fprintf(stderr,"Oops ... tcp/ip cannot open socket\n");
    return (-1);
  }
  if (connect(unit,(struct sockaddr *)&sin,sizeof(sin)) < 0) {
    fprintf(stderr,"Oops ... tcp/ip cannot connect to server\n");
    return (-1);
  }
  return(unit);
}

int numericinuse(long num) {
  serverdata *b;
  b=sls;
  while (b!=NULL) {
    if (b->numeric==num) { return 1; }
    b=b->next;
  }
  return 0;
}

void delchan(char *name) {
  channel *a, *b; unsigned int c;
  c=clhash(name);
  a=cls[c]; b=NULL;
  while (a!=NULL) {
    if (strcmp(a->name,name)==0) {
      // this is the channel we want to delete
      if (b==NULL) {
        cls[c]=a->next;
      } else {
        b->next=a->next;
      }
      array_free(&(a->users));
      freeastring(a->name);
      freeastring(a->topic);
      free(a);
      a=NULL;
      return;
    }
    b=a;
    a=a->next;
  }
}

void deluserfromallchans(long num) {
  channel **c; unsigned long i; userdata *a;
  i=ulhash(num);
  a=uls[i];
  while (a!=NULL) {
    if (a->numeric==num) {
      c=(channel **)(a->chans.content);
      for (i=0; i < a->chans.cursi;i++) {
        deluserfromchan(c[i]->name,num); // Note that from now on c[i]->name
                                         // might point to nonsense!
      }
      while (a->chans.cursi > 0) { array_delslot(&(a->chans),0); }
      return;
    }
    a=a->next;
  }
}

/* delallchanson(unum) - deletes all channels that existed on server with
                         numeric unum (on a split) */
void delallchanson(long num) {
  channel *a, *b, *d; chanuser *c; int i;
  long j;
  for (i=0;i<SIZEOFCL;i++) {
    a=cls[i]; b=NULL;
    while (a!=NULL) {
      c=(chanuser *)a->users.content;
      for (j=0;j < a->users.cursi;j++) { // Could be problem
        if ((c[j].numeric>>SRVSHIFT)==num) {
          array_delslot(&(a->users),j);
          c=(chanuser *)a->users.content;
          j--;
        }
      }
      if (a->users.cursi==0) { 
        // Delete the channel
        if (b==NULL) {
          cls[i]=a->next;
        } else {
          b->next=a->next;
        }
        d=a->next;
        array_free(&(a->users));
        freeastring(a->name);
        freeastring(a->topic);
        free(a);
        a=d;
      } else {
        b=a;
        a=a->next;
      }
    }
  }
}

channel * getchanptr(char *name) {
  channel *a;
  a=cls[clhash(name)];
  while (a!=NULL) {
    if (strcmp(a->name,name)==0) {
      return a;
    }
    a=a->next;
  }
  return NULL;
}

long getsrvnum(char *a) {
  serverdata *b;
  b=sls;
  while (b!=NULL) {
    if (strcmp(b->nick,a)==0) { return b->numeric; }
    b=b->next;
  }
  return -1;
}

int serverdoesnotreplytopings(serverdata *s) {
  char *tmps; char *a;
  if (s->connectedto==tokentolong(servernumeric)) { return 1; }
  if (noreplytoping==NULL) { return 0; }
  tmps=(char *)malloc(strlen(noreplytoping)+1);
  strcpy(tmps,noreplytoping);
  a=strtok(tmps," ");
  while (a!=NULL) {
    if (strcmp(s->nick,a)==0) {
      free(tmps); return 1;
    }
    a=strtok(NULL," ");
  }
  free(tmps);
  return 0;
}

void pingallservers() {
  serverdata *b; char tmps2[TMPSSIZE];
  b=sls;
  while (b!=NULL) {
    b->pingreply=0; b->pingsent=getnettime(); strcpy(b->ports,"");
    longtotoken(b->numeric,tmps2,2);
    sendtouplink("%sAAB R u %s\r\n",servernumeric,tmps2);
    sendtouplink("%sAAB R P %s\r\n",servernumeric,tmps2);
    b=b->next;
  }
  fflush(sockout);
}

long countusershit(char *pattern) {
  long howmany=0; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int i;
  int icis; char tmps4[TMPSSIZE]; unsigned int x1,x2,x3,x4;
  icis=0;
  if ((pattern[0]=='#') || (pattern[0]=='+') || (pattern[0]=='&')) { /* Channelpattern */
    channel *c;
    for (i=0;i<SIZEOFCL;i++) {
      c=cls[i];
      strcpy(tmps2,pattern); toLowerCase(tmps2);
      if ((ischarinstr('*',tmps2)) || (ischarinstr('?',tmps2))) {
        icis=1;
      }
      while (c!=NULL) {
        if (icis==1) { /* There is a wildcard in the pattern, we'll have to do
                          an (expensive) pattern match */
          strcpy(tmps3,c->name);
          if (match2strings(tmps2,tmps3)) {
            howmany++;
          }
        } else { /* Ha, we only need an exact match, that's cheap! */
          if (strcmp(tmps2,c->name)==0) { howmany++; }
        }
        c=c->next;
      }
    }
  } else { /* User@host or nick!user@host */
    char patnick[TMPSSIZE]; char patident[TMPSSIZE]; char pathost[TMPSSIZE];
    int res; int niiwc, idiwc, hoiwc; /* iwc = is containing wildcards */
    int doesmatch; userdata *a; int hoisn=0; /*host is subnet */
    unsigned long snip=0, snmask=0;
    int nimae, idmae, homae; /* mae=matches everything */
    if (ischarinstr('!',pattern)) {
      res=sscanf(pattern,"%[^!]%[^@]%[^\n]",patnick,patident,pathost);
      if (res<3) {
        putlog("Not a valid user-pattern: %s",pattern);
        return 0;
      }
      delchar(patident,'!'); delchar(pathost,'@');
    } else {
      if (ischarinstr('@',pattern)) {
        strcpy(patnick,"*");
        res=sscanf(pattern,"%[^@]%[^\n]",patident,pathost);
        if (res<2) {
          putlog("Not a valid user-pattern: %s",pattern);
          return 0;
        }
        delchar(pathost,'@');
      } else {
        strcpy(patnick,"*");
        strcpy(patident,"*");
        strcpy(pathost,pattern);
      }
    }
    toLowerCase(patnick);
    toLowerCase(patident);
    toLowerCase(pathost);
    if ((ischarinstr('*',patnick)) || (ischarinstr('?',patnick))) { niiwc=1; } else { niiwc=0; }
    if ((ischarinstr('*',patident)) || (ischarinstr('?',patident))) { idiwc=1; } else { idiwc=0; }
    if ((ischarinstr('*',pathost)) || (ischarinstr('?',pathost))) {
      hoiwc=1;
    } else {
      hoiwc=0;
      if (ischarinstr('/',pathost)) {
        char patip[TMPSSIZE]; char patmask[TMPSSIZE];
        hoisn=1;
        res=sscanf(pathost,"%[^/]%[^\n]",patip,patmask);
        if (res<2) { putlog("Not a valid subnetmask: %s",pattern); return 0; }
        delchar(patmask,'/');
        snmask=strtoul(patmask,NULL,10);
        if (snmask>32) { putlog("Not a valid subnetmask: %s",pattern); return 0; }
        snip=parseipv4(patip);
      } else {
        snip=parseipv4(pathost);
        if (snip>0) {
          hoisn=1; snmask=32;
        }
      }
    }
    nimae=(strcmp(patnick,"*")==0);
    idmae=(strcmp(patident,"*")==0);
    homae=(strcmp(pathost,"*")==0);
    if ((hoisn==1) && (nimae) && (idmae)) {
      return sncget(snip,snmask);
    }
    snmask=netmasks[snmask];
    if ((hoisn==1) && (snip==0) && (snmask==0)) { homae=1; }
//    putlog("DEBUG: matching %s %d %s %d %s %d",patnick,niiwc,patident,idiwc,pathost,hoiwc);
    for (i=0;i<SIZEOFUL;i++) {
      a=uls[i];
      while (a!=NULL) {
        doesmatch=1;
        if (!homae) {
          if (hoisn>0) {
            if ((a->realip&snmask)!=(snip&snmask)) {
              doesmatch=0;
            }
          } else {
            splitip(a->realip,&x1,&x2,&x3,&x4);
            sprintf(tmps4,"%u.%u.%u.%u",x1,x2,x3,x4);
            strcpy(tmps3,a->host); toLowerCase(tmps3);
            if (hoiwc>0) {
              if ((!match2strings(pathost,tmps4)) && (!match2strings(pathost,tmps3))) {
                doesmatch=0;
              }
            } else {
              if ((strcmp(pathost,tmps4)!=0) && (strcmp(pathost,tmps3)!=0)) {
                doesmatch=0;
              }
            }
          }
        }
        if (!idmae) {
          strcpy(tmps3,a->ident); toLowerCase(tmps3);
          if (idiwc>0) {
            if (!match2strings(patident,tmps3)) { doesmatch=0; }
          } else {
            if (strcmp(patident,tmps3)!=0) { doesmatch=0; }
          }
        }
        if (!nimae) {
          strcpy(tmps3,a->nick); toLowerCase(tmps3);
          if (niiwc>0) {
            if (!match2strings(patnick,tmps3)) { doesmatch=0; }
          } else {
            if (strcmp(patnick,tmps3)!=0) { doesmatch=0; }
          }
        }
        if (doesmatch==1) { howmany++; }
        a=a->next;
      }
    }
  }
  return howmany;
}

long noticeallircops(char *thetext) {
  userdata *a; char target[10]; int i; long cnt=0;
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      if ((ischarinstr('o',a->umode)) && (!ischarinstr('d',a->umode))) {
        longtotoken(a->numeric,target,5);
        if (getwantsnotic2(a->numeric)) {
          sendtouplink("%sAAB O %s :%s\r\n",servernumeric,target,thetext);
        } else {
          sendtouplink("%sAAB P %s :%s\r\n",servernumeric,target,thetext);
        }
        cnt++;
      }
      a=a->next;
    }
  }
  fflush(sockout);
  return cnt;
}

void delalluserson(long num) {
  userdata *a, *b, *c; int i;
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i]; b=NULL;
    while (a!=NULL) {
      if ((a->numeric>>SRVSHIFT)==num) {
        delautheduser(a->numeric);
        delfromnicklist(a->nick);
        sncdel(a->realip);
        rnldel(a->realname);
        trustdelclient(a->ident,a->realip);
        c=a;
        if (b==NULL) {
          uls[i]=a->next;
        } else {
          b->next=a->next;
        }
        a=a->next;
        array_free(&(c->chans));
        free(c);
      } else {
        b=a;
        a=a->next;
      }
    }
  }
}

void delsrv(char *a) {
  long num; char tmps2[100];
  serverdata *b, *c;
  num=getsrvnum(a);
  longtotoken(num,tmps2,2);
  putlog("Lost server: %s (Numeric %s)",a,tmps2);
  addsplit(a);
  delallchanson(num);
  delalluserson(num);
  b=sls; c=NULL;
  while (b!=NULL) {
    if (b->numeric==num) {
      if (c==NULL) {
        sls=b->next;
      } else {
        c->next=b->next;
      }
      free(b);
      b=sls; c=NULL;
      while (b!=NULL) {
        if (b->connectedto==num) {
          delsrv(b->nick);
          b=sls; c=NULL;
        } else {
          c=b;
          b=b->next;
        }
      }
      return;
    }
    c=b;
    b=b->next;
  }
}

void deluserfromchan(char *name, long numeric) {
  channel *a; chanuser *b;
  long i;
//  printf("Deluser %d from %s\n",numeric,name);
  a=getchanptr(name);
  if (a==NULL) {
    putlog("!!! Attempt to kill user from nonexistant channel %s !!!",name);
    return;
  }
  if (a->users.cursi==0) {
    putlog("!!! Attempt to kill user from empty channel %s !!!",name);
    return;
  }
  b=(chanuser *)a->users.content;
  if (b==NULL) { return; }
  for (i=0;i < a->users.cursi;i++) {
    if (b[i].numeric==numeric) {
      array_delslot(&(a->users),i);
      i--;
      b=(chanuser *)a->users.content;   /* Can't believe i forgot that, ARGH */
    }
  }
  if (a->users.cursi==0) {
    // The channel is now empty, this was the last user in it
    delchan(name);
  }
}

void addnicktoul(userdata *a) {
  int i;
  i=ulhash(a->numeric);
  a->next=uls[i];
  uls[i]=a;
}

void addchantouser(userdata *a, channel *c) {
  int i; channel **d;
  i=array_getfreeslot(&(a->chans));
  d=(channel **)(a->chans.content);
  d[i]=c;
}

void addchantouser2(long num, channel *c) {
  userdata *a; unsigned int i;
  i=ulhash(num);
  if (i>=SIZEOFUL) {
    putlog("!!! Warning: ulhash returned value out of range! (%u) !!!",i);
    putlog("DEBUG: Lastline was: %s\n",lastline);
    return;
  }
  a=uls[i];
  while (a!=NULL) {
    if (a->numeric==num) {
      addchantouser(a,c);
      return;
    }
    a=a->next;
  }
}

void delchanfromuser(long num, char *channam) {
  unsigned long i; channel **d; userdata *a; unsigned int j;
  j=ulhash(num);
  a=uls[j];
  while (a!=NULL) {
    if (a->numeric==num) {
      d=(channel **)(a->chans.content);
      for (i=0;i<a->chans.cursi;i++) {
        if (strcmp(channam,d[i]->name)==0) {
          array_delslot(&(a->chans),i);
          return;
        }
      }
      return;
    }
    a=a->next;
  }
}

void newserver(char *nick, int hopsaway, long numeric, long createdat, long connectat, long connectedto, long maxclients) {
  serverdata *a; char tmps2[100];
  longtotoken(numeric,tmps2,2);
  putlog("New Server: %s (Numeric %s)",nick,tmps2);
  a=(serverdata *) malloc(sizeof(serverdata));
  a->hopsaway=hopsaway;
  a->numeric=numeric;
  a->createdat=createdat; a->connectat=connectat;
  a->connectedto=connectedto; a->maxclients=maxclients;
  a->pingreply=0; a->pingsent=0;
  strcpy(a->ports,"");
  if (strlen(nick)>SRVLEN) {
    putlog("!!! SERVERNAME exceeds maxlength! | %s | !!!",lastline);
  }
  strncpy(a->nick,nick,SRVLEN);
  a->nick[SRVLEN]='\0';
  if (burstcomplete==1) { changesplitstatus(a->nick,1); }
  a->next=sls;
  sls=a;
}

int chanexists(char *name) {
  channel *a;
  a=cls[clhash(name)];
  while (a!=NULL) {
    if (strcmp(name,a->name)==0) { return 1; }
    a=a->next;
  }
  return 0;
}

channel * newchan(char *name, long createdat) {
  channel *a; unsigned int b;
//  if (name[0]!='#') { printf("New strange channel: %s \n",name); }
  a=(channel *) malloc(sizeof(channel));
  a->name=getastring(name);
  a->createdat=createdat;
  a->flags=0;
  array_init(&(a->users),sizeof(chanuser));
  array_setlim1(&(a->users),10);
  array_setlim2(&(a->users),15);
  a->reg=findregchannelifexists(name);
  a->canhavebans=0;
  a->topic=getastring("");
  b=clhash(name);
  a->next=cls[b];
  cls[b]=a;
  return a;
}

void setchankey(char *name, char *key) {
  channel *a;
  a=getchanptr(name);
  if (a!=NULL) { mystrncpy(a->chankey,key,CHANKEYLEN); }
}

void setchanke2(channel *a, char *key) {
  mystrncpy(a->chankey,key,CHANKEYLEN);
}

void setchanlim(char *name, int lim) {
  channel *a;
  a=getchanptr(name);
  if (a!=NULL) { a->chanlim=lim; }
}

void setchanli2(channel *a, int lim) {
  a->chanlim=lim;
}

void setfwchan(char *name, char *chan) {
  channel *a;
  a=getchanptr(name);
  if (a!=NULL) { mystrncpy(a->fwchan,chan,CHANNELLEN); }
}

void setfwchan2(channel *a, char *chan) {
  mystrncpy(a->fwchan,chan,CHANNELLEN);
}

void setchanflag(char *name, int flags) {
  channel *a;
  a=getchanptr(name);
  if (a!=NULL) { a->flags=(a->flags | flags); }
}

void delchanflag(char *name, int flags) {
  channel *a;
  a=getchanptr(name);
  if (a!=NULL) { a->flags=(a->flags & (~flags)); }
}

void setchanfla2(channel *a, int flags) {
  a->flags=(a->flags | flags);
}

void delchanfla2(channel *a, int flags) {
  a->flags=(a->flags & (~flags));
}

void addusertocptr(channel *b, long numeric) {
  chanuser *a; unsigned long i;
  i=array_getfreeslot(&(b->users));
  a=(chanuser *)b->users.content;
  a[i].numeric=numeric;
  a[i].flags=0;
}

void addusertochan(char *name, long numeric) {
  channel *b;
  b=getchanptr(name);
  if (b==NULL) {
    putlog("!!! Attempt to add user to nonexistant channel %s !!!",name);
    return;
  }
  addusertocptr(b,numeric);
}

void addgline(char *hostmask, char *reason, char *bywhom, long howlong, int sendtonet) {
  int res; char escapedhm[TMPSSIZE], escapedreas[TMPSSIZE], escapedbywhom[TMPSSIZE]; char query[5000]="";
  if (howlong==0) { return; }
  if (sendtonet!=0) {
    sendtouplink("%s GL * +%s %ld :%s\r\n",servernumeric,hostmask,howlong,reason);
    fflush(sockout);
  }
  mysql_escape_string(escapedhm,hostmask,strlen(hostmask));
  mysql_escape_string(escapedreas,reason,strlen(reason));
  mysql_escape_string(escapedbywhom,bywhom,strlen(bywhom));
  sprintf(query,"DELETE FROM glines WHERE hostmask='%s'",escapedhm);
  mysql_query(&sqlcon,query);
  sprintf(query,"INSERT INTO glines values('%s','%s',%ld,'%s')",escapedhm,escapedbywhom,(howlong+getnettime()),escapedreas);
//  printf("%s\n",query);
  res=mysql_query(&sqlcon,query);
  if (res!=0) {
    putlog("!!! Failed to insert gline into database !!!");
    return;
  }
}

void remgline(char *hostmask, int sendtonet) {
  int res; char escapedhm[TMPSSIZE]; char query[5000]="";
  if (sendtonet!=0) {
    sendtouplink("%s GL * -%s\r\n",servernumeric,hostmask);
    fflush(sockout);
  }
  mysql_escape_string(escapedhm,hostmask,strlen(hostmask));
  sprintf(query,"DELETE FROM glines WHERE hostmask='%s'",escapedhm);
  res=mysql_query(&sqlcon,query);
  if (res!=0) {
    putlog("!!! Failed to remove gline '%s' from database !!!",hostmask);
    return;
  }
}

void purgeglines() {
  int res; char query[TMPSSIZE];
  sprintf(query,"DELETE FROM glines WHERE expires<%ld",(long int)getnettime());
  res=mysql_query(&sqlcon,query);
  if (res!=0) {
    putlog("!!! GLINE-Purge returned error !!!");
    return;
  }
}

int gotallpingreplys() {
  serverdata *s;
  s=sls;
  while (s!=NULL) {
    if (s->pingreply==0) {
      if (serverdoesnotreplytopings(s)!=1) { return 0; }
    }
    s=s->next;
  }
  return 1;
}

long getmaxping() {
  serverdata *s; long maxping;
  s=sls; maxping=-1;
  while (s!=NULL) {
    if (s->pingreply!=0) {
      if ((s->pingreply-s->pingsent)>maxping) { maxping=s->pingreply-s->pingsent; }
    }
    s=s->next;
  }
  return maxping;
}

void changechanmod2(channel *a, long numeric, int sm, int flags) {
  // sm = Setmode: 0 = set flags, 1 = add flags, 2 = remove flags
  chanuser *b; unsigned long i;
  b=(chanuser *)a->users.content;
  for (i=0;i<a->users.cursi;i++) {
    if (b[i].numeric==numeric) {
      if (sm==0) { b[i].flags=flags; }
      if (sm==1) { b[i].flags=(b[i].flags | flags); }
      if (sm==2) { b[i].flags=(b[i].flags & (~ flags)); }
      return;
    }
  }
//  putlog("!!! Mode change for nonexistant user on %s !!!",a->name);
}

void changechanmode(char *name, long numeric, int sm, int flags) {
  // sm = Setmode: 0 = set flags, 1 = add flags, 2 = remove flags
  channel *a;
  a=getchanptr(name);
  if (a==NULL) {
    putlog("!!! Mode change for nonexistant channel %s !!!",name);
    return;
  }
  changechanmod2(a,numeric,sm,flags);
}

int getchanmode2(channel *a, long numeric) {
  chanuser *b; unsigned long i;
  b=(chanuser *)a->users.content;
  for (i=0;i<a->users.cursi;i++) {
    if (b[i].numeric==numeric) {
      return b[i].flags;
    }
  }
  return -1;
}

int getchanmodes(char *name, long numeric) {
  channel *a;
  a=getchanptr(name);
  if (a==NULL) { return -1; } else { return getchanmode2(a,numeric); }
}

int loadconfig(char *filename, int isrehash) {
  FILE *myf; char myline[1024]; char *res;
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int r2, r3;
  myf=fopen(filename,"r");
  if (myf==NULL) { return (-1); }
  if (noreplytoping!=NULL) { free(noreplytoping); noreplytoping=NULL; }
  for (r2=0;r2<=32;r2++) { mf4warn[r2]=-1; }
  do {
    res=fgets(myline,1000,myf);
    delchar(myline,'\n');
    delchar(myline,'\r');
    if ((myline[0]!='\0') && (myline[0]!='#')) {
      // This line is not a comment, we'll have to parse it
      r2=sscanf(myline,"%[^=]%[^\n]",tmps2,tmps3);
      if (r2!=2) {
        putlog("Illegal line format in configfile: %s",myline);
      } else {
        delchar(tmps3,'=');
//        printf("%s // %s\n",tmps2,tmps3);
        if (strcmp(tmps2,"logfile")==0) { strcpy(logfile,tmps3); }
        if (strcmp(tmps2,"helppath")==0) { strcpy(helppath,tmps3); }
        if (strcmp(tmps2,"modpath")==0) { strcpy(modpath,tmps3); }
        if (strcmp(tmps2,"modend")==0) { strcpy(modend,tmps3); }
        if (strncmp(tmps2,"mf4warn",7)==0) {
          long h=strtol(&tmps2[7],NULL,10);
          if ((h<0) || (h>32)) {
            putlog("Illegal mf4warn subnet (%ld)",h);
          } else {
            mf4warn[h]=strtol(tmps3,NULL,10);
          }
        }
        if (strcmp(tmps2,"clonewarn")==0) {
          putlog("WARNING: You are using an obsolete option in your config file (clonewarn)!");
          putlog("Please update your config file, as the results may be unpredictable otherwise.");
        }
        if (strcmp(tmps2,"glineautosync")==0) { glineautosync=strtol(tmps3,NULL,10); }
        if (strcmp(tmps2,"loadmod")==0) { 
          int res;
          if (isrehash==1) {
            res=removemodule(tmps3);
            if (res==0) {
              putlog("Module was unloaded successfully: %s",tmps3);
            }
            if (res==1) {
              putlog("Module '%s' was not loaded before", tmps3);
            }
            if (res<0) {
              putlog("Error unloading module '%s'", tmps3);
            }
          }
          res=loadandinitmodule(tmps3);
          if (res==0) {
            putlog("Module loaded successfully: %s",tmps3);
          } else {
            putlog("Failed to load module: %s",tmps3);
          }
        }
        if (strcmp(tmps2,"noreplytoping")==0) {
          if (noreplytoping==NULL) {
            noreplytoping=(char *)malloc(strlen(tmps3)+1);
            strcpy(noreplytoping,tmps3);
          } else {
            noreplytoping=(char *)realloc(noreplytoping,strlen(noreplytoping)+strlen(tmps3)+2);
            strcat(noreplytoping," ");
            strcat(noreplytoping,tmps3);
          }
        }
        if (strcmp(tmps2,"logtimestampformat")==0) { mystrncpy(logtimestampformat,tmps3,(TMPSSIZE/2-1)); }
        if (isrehash==0) {
          if (strcmp(tmps2,"mynick")==0) { mystrncpy(mynick,tmps3,NICKLEN); }
          if (strcmp(tmps2,"myname")==0) { mystrncpy(myname,tmps3,99); }
          if (strcmp(tmps2,"conto")==0) { mystrncpy(conto,tmps3,99); }
          if (strcmp(tmps2,"conport")==0) { conport=strtoul(tmps3,NULL,10); }
          if (strcmp(tmps2,"conpass")==0) { mystrncpy(conpass,tmps3,99); }
          if (strcmp(tmps2,"numeric")==0) { longtotoken(atoi(tmps3),servernumeric,2); }
          if (strcmp(tmps2,"sqlhost")==0) { mystrncpy(sqlhost,tmps3,99); }
          if (strcmp(tmps2,"sqlport")==0) { sqlport=atol(tmps3); }
          if (strcmp(tmps2,"sqluser")==0) { mystrncpy(sqluser,tmps3,99); }
          if (strcmp(tmps2,"sqlpass")==0) { mystrncpy(sqlpass,tmps3,99); }
          if (strcmp(tmps2,"sqldb")==0) { mystrncpy(sqldb,tmps3,99); }
          if (strcmp(tmps2,"ipv4sncstartmask")==0) { ipv4sncstartmask=(strtoul(tmps3,NULL,10) % 33); }
	  if (strcmp(tmps2,"stayhere")==0) { stayhere=atol(tmps3); }
        }
      }
    } else {
//      printf("Line ignored: %s\n",myline);
    }
  } while (res!=NULL);
  fclose(myf);
  r3=-1;
  for (r2=0;r2<=32;r2++) {
    if ((mf4warn[r2]==-1) && (r3>0)) { // Can we fill this in automatically?
      int r4;
      for (r4=r2;r4<=32;r4++) {
        if (mf4warn[r4]!=-1) {
          mf4warn[r2]=r3+((r3-mf4warn[r4])/(r4-r2)); // No this calculation is not
                                 // broken, it is not supposed to be linear.
          break;
        }
      }
    } else {
      r3=mf4warn[r2];
    }
  }
  return 0;
}

void numtonick(long num, char *res) {
  userdata *a;
  a=uls[ulhash(num)];
  while (a!=NULL) {
    if (num==a->numeric) {
      strcpy(res,a->nick);
      return;
    }
    a=a->next;
  }
  strcpy(res,"");
  putlog("!!! Failed to convert numeric %ld to nick !!!",num);
}

void numtohostmask(long num, char *res) {
  userdata *a;
  a=uls[ulhash(num)];
  while (a!=NULL) {
    if (num==a->numeric) {
      sprintf(res,"%s!%s@%s",a->nick,a->ident,a->host);
      return;
    }
    a=a->next;
  }
  strcpy(res,"");
  putlog("!!! Failed to convert numeric %ld to hostmask !!!",num);
}

int alreadyinglinesto(char *nick) {
  char tmps2[25000]; char *tmps3, *tmps4;
  strcpy(tmps2,sendglinesto);
  tmps3=strtok_r(tmps2,",",&tmps4);
  while (tmps3!=NULL) {
    if (strcmp(tmps3,nick)==0) { return 1; }
    tmps3=strtok_r(NULL,",",&tmps4);
  }
  return 0;
}

void dumpuserlist() {
  userdata *a; int i; channel *c; long j; userdata **b;
  printf("Dumping all lists...\n");
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      printf("User: %s ; Realname: %s\n",a->nick,a->realname);
      a=a->next;
    }
  }
  for (i=0;i<SIZEOFCL;i++) {
    c=cls[i];
    while (c!=NULL) {
      printf("Channel: %s\n",c->name);
      c=c->next;
    }
  }
  for (i=0;i<SIZEOFNL;i++) {
    b=(userdata **)nls[i].content;
    for (j=0; j<nls[i].cursi; j++) {
      printf("Nicklist: %s\n",b[j]->nick);
    }
  }
}

long nicktonu2(char *a) {
  userdata **b; long i; unsigned int hash;
  hash=nlhash(a);
  b=(userdata **)nls[hash].content;
  for (i=0;i<nls[hash].cursi;i++) {
    if (strcmp(b[i]->nick,a)==0) {
      return b[i]->numeric;
    }
  }
  return -1;
}

long nicktonum(char *a) {
  long c;
  c=nicktonu2(a);
  if (c==-1) {
    putlog("!!! Couldnt translate nick %s to numeric !!!",a);
  }
  return c;
}

unsigned long countusersonthenet() {
  return ipv4usercount;
}

long countusersonsrv(long num) {
  userdata *a; long n=0; int i;
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      if ((a->numeric>>SRVSHIFT)==num) { n++; }
      a=a->next;
    }
  }
  return n;
}

long countopersonsrv(long num) {
  userdata *a; long n=0; int i;
  for (i=0;i<SIZEOFUL;i++) {
    a=uls[i];
    while (a!=NULL) {
      if ((a->numeric>>SRVSHIFT)==num) {
        if (ischarinstr('o',a->umode)) { n++; }
      }
      a=a->next;
    }
  }
  return n;
}

void showhelp(long unum, char *command) {
  FILE *helpfile; char fnam[TMPSSIZE]; char tmps2[TMPSSIZE]; char *res;
  strcpy(fnam,helppath);
  strcpy(tmps2,command);
  toLowerCase(tmps2);
  delchar(tmps2,'/');
  delchar(tmps2,'\\');
  delchar(tmps2,':');
  strcat(fnam,tmps2);
  strcat(fnam,".help");
  helpfile=fopen(fnam,"r");
  if (helpfile==NULL) {
    msgtouser(unum,"No help available for that.");
    return;
  }
  do {
    res=fgets(tmps2,300,helpfile);
    if (res!=NULL) {
      delchar(tmps2,'\r');
      delchar(tmps2,'\n');
      printhelp(unum,tmps2);
    }
  } while (res!=NULL);
  msgtouser(unum,"--- End of help ---");
  fclose(helpfile);
}

void glinesync() {
  int res; MYSQL_RES *myres; MYSQL_ROW myrow; unsigned long i; char *tmps2, *tmps3; char FICKEN[20480];
  int numsrvs=0;
  purgeglines();
  tmps2=&sendglinesto[0];
  while (*tmps2!='\0') {
    if (*tmps2==',') { numsrvs++; }
    tmps2++;
  }
  if  (numsrvs>MAXINDIVIDUALGLINESYNC) {
    putlog("Resyncing glines to * (Because more than %d servers (%d) need resyncing)",MAXINDIVIDUALGLINESYNC,numsrvs);
  } else {
    putlog("Resyncing glines to %s",sendglinesto);
  }
  res=mysql_query(&sqlcon,"SELECT * FROM glines");
  if (res!=0) {
    putlog("!!! Failed to query gline-list from database in %s line %d !!!",
      __FILE__ , __LINE__);
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=4) {
    putlog("!!! GList in database has invalid format !!!");
    return;
  }
  i=0;
  while ((myrow=mysql_fetch_row(myres))) {
    if (numsrvs>MAXINDIVIDUALGLINESYNC) {
      sendtouplink("%s GL * +%s %ld :%s\r\n",servernumeric,myrow[0],strtol(myrow[2],NULL,10)-getnettime(),myrow[3]);
      i++;
      if ((i%1000)==0) { putlog("%lu",i); }
    } else {
      strcpy(FICKEN,sendglinesto);
      tmps3=strtok_r(FICKEN,",",&tmps2);
      while (tmps3!=NULL) {
        sendtouplink("%s GL %s +%s %ld :%s\r\n",servernumeric,tmps3,myrow[0],strtol(myrow[2],NULL,10)-getnettime(),myrow[3]);
        tmps3=strtok_r(NULL,",",&tmps2);
        i++;
        if ((i%1000)==0) { putlog("%lu",i); }
      }
    }
  }
  fflush(sockout);
  strcpy(sendglinesto,"");
  mysql_free_result(myres);
  putlog("Done, %lu total.",i);
}

int isglineset(char *mask) {
  int res; MYSQL_RES *myres; MYSQL_ROW myrow; char tmps2[TMPSSIZE*2], tmps3[TMPSSIZE];
  purgeglines();
  mysql_escape_string(tmps3,mask,strlen(mask));
  sprintf(tmps2,"SELECT * FROM glines WHERE hostmask='%s'",tmps3);
  res=mysql_query(&sqlcon,tmps2);
  if (res!=0) {
    return 0;
  }
  myres=mysql_store_result(&sqlcon);
  if (myres==NULL) { return 0; }
  if (mysql_num_fields(myres)!=4) {
    mysql_free_result(myres);
    return 0;
  }
  if ((myrow=mysql_fetch_row(myres))) {
    mysql_free_result(myres);
    return 1;
  } else {
    mysql_free_result(myres);
    return 0;
  }
}

void saveall() {
//  savetrustlist();  /* subnet-trusts */
  rnglsave();
  fakeusersave();
  savetrusts();     /* host trustgroups */
  saveallchans();   /* channel op database */
  savetrustdeny();
}

void dochanneljoins() {
  MYSQL_RES *sqlres; unsigned int numfields;
  MYSQL_ROW sqlrow; int res; channel *ctmp;
  res=mysql_query(&sqlcon,"SELECT * FROM channels");
  if (res!=0) {
    putlog("!!! Could not read my channellist from the database !!!");
    return;
  }
  sqlres=mysql_store_result(&sqlcon);
  numfields=mysql_num_fields(sqlres);
  if (numfields!=1) {
    putlog("!!! Channellist has illegal format !!!");
    mysql_free_result(sqlres);
    return;
  }
  while ((sqlrow=mysql_fetch_row(sqlres))) {
    toLowerCase(sqlrow[0]);
    if (!chanexists(sqlrow[0])) {
      ctmp=newchan(sqlrow[0],starttime);
      sendtouplink("%sAAB C %s %ld\r\n",servernumeric,sqlrow[0],starttime);
    } else {
      ctmp=getchanptr(sqlrow[0]);
      sendtouplink("%sAAB J %s\r\n",servernumeric,sqlrow[0]);
      sendtouplink("%s M %s +o %sAAB\r\n",servernumeric,sqlrow[0],servernumeric);
    }
    addusertocptr(ctmp,(tokentolong(servernumeric)<<SRVSHIFT)+1);
    changechanmod2(ctmp,(tokentolong(servernumeric)<<SRVSHIFT)+1,0,um_o);
    addchantouser2((tokentolong(servernumeric)<<SRVSHIFT)+1,ctmp);
  }
  fflush(sockout);
  mysql_free_result(sqlres);
}

void goaway() {
  if (stayhere==0) {  
    putlog("Ok, we are up - time to go in the background, cya.");
    weareinvisible=1;
    if (fork())
      exit(0);
    setsid();
  } else {
    putlog("stayhere not set to 0 - will not launch into background!");
  }    
}

void addtonicklist(userdata *a) {
  userdata **b; long l; unsigned int hash;
  hash=nlhash(a->nick);
  l=array_getfreeslot(&(nls[hash]));
  b=(userdata **)nls[hash].content;
  b[l]=a;
}

void delfromnicklist(char *nick) {
  long l; unsigned int hash; userdata **a;
  hash=nlhash(nick);
  a=(userdata **)nls[hash].content;
  for (l=0;l<nls[hash].cursi;l++) {
    if (strcmp(a[l]->nick,nick)==0) {
      array_delslot(&(nls[hash]),l);
      l--;
      a=(userdata **)nls[hash].content;
    }
  }
}

void lastlinesplit(void) {
  char *c; int i=0; int hadcolon=0;
  c=&lastline[0]; paramcount=0;
  for (;((*c!='\n') && (*c!='\0') && (*c!='\r') && (paramcount<(MAXPARAMS-2)));c++) {
    if ((hadcolon==0) && (*c==' ')) {
      params[paramcount][i]='\0';
      paramcount++; i=0;
      while (*(c+1)==' ') { c++; }
    } else {
      if ((hadcolon==0) && (*c==':') && (i==0) && (paramcount>1)) {
        hadcolon=1;
      } else {
        params[paramcount][i]=*c;
        i++;
      }
    }
  }
  params[paramcount][i]='\0';
  paramcount++;
}

void writestatstodb(void) {
  serverdata *s; char sqlquery[5000]; long ustot, optot;
  s=sls; ustot=0; optot=0;
  while (s!=NULL) {
    long ushere, ophere;
    ushere=countusersonsrv(s->numeric);
    ophere=countopersonsrv(s->numeric);
    ustot+=ushere;
    optot+=ophere;
    sprintf(sqlquery,"REPLACE INTO stats VALUES('%s','%ld','%ld','%ld','%lu','%ld')",
      s->nick,ushere,ophere,s->createdat,s->connectat,getnettime());
    mysql_query(&sqlcon,sqlquery);
    s=s->next;
  }
  sprintf(sqlquery,"REPLACE INTO stats VALUES('_TOTAL_','%ld','%ld','0','0','%ld')",
    ustot,optot,getnettime());
  mysql_query(&sqlcon,sqlquery);
  /* New entries are in, now lets get rid of old values */
  sprintf(sqlquery,"DELETE FROM stats WHERE TS<'%ld'",getnettime()-300);
  mysql_query(&sqlcon,sqlquery);
}

int main(int argc, char **argv) {
  int whatever; char prevline[TMPSSIZE]; int iii; struct sigaction sa; unsigned long trustres;
  burstcomplete=0; sls=NULL;
  uplinkup=0;
  /* Install signalhandler */
  sa.sa_handler=sighuphandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags=0;
  sigaction(SIGHUP,&sa,NULL);
  stringtoolsinit();
  for (iii=0;iii<SIZEOFUL;iii++) {
    uls[iii]=NULL;
  }
  for (iii=0;iii<SIZEOFCL;iii++) {
    cls[iii]=NULL;
  }
  for (iii=0;iii<SIZEOFTL;iii++) {
    trustedhosts[iii]=NULL;
  }
  for (iii=0;iii<SIZEOFIDMAP;iii++) {
    trustedgroups[iii]=NULL;
  }
  for (iii=0;iii<SIZEOFNL;iii++) {
    array_init(&(nls[iii]),sizeof(userdata *));
    array_setlim1(&(nls[iii]),50);
    array_setlim2(&(nls[iii]),75);
  }
  for (iii=0;iii<SIZEOFCL;iii++) {
    array_init(&(reg[iii]),sizeof(void *));
    array_setlim1(&(reg[iii]),10);
    array_setlim2(&(reg[iii]),15);    
  }
  array_init(&modulelist,sizeof(loadedmod));
  array_setlim1(&modulelist,50);
  array_setlim2(&modulelist,75);
  array_init(&commandlist,sizeof(dyncommands));
  array_setlim1(&commandlist,50);
  array_setlim2(&commandlist,75);
  array_init(&fakecmdlist,sizeof(dynfakecmds));
  array_setlim1(&fakecmdlist,50);
  array_setlim2(&fakecmdlist,75);
  for (iii=0;iii<SIZEOFSHL;iii++) {
    array_init(&serverhandlerlist[iii],sizeof(aserverhandler));
    array_setlim1(&serverhandlerlist[iii],5);
    array_setlim2(&serverhandlerlist[iii],7);
  }
  for (iii=0;iii<SIZEOFSHL;iii++) {
    array_init(&internaleventlist[iii],sizeof(aieventhandler));
    array_setlim1(&internaleventlist[iii],5);
    array_setlim2(&internaleventlist[iii],7);
  }
  for (iii=0;iii<SIZEOFSHL;iii++) {
    array_init(&fchanmsglist[iii],sizeof(fchmsg));
    array_setlim1(&fchanmsglist[iii],5);
    array_setlim2(&fchanmsglist[iii],7);
  }
  array_init(&deniedtrusts,sizeof(trustdeny));
  strcpy(sendglinesto,"");
  numcf=sizeof(chanflags)/sizeof(flags);
  numuf=sizeof(userflags)/sizeof(flags);
  timestdif=0;
  longtotoken(62,servernumeric,2);
  srand((int)time(NULL));
  if (argc>1) {
    putlog("Trying to load config: %s...",argv[1]);
    configfilename=alacstr(argv[1]);
  } else {
    putlog("Trying to load default config xevres.conf...");
    configfilename=alacstr("xevres.conf");
  }
  putlog("%s",(loadconfig(configfilename,0)<0) ? "FAILED" : "done.");
  nicktomsg=(char *)malloc(strlen(mynick)+strlen(myname)+2);
  if (nicktomsg==NULL) {
    printf("Out of memory. BIG BAD CRASH in %s line %d.\n", __FILE__, __LINE__); exit(1);
  }
  strcpy(nicktomsg,mynick);
  strcat(nicktomsg,"@");
  strcat(nicktomsg,myname);
  toLowerCase(nicktomsg);
  putlog("Trying to open SQL-Connection...");
  whatever=opensql(sqlhost,sqluser,sqlpass,sqldb,sqlport);
  if (whatever<0) {
    putlog("Connection to SQL-Server on %s failed. exiting.",sqlhost);
    return -2;
  }
  putlog("SQL-Connection succeeded, executing create-querys...");
  mysql_query(&sqlcon,"CREATE TABLE channels (name varchar(250) NOT NULL, PRIMARY KEY (name), UNIQUE (name))");
  mysql_query(&sqlcon,"CREATE TABLE glines (hostmask varchar(250) NOT NULL, creator varchar(100) NOT NULL, expires bigint(20) DEFAULT '0' NOT NULL, reason varchar(255) NOT NULL, PRIMARY KEY (hostmask))");
  mysql_query(&sqlcon,"CREATE TABLE users (username varchar(40) NOT NULL, password varchar(40) NOT NULL, authlevel int(11) DEFAULT '0' NOT NULL, lastauthed bigint, wantsnotice int DEFAULT 0, noticemask bigint DEFAULT 0, PRIMARY KEY (username), UNIQUE username (username))");
  mysql_query(&sqlcon,"CREATE TABLE realnameglines (mask VARCHAR (150) not null , howtogline INT not null , creator VARCHAR(40) , expires BIGINT , reason VARCHAR (100) not null , PRIMARY KEY (mask), INDEX (mask), UNIQUE (mask))");
  mysql_query(&sqlcon,"CREATE TABLE rnglinelog (id INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, timestamp BIGINT NOT NULL, event VARCHAR(255) NOT NULL, PRIMARY KEY (id))");
  mysql_query(&sqlcon,"CREATE TABLE fakeusers (nick VARCHAR (40) not null , ident VARCHAR (40) not null , host VARCHAR (200) not null , realname VARCHAR (200) not null , PRIMARY KEY (nick), INDEX (nick), UNIQUE (nick))");
  mysql_query(&sqlcon,"CREATE TABLE trustgroups (ID BIGINT not null , name VARCHAR (70) not null , trustedfor INT not null , expires BIGINT not null , maxperident INT not null , enforceident INT not null , contact VARCHAR (150) not null , comment VARCHAR (200) not null , creator VARCHAR(40) not null , lastused BIGINT not null , maxused BIGINT not null , maxreset BIGINT not null , PRIMARY KEY (ID), UNIQUE (ID))");
  mysql_query(&sqlcon,"CREATE TABLE trustedhosts (hostname VARCHAR (40) not null , groupid BIGINT not null , lastused BIGINT not null , maxused BIGINT not null , maxreset BIGINT not null , PRIMARY KEY (hostname), UNIQUE (hostname))");
  mysql_query(&sqlcon,"CREATE TABLE stats (servername VARCHAR(200) NOT NULL, users INT, opers INT, uptime BIGINT, linkup BIGINT, TS BIGINT, PRIMARY KEY (servername), UNIQUE (servername))");
  mysql_query(&sqlcon,"CREATE TABLE trustdenys (network VARCHAR(150) NOT NULL, netmask INT, creator VARCHAR(100) NOT NULL, reason VARCHAR(255) NOT NULL, expires BIGINT, created BIGINT, t INT, PRIMARY KEY(network), UNIQUE(network, netmask))");
  putlog("Tables should have been created if they didn't exist before.");
  for (iii=0;iii<SIZEOFSNC;iii++) {
    ipv4snc[iii]=NULL; // Not really needed, but just to be safe.
    impv4snt[iii]=NULL;
  }
  loadtrustlist(); /* Subnet-Trustlist */
  trustres=loadtrusts();    /* Host-Trustgroups */
  putlog("Loaded %lu trustgroups and %lu trusted hosts from database",trustres>>16,trustres & 0xFFFF);
  loadtrustdeny();
  putlog("Loading Realname-GLINEs...");
  rnglcreate();
  rnglload();
  for (iii=0;iii<256;iii++) {
    array_init(&mainrnl[iii],sizeof(rnlentry));
  }
  putlog("Loading Chanfix-data...");
  loadallchans();
  array_init(&myfakes,sizeof(afakeuser));
  createfakeuser((tokentolong(servernumeric)<<SRVSHIFT)+1,mynick,"xevres",myname,"i am xevres",0);
  array_init(&splitservers,sizeof(splitserv));
  addsplit("dummy_to_prevent_ops_if_net_is_split_and_o_doesnt_know_yet");
  putlog("Connecting to %s Port %u",conto,conport);
  whatever=tcpopen(conto,conport);
  if (whatever<0) {
    putlog("Connection failed.");
    return -1;
  }
  putlog("Connection accepted.");
  sockin=fdopen(whatever,"r");
  sockout=fdopen(whatever,"w");
  sendtouplink("PASS %s\r\n",conpass);
  starttime=time(NULL);
  lastsettime=starttime; waitingforping=0;
  lasthourly=starttime; laststatswrite=starttime;
  lastmin=starttime;
  longtotoken(iptolong(127,0,0,1),tmps1,6);
  sendtouplink("SERVER %s 1 %ld %ld J10 %sA]] 0 +hs :xevres %s\r\n",myname,starttime,starttime,servernumeric,operservversion);
  sendtouplink("%s N %s 1 %ld xevres %s +odkX %s %sAAB :Xevres Rocks %s\r\n",servernumeric,mynick,starttime,myname,tmps1,servernumeric,operservversion);
  sendtouplink("%s EB\r\n",servernumeric);
  uplinkup=1;
  dointernalevents("EB","");
  fflush(sockout);
  /* sync the X usermodes */
  userdata *up;
  up=getudptr((tokentolong(servernumeric)<<SRVSHIFT)+1);
  strcat(up->umode,"odkX");
  /* Register all serverhandlers */
  registerserverhandler("CORE","EB",handleeobmsg);
  registerserverhandler("CORE","N",handlenickmsg);
  registerserverhandler("CORE","Q",handlequitmsg);
  registerserverhandler("CORE","D",handlekillmsg);
  registerserverhandler("CORE","S",handleservermsg);
  registerserverhandler("CORE","G",handleping);
  registerserverhandler("CORE","M",handlemodemsg);
  registerserverhandler("CORE","OM",handlemodemsg);
  registerserverhandler("CORE","P",handleprivmsg);
  registerserverhandler("CORE","SQ",handlesquit);
  registerserverhandler("CORE","B",handleburstmsg);
  registerserverhandler("CORE","C",handlecreatemsg);
  registerserverhandler("CORE","J",handlejoinmsg);
  registerserverhandler("CORE","L",handlepartmsg);
  registerserverhandler("CORE","K",handlekickmsg);
  registerserverhandler("CORE","GL",handleglinemsg);
  registerserverhandler("CORE","R",handlestatsmsg);
  registerserverhandler("CORE","EA",handleeoback);
  registerserverhandler("CORE","T",handletopic);
  registerserverhandler("CORE","W",handleremotewhois);
  registerserverhandler("CORE","242",handlestatsureply);
  registerserverhandler("CORE","217",handlestatsPreply);
  registerserverhandler("CORE","AC",handleaccountmsg);
  registerserverhandler("CORE","CM",handleclearmodemsg);
  /* Registered all serverhandlers */
  do {
    strcpy(prevline,lastline);
    lastline[0]='\0'; command[0]='\0';
    do {
      errno=0;
      fgets(lastline,513,sockin);
      if (igotsighup==1) {
        int muh, bla2;
        muh=errno;
        putlog("Got SIGHUP - executing saveall...");
        saveall();
        putlog("Done.");
        putlog("Rehashing config file...");
        bla2=loadconfig(configfilename, 1);
        if (bla2<0) {
          putlog("FAILED, return code %d!",bla2);
        } else {
          putlog("Success.");
        }
        igotsighup=0; errno=muh;
      }
    } while (errno==EINTR);
    /* Remove the trailing linefeed */
    if (strlen(lastline)>0) {
      delchar(lastline,'\r');
      delchar(lastline,'\n');
    }
    if (lastline[0]!='\0') {
      long cuneti; // Current net time
      cuneti=getnettime();
      if ((lasthourly+3600)<cuneti) {
        putlog("Starting hourly duties...");
        saveall();
        purgeglines();
        splitpurge();
	dointernalevents("60MIN","");
        lasthourly=getnettime();
        putlog("Hourly duties done.");
      }
      if (resyncglinesat!=0) {
        if (resyncglinesat<=cuneti) {
          if (glineautosync==1) { glinesync(); }
          // Time to synch the glines
          resyncglinesat=0;
        }
      }
      if ((lastscan+CFINTERVAL)<cuneti) { /* Time for another scan */
        lastscan=cuneti;
        checkallchans();      
      }
      if ((laststatswrite+300)<cuneti) { /* 5 Minutes since last statswrite */
        laststatswrite=cuneti;
        writestatstodb();
      }
      if ((lastmin+60)<cuneti) { /* 1 minute is over */
        lastmin=cuneti;
        dointernalevents("1MIN","");
      }
      if ((lastsettime+3600)<cuneti) {
        // Last settime was 1 hour ago, time to do it again
        if (waitingforping==0) {
          waitingforping=cuneti;
          pingallservers();
        } else {
          if ((waitingforping+60)<cuneti) {
            // OK, we pinged 60 seconds ago, but still not all replied.
            // don't do settime now. Try again in 5 minutes
            putlog("! Networklag >1 minute! Cannot do settime! Will try again in 5 min");
            lastsettime+=6*60; waitingforping=0;
          }
        }
      }
      lastlinesplit();
      if (paramcount<2) { strcpy(command,""); } else { strcpy(command,params[1]); }
#if DEBUGLEVEL > 10
      putlog("<RX> %s",lastline);
#endif
      if (strcmp(params[0],"SERVER")==0) {
        handleservermsg(1);
        putlog("I am connected to numeric: %s",uplnum);
      } else if (strcmp(params[0],"PASS")==0) {
        putlog("Got password (and dont give a fuck about it)");
      } else {
        somethingtonumeric(params[0],sender);
        doserverhandlers(command);
      }
    }
  } while (lastline[0]!='\0');
  fclose(sockin);
  fclose(sockout);
  putlog("Last line received was: %s",prevline);
  putlog("Closing SQL-Connection...");
  mysql_close(&sqlcon);
  putlog("Cleaning up...");
  while (modulelist.cursi>0) { removemodule(((loadedmod *)(modulelist.content))[0].name); }
  array_free(&modulelist);
  array_free(&commandlist);
  array_free(&fakecmdlist);
  dumpuserlist();
  for (iii=0;iii<SIZEOFNL;iii++) { array_free(&nls[iii]); }
  for (iii=0;iii<SIZEOFCL;iii++) {
    channel *a, *b;
    a=cls[iii];
    while (a!=NULL) {
      b=a;
      a=a->next;
      array_free(&(b->users));
      freeastring(b->name);
      freeastring(b->topic);
      free(b);
    }
  }
  for (iii=0;iii<SIZEOFUL;iii++) {
    userdata *a, *b;
    a=uls[iii];
    while (a!=NULL) {
      b=a;
      a=a->next;
      freeastring(b->host);
      freeastring(b->realname);
      free(b);
    }
  }
  for (iii=0;iii<256;iii++) { array_free(&(mainrnl[iii])); }
  free(nicktomsg);
  putlog("Done.");
  return 0;
}
