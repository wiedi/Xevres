/*
Operservice 2 - trustcommands.c
Loadable module
(C) Michael Meier 2002 - released under GPL
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
#include "../dlmalloc/malloc.h"
#endif

/* Because the name of the mod is used in several places, we define it here so
   we can easily change it. It should be all lowercase. */
#define MODNAM "trustcommands"
/* Note: This .c-file will have to be named MODNAM.c! */

int trustlistsorthelper(const void *a, const void *b) {
  unsigned long *c, *d;
  c=(unsigned long *)a;
  d=(unsigned long *)b;
  return (*c - *d);
}

unsigned long displaytrustgroupwithid(long unum, unsigned long showid, unsigned int showhosts) {
  long i; trustedhost *th; trustedgroup *tg; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; char * mycip;
  unsigned long linesshown=0;
  tg=findtrustgroupbyID(showid);
  if (tg==NULL) { return linesshown; }
  newmsgtouser(unum,"Trustgroup '%s' (ID #%lu) last changed by %s - currently online: %d",tg->name,tg->id,tg->creator,tg->currentlyon);
  longtoduration(tmps3,tg->expires - getnettime());
  newmsgtouser(unum,"Settings: Enforcing ident: %s - trusted for %d - %d per user - expires in %s",
    (tg->enforceident == 1) ? "yes" : "no ", tg->trustedfor, tg->maxperident, tmps3);
  newmsgtouser(unum,"Contact: %s - Comment: %s",tg->contact,tg->comment);
  longtoduration(tmps3,(getnettime() - tg->lastused)); strcat(tmps3," ago");
  longtoduration(tmps2,(getnettime() - tg->maxreset)); strcat(tmps2," ago");
  newmsgtouser(unum,"Last used: %s - Max usage: %lu - Last Maxuse reset: %s",
    (tg->currentlyon>0) ? "now" : ((tg->lastused==0) ? "never" : tmps3), tg->maxused, (tg->maxreset==0) ? "never" : tmps2);
  if (showhosts==0) { return 4; }
  tmps2[0]=0; linesshown=4;
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      if (th->id==showid) {
        longtoduration(tmps3,(getnettime() - th->lastused)); strcat(tmps3," ago");
        mycip=printipv4(th->IPv4);
        if (tmps2[0]==0) {
          sprintf(tmps2,"%s (Cur: %lu Max: %lu Last: %s)",mycip,
            th->currentlyon,th->maxused,(th->currentlyon>0) ? "now" : ((th->lastused==0) ? "never" : tmps3));
        } else {
          newmsgtouser(unum,"|-%s",tmps2);
          linesshown++;
          sprintf(tmps2,"%s (Cur: %lu Max: %lu Last: %s)",mycip,
            th->currentlyon,th->maxused,(th->currentlyon>0) ? "now" : ((th->lastused==0) ? "never" : tmps3));
        }
        free(mycip);
      }
      th=(void *)th->next;
    }
  }
  if (tmps2[0]==0) {
    msgtouser(unum,"`- no hosts in that group");
  } else {
    newmsgtouser(unum,"`-%s",tmps2);
  }
  linesshown++;
  return linesshown;
}

