/*
Xevres (based on Operservice 2) - general.c
These are general purpose functions used by O or its modules.
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
#include <stdarg.h>
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

#ifndef WE_R_ON_BSD

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <crypt.h>

#endif
 
static unsigned long crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
};
int weareinvisible;

time_t getnettime() {
  return time(NULL)+timestdif;
}

unsigned long parseipv4(char *ip) {
  char b[4][100]; int res; unsigned long c[4]; char dumc;
  if (strlen(ip)>90) { return 0; }
  res=sscanf(ip,"%[1234567890]%c%[1234567890]%c%[1234567890]%c%[1234567890]%c",b[0],&dumc,b[1],&dumc,b[2],&dumc,b[3],&dumc);
  if (res!=7) { return 0; }
  for (res=0;res<4;res++) {
    c[res]=strtoul(b[res],NULL,10);
    if (c[res]>255) { return 0; }
  }
  return (((((c[0]<<8)+c[1])<<8)+c[2])<<8)+c[3];
}

int isvalidipv4(char *ip) {
  char b[4][100]; int res; unsigned long c[4]; char dumc;
  if (strlen(ip)>90) { return 0; }
  res=sscanf(ip,"%[1234567890]%c%[1234567890]%c%[1234567890]%c%[1234567890]%c",b[0],&dumc,b[1],&dumc,b[2],&dumc,b[3],&dumc);
  if (res!=7) { return 0; }
  for (res=0;res<4;res++) {
    c[res]=strtoul(b[res],NULL,10);
    if (c[res]>255) { return 0; }
  }
  return 1;
}

void splitip(unsigned long IP, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d) {
  unsigned long t;
  t=IP;
  *d=t%256; t>>=8; *c=t%256; t>>=8; *b=t%256; t>>=8; *a=t;
}

char * printipv4(unsigned long IPv4) {
  char tmps[40]; unsigned int ipa,ipb,ipc,ipd;
  splitip(IPv4,&ipa,&ipb,&ipc,&ipd);
  sprintf(tmps,"%d.%d.%d.%d",ipa,ipb,ipc,ipd);
  return strdup(tmps);
}

unsigned int clhash(char *txt) {
  return (unsigned int)(crc32(txt)%SIZEOFCL);
}

unsigned int ulhash(long num) {
  return (((unsigned long)num)%SIZEOFUL);
}

unsigned int nlhash(char *nick) {
  return (unsigned int)(crc32(nick)%SIZEOFNL);
}

unsigned int shlhash(char *cmd) {
  return (unsigned int)(crc32(cmd)%SIZEOFSHL);
}

unsigned int strthash(char *s) {
  return (unsigned int)(crc32(s)%STRTHASHSIZE);
}

/* Encrypts pass with a "random" salt, and stores the result in res */
void encryptpwd(char *res, char *pass) {
  char *a; char salt[3];
  salt[2]='\0'; salt[1]=crytok[time(NULL)%64]; salt[0]=crytok[(time(NULL)/424)%64];
  a=crypt(pass,salt);
  strcpy(res,a);
}

/* Checkpassword. Returns 1 if encrypted matches pass, 0 otherwise */
int checkpwd(char *encrypted, char *pass) {
  char *a; char salt[3];
  salt[0]=encrypted[0]; salt[1]=encrypted[1]; salt[2]='\0';
  a=crypt(pass,salt);
  return (strcmp(encrypted,a)==0);
}

/* Next proc is stolen from some library
   http://avrc.city.ac.uk/nethack/nethack-cxref-3.3.1/src/hacklib.c.src.html */
int match2strings(char *patrn, char *strng) {
  char s, p;
  /*
   :  Simple pattern matcher:  '*' matches 0 or more characters, '?' matches
   :  any single character.  Returns TRUE if 'strng' matches 'patrn'.
   */
 pmatch_top:
     s = *strng++;  p = *patrn++; /* get next chars and pre-advance */
     if (!p)                      /* end of pattern */
  return((s == '\0'));           /* matches iff end of string too */
     else if (p == '*')           /* wildcard reached */
  return(((!*patrn || match2strings(patrn, strng-1)) ? 1 : s ? match2strings(patrn-1, strng) : 0));
     else if (p != s && (p != '?' || !s))  /* check single character */
  return 0;           /* doesn't match */
     else                         /* return pmatch(patrn, strng); */
  goto pmatch_top;        /* optimize tail recursion */
}

