/*
Xevres (based on Operservice 2)
subnet counting functions.
Note: This file previously contained functions for handling /24 subnets.
      It now contains functions for handling the O subnet counts.
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
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

/* This is the way to go. New functions. */

/* internal use only */
unsigned long snchashf(unsigned long key) {
  return (key % SIZEOFSNC);
}

subnetcount * getnewsnc() {
  subnetcount * res;
  res=(subnetcount *)malloc(sizeof(subnetcount));
  res->count=0; res->succ0=NULL; res->succ1=NULL;
  return res;
}

subnetcount * sncfindsubnetstart(subnetchelp * ipv4snc[], unsigned long IP, int createifnecessary) {
  unsigned long tmpl; unsigned long thenet; subnetchelp * sh;
  tmpl=snchashf(IP >> ipv4sncstartmask);
  sh=ipv4snc[tmpl];
  thenet=(IP & netmasks[ipv4sncstartmask]);
  while (sh!=NULL) {
    if (thenet==sh->sn) {
      return sh->snc;
    }
    sh=sh->next;
  }
  if (createifnecessary==0) { return NULL; }
  sh=(subnetchelp *)malloc(sizeof(subnetchelp));
  sh->sn=thenet;
  sh->next=ipv4snc[tmpl];
  ipv4snc[tmpl]=sh;
  sh->snc=getnewsnc();
  return sh->snc;
}

void snckillsubnetstart(subnetchelp * ipv4snc[], unsigned long IP) {
  unsigned long tmpl; unsigned long thenet; subnetchelp * sh; subnetchelp * sh2;
  tmpl=snchashf(IP >> ipv4sncstartmask);
  sh=ipv4snc[tmpl]; sh2=NULL;
  thenet=(IP & netmasks[ipv4sncstartmask]);
  while (sh!=NULL) {
    if (thenet==sh->sn) {
      if (sh2==NULL) {
        ipv4snc[tmpl]=sh->next;
      } else {
        sh2->next=sh->next;
      }
      free(sh);
      return;
    }
    sh2=sh;
    sh=sh->next;
  }
}

void killimplsntrust(subnetcount * snc) {
  if (snc->succ0!=NULL) { killimplsntrust(snc->succ0); }
  if (snc->succ1!=NULL) { killimplsntrust(snc->succ1); }
  free(snc);
  return;
}

void impsntadd(unsigned long IP, unsigned long n) {
  int i; subnetcount * s; unsigned long iptmp;
  s=sncfindsubnetstart(impv4snt,IP,1);
  iptmp=IP<<ipv4sncstartmask;
  for (i=ipv4sncstartmask;i<=32;i++) {
    s->count+=n;
    if (i<32) {
      if ((iptmp&0x80000000LU)) {
        if (s->succ1==NULL) { s->succ1=getnewsnc(); }
        s=s->succ1;
      } else {
        if (s->succ0==NULL) { s->succ0=getnewsnc(); }
        s=s->succ0;
      }
    }
    iptmp=iptmp<<1;
  }
}

/* These functions are public and "exported" in globals.h */
/* This function increases the subnet counters. If res is non-null, it has
   to be a pointer to an unsigned long [33] (not 32!) array, that will
   be filled with the current data for all this IPs subnets. Values that
   are unknown (because the subnetsize is above ipv4sncstartmask) will
   be filled with 0. */
void sncadd(unsigned long IP, unsigned long *res) {
  int i; subnetcount * s; unsigned long iptmp;
  ipv4usercount++;
  if (res!=NULL) {
    res[0]=ipv4usercount;
    for (i=1;i<ipv4sncstartmask;i++) { res[i]=0; }
  }
  s=sncfindsubnetstart(ipv4snc,IP,1);
  iptmp=IP<<ipv4sncstartmask;
  for (i=ipv4sncstartmask;i<=32;i++) {
    s->count++;
    if (res!=NULL) { res[i]=s->count; }
    if (i<32) {
      if ((iptmp&0x80000000LU)) {
        if (s->succ1==NULL) { s->succ1=getnewsnc(); }
        s=s->succ1;
      } else {
        if (s->succ0==NULL) { s->succ0=getnewsnc(); }
        s=s->succ0;
      }
    }
    iptmp=iptmp<<1;
  }
}

void sncdel(unsigned long IP) {
  int i; subnetcount * s; subnetcount * s2=NULL; unsigned long iptmp;
  ipv4usercount--;
  s=sncfindsubnetstart(ipv4snc,IP,0);
  if (s==NULL) { return; }
  iptmp=IP<<ipv4sncstartmask;
  if (s->count==1) {
    snckillsubnetstart(ipv4snc,IP);
  }
  for (i=ipv4sncstartmask;i<=32;i++) {
    if (s==NULL) { return; }
    s->count--;
    if ((s2!=NULL) && (s->count==0)) {
      if (s2->succ1==s) { s2->succ1=0; }
      if (s2->succ0==s) { s2->succ0=0; }
    }
    s2=s;
    if ((iptmp&0x80000000LU)) {
      s=s->succ1;
    } else {
      s=s->succ0;
    }
    iptmp=iptmp<<1;
    if (s2->count==0) { free(s2); }
  }
}

unsigned long sncget(unsigned long IP, unsigned long mask) {
  subnetcount * s; int i; unsigned long iptmp;
  if (mask==0) { return ipv4usercount; }
  if (mask>32) { return 0; }
  if (mask<ipv4sncstartmask) { return 0; }
  s=sncfindsubnetstart(ipv4snc,IP,0);
  iptmp=IP<<ipv4sncstartmask;
  for (i=ipv4sncstartmask;i<mask;i++) {
    if (s==NULL) { return 0; }
    if ((iptmp&0x80000000LU)) {
      s=s->succ1;
    } else {
      s=s->succ0;
    }
    iptmp=iptmp<<1;
  }
  if (s==NULL) { return 0; } else { return s->count; }
}

void recreateimpsntrusts(void) {
  int i; subnetchelp * snch; subnetchelp * tmp;
  trustedhost * th; trustedgroup * tg;
  for (i=0;i<SIZEOFSNC;i++) {
    snch=impv4snt[i];
    while (snch!=NULL) {
      if (snch->snc!=NULL) { killimplsntrust(snch->snc); }
      tmp=snch;
      snch=snch->next;
      free(tmp);
    }
    impv4snt[i]=NULL;
  }
  /* List is now cleared, recreate it from scratch */
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      tg=findtrustgroupbyID(th->id);
      if (tg!=NULL) {	impsntadd(th->IPv4,tg->trustedfor); }
      th=th->next;
    }
  }
}

/* This function returns the trustcounts for the subnets belonging to the IP.
   res has to be a pointer to an unsigned long [33] (not 32!) array, that will
   be filled with the current data for all this IPs subnets. Values that
   are unknown (because the subnetsize is above ipv4sncstartmask) will
   be filled with 0. */
void getimpsntrusts(unsigned long IP, unsigned long *res) {
  int i; subnetcount * s; unsigned long iptmp;
  if (res==NULL) { return; }
  for (i=0;i<ipv4sncstartmask;i++) { res[i]=0; }
  s=sncfindsubnetstart(impv4snt,IP,0);
  iptmp=IP<<ipv4sncstartmask;
  for (i=ipv4sncstartmask;i<=32;i++) {
    if (s==NULL) {
      res[i]=0;
    } else {
      res[i]=s->count;
      if ((iptmp&0x80000000LU)) {
        s=s->succ1;
      } else {
        s=s->succ0;
      }
    }
    iptmp=iptmp<<1;
  }
}

/* End of new functions */