void dotrustgroupmodify(long unum, char *tail) {
  int res; signed long newv, previous;
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE]; trustedgroup *tg;
  char whats[4][20] = {"Maxclones","Maxperuser","Expiration","Enforceident"}; int whatwedo;
  res = sscanf(tail, "%s %s %30[^+-=1234567890]%30[^\n]", tmps2, tmps3, tmps5, tmps4);
  if (res==2) {
    res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps5);
  }
  if (res < 3) {
    msgtouser(unum, "Syntax: trustgroupmodify <groupname OR #groupid> [what][+|-|=]number");
    msgtouser(unum, "        for the number, +20 means add 20, -15 means subtract 15,");
    msgtouser(unum, "        =35 means it to 35, 20 means add 20.");
    msgtouser(unum, "        what can be one of: maxclones, maxperuser, expire, enforceident");
    msgtouser(unum, "        and defaults to maxclones.");
    return;
  }
  toLowerCase(tmps3);
  if (tmps3[0] == '#') {
    tg = findtrustgroupbyID(strtol(&tmps3[1], NULL, 10));
  } else {
    tg = findtrustgroupbyname(tmps3);
  }
  if (tg == NULL) {
    newmsgtouser(unum, "A trustgroup with that %s does not exist.", (tmps3[0] == '#') ? "ID" : "name");
    return;
  }
  if (tg->id == 0) {
    msgtouser(unum, "Internal error: Trustgroup has ID 0");
    return;
  }
  if (res==3) { /* No "what" argument, we assume maxclones */
    strcpy(tmps4,tmps5);
    strcpy(tmps5,"maxclones");
  }
  toLowerCase(tmps5);
  whatwedo=-1;
  if (strcmp(tmps5,"maxclones")==0) { whatwedo=0; }
  if (strcmp(tmps5,"maxperuser")==0) { whatwedo=1; }
  if (strncmp(tmps5,"expir",5)==0) { whatwedo=2; }
  if (strcmp(tmps5,"maxperident")==0) { whatwedo=1; }
  if (strcmp(tmps5,"enforceident")==0) { whatwedo=3; }
  if (whatwedo<0) {
    newmsgtouser(unum,"Don't know what %s is (and of course not how to change it either).",tmps5);
    return;
  }
  switch (whatwedo) {
    case 0: previous = tg->trustedfor; break;
    case 1: previous = tg->maxperident; break;
    case 2: previous = tg->expires; break;
    case 3: previous = tg->enforceident; break;
    default: msgtouser(unum,"Internal error: programmer too dumb."); return;
  }
  if (tmps4[0] == '=') {
    if (whatwedo==2) {
      newv = durationtolong(tmps4 + 1);
      if (newv<=0) {
        msgtouser(unum,"Invalid duration.");
        return;
      }
      newv+=getnettime();
    } else {
      newv = strtol(tmps4 + 1, NULL, 10);
    }
    if (newv > previous) {
      tmps4[0] = '+';
    } else {
      tmps4[0] = '-';
    }
  } else {
    if (whatwedo==2) {
      if ((tmps4[0]=='-') || (tmps4[0]=='+')) {
        if (tmps4[0]=='-') {
          newv = previous-durationtolong(tmps4 + 1);
        } else {
          newv = previous+durationtolong(tmps4 + 1);
        }
      } else {
        newv=previous+durationtolong(tmps4);
      }
    } else {
      newv = previous + strtol(tmps4, NULL, 10);
    }
  }
  if ((newv<0) || ((newv <= 0) && (whatwedo==1))) {
    msgtouser(unum, "That would make it less or equal to 0 (zero).");
    return;
  }
  if (whatwedo==3) { newv=newv%2; }
  switch (whatwedo) {
    case 0: tg->trustedfor = newv; break;
    case 1: tg->maxperident = newv; break;
    case 2: tg->expires = newv; break;
    case 3: tg->enforceident = newv; break;
    default: msgtouser(unum,"Internal error: programmer too dumb #2."); return;
  }
  getauthedas(tg->creator,unum);
  newmsgtouser(unum, "%s %s from %d to %d.", whats[whatwedo], (tmps4[0] == '-') ? "lowered" : "raised", previous, newv);
  sprintf(tmps2,"%s created/updated Trustgroup %s (#%ld) - %lu / %lu / %d",
    tg->creator,tg->name,tg->id,tg->trustedfor,tg->maxperident,tg->enforceident);
  sendtonoticemask(NM_TRUSTS,tmps2);
}

void dotrustgroupdel(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  trustedgroup *tg;
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res!=2) {
    msgtouser(unum,"Syntax: trustgroupdel <groupname OR #groupid>");
    msgtouser(unum,"This deletes the group including all hosts so be careful with this!");
    return;
  }
  toLowerCase(tmps3);
  if (tmps3[0]=='#') {
    tg=findtrustgroupbyID(strtol(&tmps3[1],NULL,10));
  } else {
    tg=findtrustgroupbyname(tmps3);
  }
  if (tg==NULL) {
    sprintf(tmps2,"A trustgroup with that %s does not exist.",(tmps3[0]=='#') ? "ID" : "name");
    msgtouser(unum,tmps2); return;
  }
  if (tg->id==0) { msgtouser(unum,"Internal error: Trustgroup has ID 0"); return; }
  numtonick(unum,tmps3);
  sprintf(tmps2,"%s deleted Trustgroup %s (#%ld)",tmps3,tg->name,tg->id);
  sendtonoticemask(NM_TRUSTS,tmps2);
  newmsgtouser(unum,"Trustgroup with %d hosts deleted.",destroytrustgroup(tg->id));
  recreateimpsntrusts();
}