int isflagset(int v, int f) {
  return ((v & f)==f);
}

void flags2string(int flags, char *buf) {
  if (isflagset(flags, um_o)) {
    strcpy(buf, "@");
  } else {
    strcpy(buf, "_");
  }
  if (isflagset(flags, um_v)) {
    strcat(buf, "+");
  } else {
    strcat(buf, "_");
  }
}

int alldigit(char *s) {
  for (;*s;s++) if (!isdigit(*s)) return(0);
  return(1);
}

unsigned long tokentolong(char* token) {
  long res=0;
  int i=0;
  while (token[i]!='\0') {
    res=res << 6;
    res+=reversetok[(unsigned int)token[i]];
    i++;
  }
  return res;
}

void longtotoken(unsigned long what, char* result, int dig) {
  int i;
  for (i=0;i<dig;i++) {
    result[dig-1-i]=tokens[what%64];
    what>>=6;
  }
  result[dig]='\0';
}

unsigned long iptolong(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
/*  return (((((a*256)+b)*256)+c)*256)+d; */
  return (((((a << 8) ^ b) << 8) ^ c) << 8) ^ d;
}

void toLowerCase(char* a) {
  int j; char *c;
  j=('a'-'A');
  c=a;
  while (*c!='\0') {
    if ((*c>='A') && (*c<='Z')) {
      *c+=j;
    }
    if (*c=='{') { *c='['; }
    if (*c=='}') { *c=']'; }
    if (*c=='|') { *c='\\'; }
    c++;
  }
}

void normnum(char* a) {
  int b;
  b=strlen(a);
  if (b==3) {
    a[4]=a[2];
    a[3]=a[1];
    a[2]='A';
    a[1]=a[0];
    a[0]='A';
    a[5]='\0';
    return;
  }
  if (b==1) {
    a[1]=a[0];
    a[0]='A';
    a[2]='\0';
    return;
  }
  if ((b!=5) && (b!=2)) {
    putlog("Couldn't norm numeric %s",a);
    putlog("DEBUG: lastline=%s\n",lastline);
  }
}

void appchar(char *a, char b) {
  char *c;
  c=a;
  while (*c!='\0') { c++; }
  *c=b; c++; *c='\0';
}

void delchar(char *a, char b) {
  char *c, *d;
  c=a; d=a;
  while (*c!='\0') {
    if (*c!=b) {
      *d=*c;
      c++; d++;
    } else {
      c++;
    }
  }
  *d='\0';
}

void longtoduration(char *dur, long what) {
  long d, h, m, s, tmpl;
  tmpl=what;
  d=(tmpl/86400); tmpl=tmpl%86400;
  h=(tmpl/3600); tmpl=tmpl%3600;
  m=(tmpl/60); tmpl=tmpl%60;
  s=tmpl;
  sprintf(dur,"%4ldd %2ldh %2ldm %2lds",d,h,m,s);
}

long durationtolong(char *dur) {
  long res; char tmps2[TMPSSIZE]; char tmpc;
  strcpy(tmps2,dur);
  tmps2[strlen(tmps2)-1]='\0';
  res=atol(tmps2);
  tmpc=dur[strlen(dur)-1];
  if (tmpc=='s') { return res; }
  if (tmpc=='m') { return (res*60); }
  if (tmpc=='h') { return (res*3600); }
  if (tmpc=='d') { return (res*86400); }
  if (tmpc=='w') { return (res*604800); }
  if (tmpc=='M') { return (res*2592000); }
  if (tmpc=='y') { return (res*31104000); }
  return -2;
}