void dotrustadd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  trustedgroup *tg; trustedhost *th; long i; userdata *u;
  trustdeny * td; unsigned long theip;
  res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
  if (res!=3) {
    msgtouser(unum,"Syntax: trustadd <groupname OR #groupid> IP"); return;
  }
  toLowerCase(tmps3);
  if (tmps3[0]=='#') {
    tg=findtrustgroupbyID(strtol(&tmps3[1],NULL,10));
  } else {
    tg=findtrustgroupbyname(tmps3);
  }
  if (tg==NULL) {
    sprintf(tmps2,"A trustgroup with that %s does not exist.",(tmps3[0]=='#') ? "ID" : "name");
    msgtouser(unum,tmps2); return;
  }
  if (tg->id==0) { msgtouser(unum,"Internal error: Trustgroup has ID 0"); return; }
  if (!isvalidipv4(tmps4)) { msgtouser(unum,"Not a valid IP(v4)."); return; }
  theip=parseipv4(tmps4);
  if (istrusted(theip)>0) {
    msgtouser(unum,"That IP already belongs to some trustgroup"); return;
  }
  td=gettrustdeny(theip);
  if (td!=NULL) {
    if (td->type==TRUSTDENY_DENY) {
      msgtouser(unum,"Trusting this IP is DENIED!");
    } else { /* Warn only */
      msgtouser(unum,"WARNING: There is a trustdeny-warning for this IP.");
    }
    newmsgtouser(unum,"The entry was last updated by %s, reason: %s",td->creator,td->reason);
    if (td->type==TRUSTDENY_DENY) { return; }
  }
  /* OK so we have a valid group and host - create new hostentry */
  th=(trustedhost *)malloc(sizeof(trustedhost));
  th->IPv4=theip;
  th->id=tg->id;
  th->lastused=0; th->maxused=0; th->maxreset=0; th->currentlyon=0;
  th->next=(void *)trustedhosts[tlhash(theip)];
  trustedhosts[tlhash(theip)]=th;
  /* Recount the host, add it to the trustgroup */
  for (i=0;i<SIZEOFUL;i++) {
    u=uls[i];
    while (u!=NULL) {
      if (theip==u->realip) {
        addtotrustedhost(u->ident,tg->id);
        updatetrustedhost(theip,+1);
      }
      u=(void *)u->next;
    }
  }
  numtonick(unum,tmps3);
  sprintf(tmps2,"%s added IP %s to trustgroup %s (#%lu)",tmps3,tmps4,tg->name,tg->id);
  sendtonoticemask(NM_TRUSTS,tmps2);
  recreateimpsntrusts();
  sprintf(tmps2,"Added IP %s to trustgroup %s (#%lu)",tmps4,tg->name,tg->id);
  msgtouser(unum,tmps2);
}

void dotrustdel(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  trustedgroup *tg; trustedhost *th, *t2; unsigned long theip; int arw=0;
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res!=2) {
    msgtouser(unum,"Syntax: trustdel IP"); return;
  }
  theip=parseipv4(tmps3);
  th=trustedhosts[tlhash(theip)]; t2=NULL;
  while (th!=NULL) {
    if (th->IPv4==theip) {
      long i; userdata *u;
      tg=findtrustgroupbyID(th->id);
      if (t2==NULL) {
        trustedhosts[tlhash(theip)]=(void *)th->next;
      } else {
        t2->next=th->next;
      }
      free(th);
      /* Recount the host */
      for (i=0;i<SIZEOFUL;i++) {
        u=uls[i];
        while (u!=NULL) {
          if (u->realip==theip) {
            int currenton;
            delfromtrustedhost(u->ident,tg->id);
            updatetrustedhost(theip,-1);
            currenton=sncget(theip,32);
            if (currenton>=mf4warn[32]) { /* Clonelimit for that host exceeded */
              if ((mf4warn[32]>=0) && (arw==0)) {
                char * mycip=printipv4(u->realip);
                sprintf(tmps2,"[%d] clones detected from (previously trusted host) %s(=%s)",currenton,u->host,mycip);
                noticeallircops(tmps2);
                putlog("[clones] %d clones detected from (now) untrusted host %s(=%s)",currenton,u->host,mycip);
                free(mycip);
                arw++;
              }
            }
          }
          u=(void *)u->next;
        }
      }
      recreateimpsntrusts();
      numtonick(unum,tmps4);
      sprintf(tmps2,"%s removed trust for IP %s",tmps4,tmps3);
      sendtonoticemask(NM_TRUSTS,tmps2);
      sprintf(tmps2,"IP removed from Trustgroup %s (#%lu)",tg->name,tg->id);
      msgtouser(unum,tmps2);
      return;
    } else {
      t2=th;
      th=(void *)th->next;
    }
  }
  msgtouser(unum,"That IP was not trusted.");
}

void dotrustgroupadd(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  char tmps6[TMPSSIZE], tmps7[TMPSSIZE], tmps8[TMPSSIZE], tmps9[TMPSSIZE];
  long dur; trustedgroup *tg;
  res=sscanf(tail,"%s %s %s %s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6,tmps7,tmps8,tmps9);
  if ((res<7) || (res>8)) {
    msgtouser(unum,"Syntax: trustgroupadd name howmany howlong maxperident enforceident contact [comment]");
    msgtouser(unum,"where name is a unique name for that group");
    msgtouser(unum,"      howmany gives the number of clones allowed from that group");
    msgtouser(unum,"      howlong gives how long the trust will last (e.g. '1y')");
    msgtouser(unum,"      maxperident sets how many clones are allowed from a single user.");
    msgtouser(unum,"                  0 means unlimited");
    msgtouser(unum,"      enforceident if set to 1 will kill all clients without idents");
    msgtouser(unum,"                   in that group");
    msgtouser(unum,"      contact is one or more contact-emails, seperated by commas,");
    msgtouser(unum,"              WITH NO SPACES!!!");
    msgtouser(unum,"      comment is an optional comment");
    return;
  }
  if (res==7) { strcpy(tmps9,"[no comment]"); }
  if (strlen(tmps3)>TRUSTNAMELEN) {
    msgtouser(unum,"The name you gave is too long");
    return;
  }
  if (tmps3[0]=='#') {
    msgtouser(unum,"Trustgroupname must not start with a '#'");
    return;
  }
  if (strlen(tmps8)>TRUSTCONTACTLEN) {
    msgtouser(unum,"contact-email is too long"); return;
  }
  if (strlen(tmps9)>TRUSTCOMLEN) {
    msgtouser(unum,"comment is too long"); return;
  }
  dur=durationtolong(tmps5);
  if (dur<1) {
    msgtouser(unum,"Invalid duration given");
    return;
  }
  if (((tmps7[0]!='0') && (tmps7[0]!='1')) || (tmps7[1]!='\0')) {
    msgtouser(unum,"enforceident is a boolean setting, that means it can only be 0 or 1"); return;
  }
  toLowerCase(tmps3);
  tg=findtrustgroupbyname(tmps3);
  if (tg!=NULL) { /* This group exists, modify it */
    sprintf(tmps2,"Trustgroup %s already exists, replacing values.",tmps3);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"Old settings were: trusted for %lu users, max. clones per user %lu, %senforcing ident",
            tg->trustedfor,tg->maxperident,(tg->enforceident==0) ? "not " : "");
    msgtouser(unum,tmps2);
    sprintf(tmps2,"Contact-eMail: %s",tg->contact);
    msgtouser(unum,tmps2);
    sprintf(tmps2,"Comment:       %s",tg->comment);
    msgtouser(unum,tmps2);
  } else { /* New group */
    unsigned long newID;
    tg=(trustedgroup *)malloc(sizeof(trustedgroup));
    if (tg==NULL) {
      printf("!!! Out of memory in dotrustgroupadd !!!\n"); exit(0);
    }
    newID=getfreetgid();
    tg->id=newID;
    tg->currentlyon=0; tg->lastused=0; tg->maxused=0; tg->maxreset=0;
    tg->next=(void *)trustedgroups[tgshash(newID)];
    trustedgroups[tgshash(newID)]=tg;
    array_init(&(tg->identcounts),sizeof(identcounter));
  }
  tg->enforceident=(tmps7[0]=='1');
  strcpy(tg->name,tmps3);
  strcpy(tg->contact,tmps8);
  strcpy(tg->comment,tmps9);
  tg->trustedfor=strtol(tmps4,NULL,10);
  tg->maxperident=strtol(tmps6,NULL,10);
  tg->expires=getnettime()+dur;
  numtonick(unum,tmps2);
  mystrncpy(tg->creator,tmps2,AUTHUSERLEN);
  sprintf(tmps2,"%s created/updated Trustgroup %s (#%ld) - %lu / %lu / %d",
    tg->creator,tg->name,tg->id,tg->trustedfor,tg->maxperident,tg->enforceident);
  sendtonoticemask(NM_TRUSTS,tmps2);
  sprintf(tmps2,"Trustgroup %s (#%ld) created/updated",tg->name,tg->id);
  msgtouser(unum,tmps2);
}

void dotrustlist(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  array listofids; long i; trustedgroup *tg; trustedhost *th; long j;
  unsigned long lastshown, showid; unsigned long numshown=0;
  char tmps5[TMPSSIZE]; unsigned int showhosts=65000;
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res>3) {
    msgtouser(unum,"Syntax: trustlist [-h] [pattern]");
    return;
  }
  if (res>=2) {
    toLowerCase(tmps3);
    if (strcmp(tmps3,"-h")==0) {
      showhosts=0;
      if (res<3) {
        strcpy(tmps3,"*");
      } else {
        strcpy(tmps3,tmps4);
      }
    } else {
      if (res==3) {
        msgtouser(unum,"Invalid parameter");
        return;
      }
    }
  }
  if (res==1) { strcpy(tmps3,"*"); }
  toLowerCase(tmps3);
  trustgroupexpire();
  if (tmps3[0]=='#') {
    if ((!ischarinstr('*',tmps3)) && (!ischarinstr('?',tmps3))) {
      newmsgtouser(unum,"Searching trustgroup with ID %lu...",strtoul(&tmps3[1],NULL,10));
      numshown=displaytrustgroupwithid(unum,strtoul(&tmps3[1],NULL,10),showhosts);
      if (numshown==0) {
        msgtouser(unum,"A trustgroup with that ID does not exist.");
      }
      return;
    }
  }
  array_init(&listofids,sizeof(unsigned long));
  for (i=0;i<SIZEOFIDMAP;i++) {
    tg=trustedgroups[i];
    while (tg!=NULL) {
      strcpy(tmps4,tg->name); toLowerCase(tmps4);
      sprintf(tmps5,"#%lu",tg->id);
      if ((match2strings(tmps3,tmps4)) || (match2strings(tmps3,tmps5))) {
        j=array_getfreeslot(&listofids);
        ((unsigned long *)listofids.content)[j]=tg->id;
      }
      tg=(void *)tg->next;
    }
  }
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      char * mycip=printipv4(th->IPv4);
      if (match2strings(tmps3,mycip)) {
        j=array_getfreeslot(&listofids);
        ((unsigned long *)listofids.content)[j]=th->id;
      }
      free(mycip);
      th=(void *)th->next;
    }
  }
  if (listofids.cursi==0) {
    msgtouser(unum,"No matches.");
    array_free(&listofids);
    return;
  }
  qsort(listofids.content, listofids.cursi, sizeof(unsigned long), trustlistsorthelper);
  lastshown=0;
  for (i=0;i<listofids.cursi;i++) {
    showid=((unsigned long *)(listofids.content))[i];
    if (showid!=lastshown) {
      lastshown=showid;
      numshown+=displaytrustgroupwithid(unum,showid,showhosts);
      if (numshown>500) {
        newmsgtouser(unum,"More than 500 lines (%lu) already shown - aborting now.",numshown);
        break;
      }
    }
  }
  array_free(&listofids);
  newmsgtouser(unum,"--- End of list - %lu lines returned from query ---",numshown);
}