int ischarinstr(char a, char *b) {
  char *c;
  c=b;
  while (*c!='\0') {
    if (*c==a) { return 1; }
    c++;
  }
  return 0;
}

/* This computes a 32 bit CRC of the data in the buffer, and returns the */
/* CRC.  The polynomial used is 0xedb88320.                              */
/* =============================================================         */
/*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or      */
/*  code or tables extracted from it, as desired without restriction.    */
/* Modified by Fox: no more length parameter, always does the whole string */
unsigned long crc32(const unsigned char *s)
{
  unsigned int i;
  unsigned long crc32val;
  
  crc32val = 0;
  for (i = 0;  s[i]!='\0';  i ++) {
      crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
  }
  return crc32val;
}

/* Takes a client- or server- nick or numeric and converts it into
   SS for servers and SSCCC for clients */
void somethingtonumeric(char *what, char *result) {
  if (what[0]==':') { /* Nick */
    long n;
    n=getsrvnum(&what[1]);
    if (n>=0) {
      longtotoken(n,result,2); return;
    }
    /* So it was no servername. Let's try if we find a client with that nick. */
    toLowerCase(what);
    n=nicktonu2(&what[1]);
    if (n>=0) {
      longtotoken(n,result,5); return;
    }
    putlog("Warning: somethingtonumeric: unknown message-origin: %s",what);
    putlog("DEBUG: lastline=%s\n",lastline);
    strcpy(result,"b0rk");
  } else { /* Numeric */
    int b;
    b=strlen(what);
    if (b==3) {
      result[4]=what[2];
      result[3]=what[1];
      result[2]='A';
      result[1]=what[0];
      result[0]='A';
      result[5]='\0';
      return;
    }
    if (b==1) {
      result[1]=what[0];
      result[0]='A';
      result[2]='\0';
      return;
    }
    if ((b==5) || (b==2)) { strcpy(result,what); return; }
    putlog("Warning: somethingtonumeric: Couldn't norm numeric %s",what);
    strcpy(result,"b0rk");
  }
}

void unescapestring(char *src, char *targ) {
  char *c, *d;
  c=src; d=targ;
  while (*c!='\0') {
    if (*c!='\\') {
      *d=*c; c++; d++;
    } else {
      c++;
      if (*c!='\0') {
        if (*c=='s') { *d=' '; d++; }
        if (*c=='\\') { *d='\\'; d++; }
        c++;
      }
    }
  }
  *d='\0';
}

userdata * getudptr(long numeric) {
  userdata *a;
  a=uls[ulhash(numeric)];
  while (a!=NULL) {
    if (a->numeric==numeric) { return a; }
    a=a->next;
  }
  return NULL;
}

int killuser(long num) {
  userdata *a, *b;
  a=uls[ulhash(num)]; b=NULL;
  while (a!=NULL) {
    if (num==a->numeric) {
      sncdel(a->realip);
      rnldel(a->realname);
      delfromnicklist(a->nick);
      trustdelclient(a->ident,a->realip);
      if (b==NULL) {
        uls[ulhash(num)]=a->next;
      } else {
        b->next=a->next;
      }
      array_free(&(a->chans));
      fakeuserkill(num);
      freeastring(a->host);
      freeastring(a->realname);
      free(a);
      return 1;
    }
    b=a;
    a=a->next;
  }
  return 0;
}

void mystrncpy(char *targ, char *sour, long n) {
  strncpy(targ,sour,n);
  targ[n]='\0';
}

void putlog(const char *template, ...) {
  time_t whatever; char tmps2[TMPSSIZE]; FILE *lf; va_list ap;
  struct tm brt;
  whatever=time(NULL);
  brt=*localtime(&whatever);
  if (strftime(tmps2,TMPSSIZE,logtimestampformat,&brt)==0) {
    if (weareinvisible==0) { printf("You fucked up the timestamp-config!\n"); }
    strcpy(tmps2,"ILLEGAL-TIMESTAMP-FORMAT");
  }
  if (weareinvisible==0) {
    printf("[%s] ",tmps2);
    va_start(ap,template);
    vprintf(template,ap);
    va_end(ap);
    printf("\n");
  }  
  if (strcmp(logfile,"")!=0) {
    lf=fopen(logfile,"a");
    if (lf!=NULL) {
      fprintf(lf,"[%s] ",tmps2);
      va_start(ap,template);
      vfprintf(lf,template,ap);
      va_end(ap);
      fprintf(lf,"\n");
      fclose(lf);
    }
  }
}