void dotrustgline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  char tmps6[TMPSSIZE]; long r2; userdata *a;
  trustedgroup *tg; trustedhost *th; int i, j; long l; array listofths;
  res=sscanf(tail, "%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res<4) {
    msgtouser(unum,"Syntax: trustgline <groupname OR #groupID> user duration [reason]");
    msgtouser(unum,"user may contain wildcards");
    return;
  }
  if (res==4) {
    numtonick(unum,tmps2);
    sprintf(tmps6,"requested by %s",tmps2);
  }
  tmps4[USERLEN+2]='\0';
  toLowerCase(tmps3); toLowerCase(tmps4);
  r2=durationtolong(tmps5);
  if (r2<=0) {
    sprintf(tmps2,"The duration you gave is invalid.");
    msgtouser(unum,tmps2);
    return;
  }
  if (tmps3[0]=='#') {
    tg=findtrustgroupbyID(strtol(&tmps3[1],NULL,10));
  } else {
    tg=findtrustgroupbyname(tmps3);
  }
  if (tg==NULL) {
    sprintf(tmps2,"A trustgroup with that %s does not exist.",(tmps3[0]=='#') ? "ID" : "name");
    msgtouser(unum,tmps2); return;
  }
  /* First, create a list of all hosts in that trustgroup... */
  array_init(&listofths,sizeof(unsigned long));
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      if (th->id==tg->id) { /* mmkay, that one belongs to our group */
        long j;
        j=array_getfreeslot(&listofths);
        ((unsigned long *)listofths.content)[j]=th->IPv4;
      }
      th=th->next;
    }
  }
  if (listofths.cursi==0) {
    msgtouser(unum,"There are no hosts in that trustgroup.");
    array_free(&listofths);
    return;
  }
  /* Now that we have the list: count number of users hit */
  l=0;
  for (j=0;j<SIZEOFUL;j++) {
    a=uls[j];
    while (a!=NULL) {
      char tmpu[USERLEN+1];
      strcpy(tmpu,a->ident);
      toLowerCase(tmpu);
      for (i=0;i<listofths.cursi;i++) {
        unsigned long *bla;
        bla=(unsigned long *)listofths.content;
        if (bla[i]==a->realip) {
          if (match2strings(tmps4,tmpu)) {
            l++; break;
          }
        }
      }
      a=(void *)a->next;
    }
  }
  if (l>GLINEMAXHIT) {
    newmsgtouser(unum,"That gline would hit more than %d (%d) Users/Channels. You probably mistyped something.",GLINEMAXHIT,l);
    if ((l<(GLINEMAXHIT*10)) && (getauthlevel(unum)>=999)) {
      msgtouser(unum,"However, your authlevel is >=999, so I hope you know what you're doing... Executing command.");
    } else {
      array_free(&listofths); return;
    }
  }
  /* OK, safety checks done - now gline the fuckers */
  numtonick(unum,tmps5);
  for (i=0;i<listofths.cursi;i++) {
    unsigned long *bla; char * mycip;
    bla=(unsigned long *)listofths.content;
    mycip=printipv4(bla[i]);
    sprintf(tmps2,"%s@%s",tmps4,mycip);
    free(mycip);
    addgline(tmps2,tmps6,tmps5,r2,1);
  }
  sprintf(tmps2,"Added GLINE for %s @ trustgroup %s (#%lu), caused %lu glines, hit %ld users",tmps4,tg->name,tg->id,
    listofths.cursi,l);
  msgtouser(unum,tmps2);
  sprintf(tmps2,"GLINE %s @ trustgroup %s (#%lu) by %s, caused %lu glines, hit %ld users",tmps4,
    tg->name,tg->id,tmps5,listofths.cursi,l);
  sendtonoticemask(NM_GLINE,tmps2);
  array_free(&listofths);
}

void dotrustungline(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  trustedgroup *tg; trustedhost *th; int i; array listofths;
  res=sscanf(tail, "%s %s %s",tmps2,tmps3,tmps4);
  if (res<3) {
    msgtouser(unum,"Syntax: trustungline <groupname OR #groupID> user");
    msgtouser(unum,"user may contain wildcards");
    return;
  }
  tmps4[USERLEN+2]='\0';
  toLowerCase(tmps3); toLowerCase(tmps4);
  if (tmps3[0]=='#') {
    tg=findtrustgroupbyID(strtol(&tmps3[1],NULL,10));
  } else {
    tg=findtrustgroupbyname(tmps3);
  }
  if (tg==NULL) {
    sprintf(tmps2,"A trustgroup with that %s does not exist.",(tmps3[0]=='#') ? "ID" : "name");
    msgtouser(unum,tmps2); return;
  }
  /* First, create a list of all hosts in that trustgroup... */
  array_init(&listofths,sizeof(unsigned long));
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      if (th->id==tg->id) { /* mmkay, that one belongs to our group */
        long j;
        j=array_getfreeslot(&listofths);
        ((unsigned long *)listofths.content)[j]=th->IPv4;
      }
      th=(void *)th->next;
    }
  }
  if (listofths.cursi==0) {
    msgtouser(unum,"There are no hosts in that trustgroup.");
    array_free(&listofths);
    return;
  }
  /* starting ungline.. */
  for (i=0;i<listofths.cursi;i++) {
    char * mycip;
    mycip=printipv4(((unsigned long *)listofths.content)[i]);
    sprintf(tmps2,"%s@%s",tmps4,mycip);
    free(mycip);
    remgline(tmps2,1);
  }
  numtonick(unum,tmps5);
  sprintf(tmps2,"Removed GLINE for %s @ trustgroup %s (#%lu)",tmps4,tg->name,tg->id);
  msgtouser(unum,tmps2);
  sprintf(tmps2,"GLINE %s @ trustgroup %s (#%lu) removed by %s",tmps4, tg->name,tg->id,tmps5);
  sendtonoticemask(NM_GLINE,tmps2);
  array_free(&listofths);
}

void dotrustspew(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  trustedgroup *tg; trustedhost *th; int i; userdata *ud; long k, l;
  array listofths; int iamtired;
  res=sscanf(tail, "%s %s %s",tmps2,tmps3,tmps4);
  if (res<2) {
    msgtouser(unum,"Syntax: trustspew <groupname OR #groupID> [user]");
    return;
  }
  if (res==2) { strcpy(tmps4,"*"); }
  tmps4[USERLEN+2]='\0';
  toLowerCase(tmps3); toLowerCase(tmps4);
  if (tmps3[0]=='#') {
    tg=findtrustgroupbyID(strtol(&tmps3[1],NULL,10));
  } else {
    tg=findtrustgroupbyname(tmps3);
  }
  if (tg==NULL) {
    sprintf(tmps2,"A trustgroup with that %s does not exist.",(tmps3[0]=='#') ? "ID" : "name");
    msgtouser(unum,tmps2); return;
  }
  /* First, create a list of all hosts in that trustgroup... */
  array_init(&listofths,sizeof(unsigned long));
  for (i=0;i<SIZEOFTL;i++) {
    th=trustedhosts[i];
    while (th!=NULL) {
      if (th->id==tg->id) { /* mmkay, that one belongs to our group */
        long j;
        j=array_getfreeslot(&listofths);
        ((unsigned long *)listofths.content)[j]=th->IPv4;
      }
      th=(void *)th->next;
    }
  }
  if (listofths.cursi==0) {
    msgtouser(unum,"There are no hosts in that trustgroup.");
    array_free(&listofths);
    return;
  }
  /* We have the list - scan through the whole userlist, and match against our
     hostlist, and if that matches, match again against the username */
  sprintf(tmps2,"Users in Trustgroup %s (#%lu) matching %s",tg->name,tg->id,tmps4);
  msgtouser(unum,tmps2);
  l=0;
  for (i=0;i<SIZEOFUL;i++) {
    ud=uls[i];
    while (ud!=NULL) {
      for (k=0;k<listofths.cursi;k++) {
        if (((unsigned long *)listofths.content)[k]==ud->realip) {
          /* Host matches, check username */
          strcpy(tmps5,ud->ident);
          toLowerCase(tmps5);
          if (match2strings(tmps4,tmps5)) {
            /* yay ident matches too - list that loser */
            channel **d; int chanmodes; char * mycip;
            l++; /* inc counter of number of users found */
            mycip=printipv4(ud->realip);
            sprintf(tmps2,"%s!%s@%s(=%s) (%s)",ud->nick,ud->ident,ud->host,mycip,ud->realname);
            msgtouser(unum,tmps2);
            if (ud->chans.cursi>0) {
              if (ud->chans.cursi==1) {
                strcpy(tmps2,"On channel:");
              } else {
                strcpy(tmps2,"On channels:");
              }
              for (iamtired=0;iamtired<(ud->chans.cursi);iamtired++) {
                d=(channel **)(ud->chans.content);
                if ((strlen(d[iamtired]->name)+strlen(tmps2))>400) {
                  strcat(tmps2," [...]"); break;
                } else {
                  chanmodes=getchanmode2(d[iamtired],ud->numeric);
                  strcat(tmps2," ");
                  if (chanmodes<0) {
                    strcat(tmps2,"?");
                  } else {
                    if (isflagset(chanmodes,um_o)) { strcat(tmps2,"@"); } else { strcat(tmps2," "); }
                  }
                  strcat(tmps2,d[iamtired]->name);
                }
              }
            } else {
              strcpy(tmps2,"Not on any (nonlocal-)channels");
            }
            msgtouser(unum,tmps2);
          }
        }
      }
      ud=(void *)ud->next;
    }
  }
  sprintf(tmps2,"--- End of list - %ld matches ---",l);
  msgtouser(unum,tmps2);
  array_free(&listofths);
}