int getauthlevel(long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      return(a->authlevel);
    }
    a=a->next;
  }
  return 0;
}

void getauthedas(char *res, long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      strcpy(res,a->authedas);
      return;
    }
    a=a->next;
  }
  strcpy(res,"");
}

int getwantsnotice(long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      return a->wantsnotice;
    }
    a=a->next;
  }
  return 0;
}

int getwantsnotic2(long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      return a->wantsnotice;
    }
    a=a->next;
  }
  return 1;
}

void msgtouser(long unum, char *txt) {
  char target[100];
  longtotoken(unum,target,5);
  if (getwantsnotice(unum)) {
    sendtouplink("%sAAB O %s :%s\r\n",servernumeric,target,txt);
  } else {
    sendtouplink("%sAAB P %s :%s\r\n",servernumeric,target,txt);
  }
  fflush(sockout);
}

void newmsgtouser(long unum, const char *template, ...) {
  char target[100]; va_list ap;
  longtotoken(unum,target,5);
  if (getwantsnotice(unum)) {
    sendtouplink("%sAAB O %s :",servernumeric,target);
  } else {
    sendtouplink("%sAAB P %s :",servernumeric,target);
  }
  va_start(ap,template);
  vfprintf(sockout,template,ap);
  va_end(ap);
  sendtouplink("\r\n");
  fflush(sockout);
}

/* msg from fake */
void msgffake(long unum, char *nick, const char *template, ...) {
  char target[100];
  char tmpnum[6]; va_list ap;
  longtotoken(unum,target,5);
  longtotoken(fake2long(nick),tmpnum,5);
  if (getwantsnotice(unum)) {
    sendtouplink("%s O %s :",tmpnum,target);
  } else {
    sendtouplink("%s P %s :",tmpnum,target);
  }
  va_start(ap,template);
  vfprintf(sockout,template,ap);
  va_end(ap);
  sendtouplink("\r\n");
  fflush(sockout);
}

void cmsgffake(char *chan, char *nick, const char *template, ...) {
  char tmpnum[6]; va_list ap;
  longtotoken(fake2long(nick),tmpnum,5);
  sendtouplink("%s P %s :",tmpnum,chan);
  va_start(ap,template);
  vfprintf(sockout,template,ap);
  va_end(ap);
  sendtouplink("\r\n");
  fflush(sockout);
}

void sendtonoticemask(unsigned long mask, char *txt) {
  autheduser *a;
  if (strlen(txt)>480) { return; }
  a=als;
  while (a!=NULL) {
    if (a->noticemask & mask) { /* This guy wants to be informed */
      if (isircop(a->numeric)) { /* And he is opered, so he is allowed to get info */
        msgtouser(a->numeric,txt);
      }
    }
    a=a->next;
  }
}

unsigned long getnoticemask(long unum) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      return a->noticemask;
    }
    a=a->next;
  }
  return 0;
}

void setnoticemask(long unum, unsigned long mask) {
  autheduser *a;
  a=als;
  while (a!=NULL) {
    if (a->numeric==unum) {
      a->noticemask=mask;
      return;
    }
    a=a->next;
  }
}

int isircop(long num) {
  userdata *a;
  a=uls[ulhash(num)];
  while (a!=NULL) {
    if (a->numeric==num) {
      if (ischarinstr('o',a->umode)) { return 1; } else { return 0; }
    }
    a=a->next;
  }
  return 0;
}