void dotrustdenyadd(long unum, char *tail) {
  int res; long i; trustdeny *td; long expires;
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  unsigned long snet, smask;
  res=sscanf(tail,"%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res<5) {
    msgtouser(unum,"Syntax: trustdenyadd warn|deny subnet[/netmask] duration reason");
    msgtouser(unum,"if netmask is omitted, then /32 (a single IP) is assumed.");
    return;
  }
  toLowerCase(tmps3);
  if (strcmp(tmps3,"warn")==0) { res=TRUSTDENY_WARN; }
  if (strcmp(tmps3,"deny")==0) { res=TRUSTDENY_DENY; }
  if ((res!=TRUSTDENY_WARN) && (res!=TRUSTDENY_DENY)) {
    msgtouser(unum,"You can only warn or deny.");
    return;
  }
  if (!ischarinstr('/',tmps4)) {
    smask=32;
    snet=parseipv4(tmps4);
  } else {
    char h1[TMPSSIZE]; char h2[TMPSSIZE]; char dumc; int r2;
    r2=sscanf(tmps4,"%[0-9.]%c%[0-9]",h1,&dumc,h2);
    if (r2!=3) { msgtouser(unum,"Invalid subnetmask."); return; }
    snet=parseipv4(h1);
    smask=strtoul(h2,NULL,10);
    if (smask>32) { msgtouser(unum,"Invalid subnetmask."); return; }
  }
  expires=durationtolong(tmps5);
  if (expires<1) {
    msgtouser(unum,"Invalid duration.");
    return;
  }
  expires+=getnettime();
  td=(trustdeny *)deniedtrusts.content;
  for (i=0;i<deniedtrusts.cursi;i++) {
    if ((td[i].v4net==snet) && (td[i].v4mask=smask)) {
      longtoduration(tmps2,getnettime()-td[i].created);
      longtoduration(tmps3,td[i].expires-getnettime());
      msgtouser(unum,"a trustdeny for that hostmask already exists - replacing values");
      newmsgtouser(unum,"Old one was created by %s %s ago, expiring in %s and mode %s",
        td[i].creator,tmps2,tmps3,(td[i].type==TRUSTDENY_WARN) ? "WARN" : "DENY");
      getauthedas(tmps2,unum);
      mystrncpy(td[i].creator,tmps2,AUTHUSERLEN);
      mystrncpy(td[i].reason,tmps6,RNGREAS);
      td[i].expires=expires;
      td[i].created=getnettime();
      td[i].type=res;
      msgtouser(unum,"Done.");
      return;
    }
  }
  /* Not existing yet - allocate new entry */
  i=array_getfreeslot(&deniedtrusts);
  td=(trustdeny *)deniedtrusts.content;
  getauthedas(tmps2,unum);
  mystrncpy(td[i].creator,tmps2,AUTHUSERLEN);
  mystrncpy(td[i].reason,tmps6,RNGREAS);
  td[i].expires=expires;
  td[i].created=getnettime();
  td[i].v4net=snet;
  td[i].v4mask=smask;
  td[i].type=res;
  recreateimpsntrusts();
  msgtouser(unum,"Done.");
}