int checkauthlevel(long unum, int minlevel) {
 /* char tmps2[TMPSSIZE]; */
 if (getauthlevel(unum)>=minlevel) {
  return 1;
 } else {
  /* sprintf(tmps2,"This command requires at least authlevel %d (your level: %d)",minlevel,getauthlevel(unum));
     msgtouser(unum,tmps2); 
  */    
  return 0;   
 }
}

serverdata * getsdptr(long numeric) {
  serverdata *s;
  s=sls;
  while (s!=NULL) {
    if (s->numeric==numeric) { return s; }
    s=s->next;
  }
  return NULL;
}

void strreverse(char *s) {
/* (fox) ouch that looks slow. Not that it would be important, but for some
   reason i want this optimized :) */
  int c,i,j;
  for (i=0, j=strlen(s)-1; i < j; i++, j--) {
    c = s[i]; s[i] = s[j]; s[j] = c;
  }
}

char *md5tostr(char *md5) {
  static char returnbuf[33];
  unsigned long *ulp;
  ulp=(unsigned long *)md5;
  sprintf(returnbuf,"%08lx%08lx%08lx%08lx",ulp[0],ulp[1],ulp[2],ulp[3]);
  return returnbuf;
}

void deopall(channel *c) {
  chanuser *b; int j;
  if (c==NULL) { return; }
  sendtouplink("%s CM %s o\r\n",servernumeric,c->name);
  b=(chanuser *)c->users.content;
  for (j=0;j<c->users.cursi;j++) {
    if (isflagset(b[j].flags,um_o)) {
      changechanmod2(c,b[j].numeric,2,um_o);
    }
  }
}

/* Allocate and copy string */
char * alacstr(char *x) {
  char *t;
  t=(char *)malloc(strlen(x)+1);
  if (t!=NULL) { strcpy(t,x); }
  return t;
}

void sendtouplink(const char *template, ...) {
  va_list ap;
#if DEBUGLEVEL > 10
  time_t whatever; int h, m, s;
  whatever=time(NULL);
  h=localtime(&whatever)->tm_hour;
  m=localtime(&whatever)->tm_min;
  s=localtime(&whatever)->tm_sec;
  printf("[%02d:%02d:%02d] <TX> ",h,m,s);
  va_start(ap,template);
  vprintf(template,ap);
  va_end(ap);
#endif
  va_start(ap,template);
  vfprintf(sockout,template,ap);
  va_end(ap);
}

char *unum2nick(long unum) {
 userdata *reqrec;
 reqrec=getudptr(unum);
 return reqrec->nick;
}


/* simulate functions */


void sim_join(char *xchan, long num) {
 channel *ctmp;
 
 if (strcmp(xchan,"0")==0) { 
   deluserfromallchans(num);
   return;
 }
 if (!chanexists(xchan)) 
   newchan(xchan,0);
 addusertochan(xchan,num);
 ctmp=getchanptr(xchan);
 if (ctmp!=NULL) 
   addchantouser2(num,ctmp);
} 

void sim_part(char *xchan, long num) {
 toLowerCase(xchan);
 delchanfromuser(num,xchan);
 deluserfromchan(xchan,num);
}

void sim_topic(char *xchan, char *topic) {
 channel *c;
 
 c=getchanptr(xchan);
 if (c==NULL) { return; }
 /* freeastring(c->topic); */
 strcpy(c->topic,topic);
}

void sim_mode(char *xchan, char *mode, long num) {
 channel *tmpcp;
 toLowerCase(xchan);
 tmpcp=getchanptr(xchan);
 if (tmpcp==NULL) { return; }
 if (mode[0] == '+') {
  if (mode[1] == 'o') {
   changechanmod2(tmpcp,num,1,um_o);
  } else if (mode[1] == 'v') {
   changechanmod2(tmpcp,num,1,um_v);
  }
 } else if (mode[0] == '-') {
  if (mode[1] == 'o') {
   changechanmod2(tmpcp,num,2,um_o);
  } else if (mode[1] == 'v') {
   changechanmod2(tmpcp,num,2,um_v);
  } 
 }
}