void dotrustdenylist(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; long i, j; trustdeny *td;
  unsigned long snet, smask; char * mycip;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) { strcpy(tmps3,"0.0.0.0/0"); }
  if (!ischarinstr('/',tmps3)) {
    smask=32;
    snet=parseipv4(tmps3);
  } else {
    char h1[TMPSSIZE]; char h2[TMPSSIZE]; char dumc; int r2;
    r2=sscanf(tmps3,"%[0-9.]%c%[0-9]",h1,&dumc,h2);
    if (r2!=3) { msgtouser(unum,"Invalid subnetmask."); return; }
    snet=parseipv4(h1);
    smask=strtoul(h2,NULL,10);
    if (smask>32) { msgtouser(unum,"Invalid subnetmask."); return; }
  }
  trustdenyexpire();
  td=(trustdeny *)deniedtrusts.content;
  j=0;
  for (i=0;i<deniedtrusts.cursi;i++) {
    if (((td[i].v4net&netmasks[smask])==(snet&netmasks[smask])) ||
        ((td[i].v4net&netmasks[td[i].v4mask])==(snet&netmasks[td[i].v4mask]))) {
      if (j==0) {
        newmsgtouser(unum,"%-30s %-15s %-17s %-1s %s","IP/mask","by","expires in","t","Reason");
      }
      longtoduration(tmps2,td[i].expires-getnettime());
      mycip=printipv4(td[i].v4net);
      newmsgtouser(unum,"%-27s/%02u %-15s %-17s %s %s",mycip,td[i].v4mask,td[i].creator,
        tmps2,(td[i].type==TRUSTDENY_DENY) ? "D" : "W", td[i].reason);
      free(mycip);
      j++;
    }
  }
  if (j==0) {
    msgtouser(unum,"--- No matches ---");
  } else {
    newmsgtouser(unum,"--- End of list - %ld matches ---",j);
  }
}

void dotrustdenydel(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; long i; trustdeny * td;
  unsigned long snet, smask;
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: trustdenydel subnet/[netmask]");
    msgtouser(unum,"if netmask is omitted, then /32 (a single IP) is assumed.");
    return;
  }
  if (!ischarinstr('/',tmps3)) {
    smask=32;
    snet=parseipv4(tmps3);
  } else {
    char h1[TMPSSIZE]; char h2[TMPSSIZE]; char dumc; int r2;
    r2=sscanf(tmps3,"%[0-9.]%c%[0-9]",h1,&dumc,h2);
    if (r2!=3) { msgtouser(unum,"Invalid subnetmask."); return; }
    snet=parseipv4(h1);
    smask=strtoul(h2,NULL,10);
    if (smask>32) { msgtouser(unum,"Invalid subnetmask."); return; }
  }
  td=(trustdeny *)deniedtrusts.content;
  for (i=0;i<deniedtrusts.cursi;i++) {
    if ((td[i].v4net==snet) && (td[i].v4mask==smask)) {
      array_delslot(&deniedtrusts,i);
      msgtouser(unum,"trustdeny removed.");
      return;
    }
  }
  msgtouser(unum,"No such trustdeny");
}

void trustcommands_init(void) {
  setmoduledesc(MODNAM,"This mod provides the trust*-commands");
  registercommand2(MODNAM,"trustgroupadd", dotrustgroupadd, 1, 900,
    "trustgroupadd\tAdds a new or modifies an existing trustgroup");
  registercommand2(MODNAM,"trustgroupmodify", dotrustgroupmodify, 1, 900,
    "trustgroupmodify name [what][+|-|=]n\tChanges settings for a trustgroup");
  registercommand2(MODNAM,"trustgroupdel", dotrustgroupdel, 1, 900,
    "trustgroupdel name|#gid\tDeletes a trustgroup (be careful with that!)");
  registercommand2(MODNAM,"trustadd", dotrustadd, 1, 900,
    "trustadd name|#gid IP\tAdds a host to a trustgroup");
  registercommand2(MODNAM,"trustdel", dotrustdel, 1, 900,
    "trustdel IP\tRemoves a host from a trustgroup");
  registercommand2(MODNAM,"trustlist", dotrustlist, 1, 0,
    "trustlist [-h] [pattern]\tLists matching or all trustgroups/hosts");
  registercommand2(MODNAM,"trustgline", dotrustgline, 1, 900,
    "trustgline ...\tGLINEs a user on all hosts of a trustgroup");
  registercommand2(MODNAM,"trustungline", dotrustungline, 1, 900,
    "trustungline ...\tRemoves GLINEs of a user on all hosts of a trustgroup");
  registercommand2(MODNAM,"trustspew", dotrustspew, 1, 0,
    "trustspew group [user]\tLists users on a trustgroup");
  registercommand2(MODNAM,"trustdenyadd", dotrustdenyadd, 1, 900,
    "trustdenyadd ...\tAdds a trustdeny to a hostmask");
  registercommand2(MODNAM,"trustdenylist", dotrustdenylist, 1, 0,
    "trustdenylist [pattern]\tLists existing trustdenys");
  registercommand2(MODNAM,"trustdenydel", dotrustdenydel, 1, 900,
    "trustdenydel subnet/mask\tRemoves a trustdeny");
}

/* This is called for cleanup (before the module gets unloaded)
   Again the name of the function has to be MODNAM_cleanup */
void trustcommands_cleanup(void) {
  deregistercommand("trustgroupadd");
  deregistercommand("trustgroupmodify");
  deregistercommand("trustgroupdel");
  deregistercommand("trustadd");
  deregistercommand("trustdel");
  deregistercommand("trustlist");
  deregistercommand("trustgline");
  deregistercommand("trustungline");
  deregistercommand("trustspew");
  deregistercommand("trustdenyadd");
  deregistercommand("trustdenylist");
  deregistercommand("trustdenydel");
}
