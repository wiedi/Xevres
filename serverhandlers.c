/*
Xevres (based on Operservice 2) - serverhandlers.c
This baby handles all commands received from the server.
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
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

void handlenickmsg() {
  unsigned long tmpl; long thisisnotgood;
  int slen; userdata *a;
  slen=strlen(sender);
  if (slen==5) {
    // Nickchange
    if (paramcount!=4) {
      putlog("!!! Nickchange failed (wrong # args): %s !!!",lastline);
      return;
    }
    if (strlen(params[2])>NICKLEN) {
      putlog("!!! handlenickmsg/ren: Nick exceeds NICKLEN: %s !!!",lastline);
    }
    toLowerCase2(params[2]);
    tmpl=tokentolong(sender);
    a=getudptr(tmpl);
    if (a==NULL) {
      putlog("!!! Nickchange failed (no such user): %s !!!",lastline);
      return;
    }
    thisisnotgood=nicktonu2(params[2]);
    if (thisisnotgood>=0) {
      userdata *b;
      b=getudptr(thisisnotgood);
      if (tmpl==thisisnotgood) { return; /* WTF was that? :luser NICK luser ?! */ }
      if (b==NULL) {
        putlog("!!! Nick change collission: Fucked up Hashtable?! !!!");
        return;
      }
      if (b->connectat==a->connectat) {
        /* Equal timestamps, Kill both */
        putlog("!!! Nick change collission: %s -> %s - same timestamp! !!!",
          a->nick,params[2]);
        deluserfromallchans(tmpl);
        delautheduser(tmpl);
        if (!killuser(tmpl)) {
          fakeuserkill(tmpl);
          putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
        }
        deluserfromallchans(thisisnotgood);
        delautheduser(thisisnotgood);
        if (!killuser(thisisnotgood)) {
          fakeuserkill(thisisnotgood);
          putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
        }
        return;
      } else {
        /* Is it the same luser? */
        if ((a->realip==b->realip) && (strcmp(a->ident,b->ident)==0)) {
          /* Yes, so kill the old ghost */
          putlog("!!! Nick change collission: %s -> %s - same uhost, old user killed !!!",
            a->nick,params[2]);
          deluserfromallchans(thisisnotgood);
          delautheduser(thisisnotgood);
          if (!killuser(thisisnotgood)) {
            fakeuserkill(thisisnotgood);
            putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
          }
        } else {
          /* No, so kill the new luser */
          putlog("!!! Nick change collission: %s -> %s - different uhost, new user killed !!!",
            a->nick,params[2]);
          deluserfromallchans(tmpl);
          delautheduser(tmpl);
          if (!killuser(tmpl)) {
            fakeuserkill(tmpl);
            putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
          }
        }
      }
    }
    delfromnicklist(a->nick);
    strcpy(a->nick,params[2]);
    addtonicklist(a);
    a->connectat=strtoul(params[3],NULL,10);
  }
  if (slen==2) {
    // New Nick introduced to the network
    int rnstart; int authnamepos=0;
    char umode[TMPSSIZE]; char numtmp[TMPSSIZE]; char realiptmp[TMPSSIZE];
    int res; unsigned long sncs[33]; unsigned long snts[33];
    if (paramcount<10) {
      putlog("!!! Nick introduction failed (wrong # args): %s !!!",lastline);
      return;
    }
    rnstart=(paramcount-1);
    if (paramcount==10) {
    	strcpy(umode,"+");
    } else {
    	int ti1, ti2;
    	strcpy(umode,params[7]);
    	ti1=0; ti2=0;
    	for (ti1=0;umode[ti1]!=0;ti1++) {
    		if (umode[ti1]=='h') { ti2++; }
    		if (umode[ti1]=='r') { authnamepos=ti2+8; ti2++; }
    	}
    }
    strcpy(numtmp,params[rnstart-1]);
    strcpy(realiptmp,params[rnstart-2]);
    normnum(numtmp);
    tmpl=tokentolong(realiptmp);  /* FIXMEFORIPV6 */
    toLowerCase2(params[2]);
    thisisnotgood=nicktonu2(params[2]);
    if (thisisnotgood>=0) { /* Nick collission */
      long ting2; userdata *ting3;
      ting2=strtoul(params[4],NULL,10);
      ting3=getudptr(thisisnotgood);
      if (ting3==NULL) {
        putlog("!!! Nick collission: Fucked up hashtable?! !!!");
        return;
      }
      /* Update: Fuck me, i should have read the ircd source earlier :)
       * If the TimeStamps are equal, we kill both (or only 'new'
       * if it was a ":server NICK new ...").
       * Otherwise we kill the youngest when user@host differ,
       * or the oldest when they are the same.  */
      if (ting2==ting3->connectat) {
        /* We cannot decide who was here earlier, so we have to kill both */
        /* Or rather: we kill the old and do not add the new one */
        putlog("!!! Nick collission: %s - same timestamp !!!",ting3->nick);
        deluserfromallchans(thisisnotgood);
        delautheduser(thisisnotgood);
        if (!killuser(thisisnotgood)) {
          fakeuserkill(thisisnotgood);
          putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
        }
        return;
      }
      if ((strcmp(params[5],ting3->ident)==0) && (tmpl==ting3->realip)) {
        /* This guy collided himself, which means the old client gets killed */
        putlog("!!! Nick collission: %s collided itself (older client killed) !!!",ting3->nick);
        deluserfromallchans(thisisnotgood);
        delautheduser(thisisnotgood);
        if (!killuser(thisisnotgood)) {
          fakeuserkill(thisisnotgood);
          putlog("!!! KILL failed on nickcollide: %s !!!",lastline);
        }
        /* No return here, so the new client gets added! */
      } else {
        /* the new client gets killed - so no need to add him at all, just return */
        putlog("!!! Nick collission: %s overriden by old client !!!",ting3->nick);
        return;
      }
    }
    /* Check for enforced ident on trusted host - in that case KILL immeadiately */
    if (strlen(params[6])>HOSTLEN) {
      putlog("!!! Hostname exceeds HOSTLEN: %s !!!",lastline);
      return;
    }
    if (params[5][0]=='~') {
      unsigned long thid;
      thid=istrusted(tmpl);
      if (thid>0) {
        trustedgroup *tg;
        tg=findtrustgroupbyID(thid);
        if (tg!=NULL) {
          if (tg->enforceident==1) {
            /* Kill the loser! */
            char mask[TMPSSIZE]; char * mycip;
            mycip=printipv4(tmpl);
            sendtouplink("%s D %s :%s IDENTD required from your host\r\n",servernumeric,
              numtmp, myname);
            putlog("[Missing IDENTD] Killed %s!%s@%s (%s)",params[2],params[5],params[6],mycip);
            /* Add a short GLINE for ~*@host... This also fflush()es the kill */
            sprintf(mask,"~*@%s",mycip);
            free(mycip);
            addgline(mask, "IDENTD required from your host", "MissingIDENT", RNGTIM, 1);
            return;
          }
        }
      }
    }
    /* FIXMEFORIPV6 */
    sncadd(tmpl,(burstcomplete>0) ? sncs : NULL);
    if (burstcomplete>0) {
      int mi;
      getimpsntrusts(tmpl,snts);
      for (mi=32;mi>=ipv4sncstartmask;mi--) {
        if (mf4warn[mi]<=0) { continue; }
        if ((sncs[mi]>=(mf4warn[mi]+snts[mi])) && (((sncs[mi]-(mf4warn[mi]+snts[mi]))%mf4warnsteps[mi])==0)) {
          char *mycip; char tmps2[TMPSSIZE]; char clgmask[TMPSSIZE];
          mycip=printipv4(tmpl&netmasks[mi]);
          clgmask[0]=0;
          if (glineonclones>0) {
          	if ((mi==32) || (glineonclones>1)) {
          		sprintf(clgmask,"*@%s/%d",mycip,mi);
          	}
          }
          if ((mi==32) && (strcmp(params[6],mycip)!=0)) {
            sprintf(tmps2,"Clones: %lu clients connected from %s/%d (%s)%s",sncs[mi],mycip,mi,params[6],
              (clgmask[0]!=0) ? " - AUTO-GLINED!" : "");
          } else {
            sprintf(tmps2,"Clones: %lu clients connected from %s/%d%s",sncs[mi],mycip,mi,
              (clgmask[0]!=0) ? " - AUTO-GLINED!" : "");
          }
          free(mycip);
          if (clgmask[0]!=0) {
          	addgline(clgmask, (mi==32) ? "Too many clones from your host"
          	                           : "Too many connections from your subnet",
          	         "TooManyClones", RNGTIM, 1);
          }
          noticeallircops(tmps2);
          putlog("%s",tmps2);
          break;
        }
      }
    }
    a=(userdata *) malloc(sizeof(userdata));
    a->next=NULL;
    a->hopsaway=atoi(params[3]);
    a->connectat=strtoul(params[4],NULL,10);
    a->realip=tokentolong(realiptmp);
    a->numeric=tokentolong(numtmp);
    if (authnamepos>0) { /* authed user */
      if (authnamepos>=paramcount) {
      	putlog("!!! Something is fucked here - authname and realname overlap! %s !!!",lastline);
      	free(a); return;
      }
      if (strlen(params[authnamepos])>NICKLEN) {
        putlog("!!! Authname exceeds NICKLEN: %s !!!",lastline);
        free(a); return;
      }
      strcpy(a->authname,params[authnamepos]);
    } else { /* not authed */
      a->authname[0]='\0';
    }
    if (strlen(params[2])>NICKLEN) {
      putlog("!!! Nick exceeds NICKLEN: %s !!!",lastline);
      free(a); return;
    } else {
      strcpy(a->nick,params[2]);
    }
    if (strlen(params[5])>USERLEN) {
      putlog("!!! Ident exceeds USERLEN: %s !!!",lastline);
      free(a); return;
    } else {
      strcpy(a->ident,params[5]);
    }
    if (strlen(&umode[1])>MAXUMODES) {
      putlog("!!! User has too many Usermodes: %s !!!",lastline);
      free(a); return;
    } else {
      strcpy(a->umode,&umode[1]);
    }
    if (strlen(params[rnstart])>REALLEN) {
      free(a); return;
      putlog("!!! Realname exceeds REALLEN: %s !!!",lastline);
    } else {
      a->realname=getastring(params[rnstart]);
    }
    a->host=getastring(params[6]);
    array_init(&(a->chans),sizeof(channel *));
    array_setlim1(&(a->chans),22);
    array_setlim2(&(a->chans),30);
    rnglcheck(a);
    res=rnladd(params[rnstart]);
    // Add warning if this realname is there too often
    trustnewclient(a->ident,a->realip);
    addnicktoul(a);
    addtonicklist(a);
    setmd5(a);
  }
}

void handlequitmsg() {
  long tmpl;
  // We have a QUIT numeric
  tmpl=tokentolong(sender);
  deluserfromallchans(tmpl);
  delautheduser(tmpl);
  if (!killuser(tmpl)) {
    putlog("!!! Deluser failed: %s !!!",lastline);
  }
}

void handleaccountmsg() {
  long unum; userdata *up;
  if (paramcount<4) {
    putlog("!!! Failed to parse ACCOUNT message: %s !!!",lastline);
    return;
  }    
  unum=tokentolong(params[2]);
  up=getudptr(unum);
  if (up==NULL) {
    putlog("!!! ACCOUNT for bad numeric %s !!!",lastline);
    return;
  }
  if (strlen(params[3])>NICKLEN) {
    putlog("!!! ACCOUNT with authname too long: %s !!!",lastline);
    return;
  } 
  if (up->authname[0]!='\0') {
    putlog("!!! ACCOUNT for already authed user: %s !!!",lastline);
    return;
  }
  strcpy(up->authname,params[3]);
  strcat(up->umode,"r");
  changemd5(up);
}

void handleservermsg(int firsts) {
  int xofs; char tmpconnectedto[TMPSSIZE], tmps2[TMPSSIZE];
  if (firsts==1) {
    xofs=0;
    strcpy(tmpconnectedto,servernumeric);
    if (paramcount<7) { putlog("!!! Failed to parse Servermessage: %s !!!",lastline); return; }
  } else {
    xofs=1;
    strcpy(tmpconnectedto,sender);
    if (paramcount<8) { putlog("!!! Failed to parse Servermessage: %s !!!",lastline); return; }
  }
  normnum(params[6+xofs]);
  strcpy(tmps2,params[6+xofs]);
  tmps2[2]='\0';
  if (firsts==1) { strcpy(uplnum,tmps2); }
  newserver(params[1+xofs],atoi(params[2+xofs]),tokentolong(tmps2),
    strtol(params[3+xofs],NULL,10),strtol(params[4+xofs],NULL,10),
    tokentolong(tmpconnectedto),tokentolong(&params[6+xofs][2]));
  if (params[5+xofs][0]=='P') {
    delsplit(params[1+xofs]);
  }
  if (burstcomplete) {
    resyncglinesat=getnettime()+60;
    if (sendglinesto[0]=='\0') {
#ifdef NUMERICGLINERESYNC
      strcpy(sendglinesto,tmps2);
#else
      strcpy(sendglinesto,params[1+xofs]);
#endif
    } else {
#ifdef NUMERICGLINERESYNC
      if (!alreadyinglinesto(params[1+xofs])) {
        if (strlen(sendglinesto)<20000) { strcat(sendglinesto,","); strcat(sendglinesto,tmps2); }
      }
#else
      if (!alreadyinglinesto(tmps2)) {
        if (strlen(sendglinesto)<20000) { strcat(sendglinesto,","); strcat(sendglinesto,params[1+xofs]); }
      }
#endif
    }
  }  
}

void handleping() {
  sendtouplink("%s Z :%s\r\n",servernumeric,(paramcount<3) ? "dummy" : params[2]);
  fflush(sockout);
  putlog("Ping? Pong!");
}

void handlekillmsg() {
  long num;
  char tmps2[TMPSSIZE];
  if (paramcount<3) {
    putlog("!!! Couldnt parse killmessage: %s !!!",lastline);
    return;
  }
  normnum(params[2]);
  sprintf(tmps2,"%sAAB",servernumeric);
  if (strcmp(params[2],tmps2)==0) {
    longtotoken(iptolong(127,0,0,1),tmps2,6);
    sendtouplink("%s N %s 1 %ld xevres %s +odk %s %sAAB :Xevres Rocks %s\r\n",servernumeric,mynick,starttime,myname,tmps2,servernumeric,operservversion);
    dointernalevents("K SELF","");
    dochanneljoins();
    return;
  }
  num=tokentolong(params[2]);
  deluserfromallchans(num);
  delautheduser(num);
  if (!killuser(num)) {
    fakeuserkill(num);
    putlog("!!! KILL failed: %s !!!",lastline);
  }
}

void handlesquit() {
  if (paramcount<3) {
    putlog("!!! Failed to parse SQUIT: %s !!!",lastline);
    return;
  }
  delsrv(params[2]);
}

void handlemodemsg() {
  userdata *a; int t1, t2;
  if (paramcount<4) {
    printf("!!! Failed to parse MODE: %s !!!\n",lastline);
    return;
  }
  if ((params[2][0]!='#') && (params[2][0]!='+') && (params[2][0]!='&')) {
    /* User changes Usermodes - since a user can only change it's own usermode
       and not even services can change usermodes, we can just search for the
       sender (parameter 0) instead of the nick (par 2). */
    long sendnum;
    sendnum=tokentolong(sender);
    a=getudptr(sendnum);
    if (a==NULL) {
      putlog("!!! Usermodechange failed (no such user): %s !!!",lastline);
      return;
    }
    t1=0; t2=0;
    while (params[3][t2]!='\0') {
      if ((params[3][t2]=='+') || (params[3][t2]=='-')) {
        if (params[3][t2]=='+') { t1=1; } else { t1=0; }
      } else {
        if (t1==1) {
          if (strlen(a->umode)<(MAXUMODES-1)) {
            if (!ischarinstr(params[3][t2],a->umode)) { appchar(a->umode,params[3][t2]); }
          }
        } else {
          delchar(a->umode,params[3][t2]);
        }
      }
      t2++;
    }
  } else {
    // Mode on a channel
    channel *tmpcp; char *c;
    toLowerCase(params[2]);
    tmpcp=getchanptr(params[2]); /* Convert the channelname to a pointer to the 
                                channel data to avoid overhead */
    if (tmpcp==NULL) {
      putlog("!!! Modechange for nonexistant channel %s !!!",params[2]);
      return;
    }
    t2=0; // assume "+"
    c=&params[3][0]; t1=4;
    for (;(*c!='\0');c++) {
      if (*c=='+') { t2=0; continue; }
      if (*c=='-') { t2=1; continue; }
      if (*c=='o') {
        if (paramcount<=t1) {
          putlog("!!! Failed to parse modes: %s !!!",lastline); return;
        } else {
          normnum(params[t1]);
          if (t2==1) {
            changechanmod2(tmpcp,tokentolong(params[t1]),2,um_o);
          } else {
            changechanmod2(tmpcp,tokentolong(params[t1]),1,um_o);
          }
          t1++;
        }
        continue; /* Just trying to make it continue with the loop faster */
      }
      if (*c=='v') {
        if (paramcount<=t1) {
          putlog("!!! Failed to parse modes: %s !!!",lastline); return;
        } else {
          normnum(params[t1]);
          if (t2==1) {
            changechanmod2(tmpcp,tokentolong(params[t1]),2,um_v);
          } else {
            changechanmod2(tmpcp,tokentolong(params[t1]),1,um_v);
          }
          t1++;
        }
        continue;
      }
      if (*c=='m') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_m);
        } else {
          delchanfla2(tmpcp,cm_m);
        }
        continue;
      }
      if (*c=='n') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_n);
        } else {
          delchanfla2(tmpcp,cm_n);
        }
        continue;
      }
      if (*c=='t') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_t);
        } else {
          delchanfla2(tmpcp,cm_t);
        }
        continue;
      }
      if (*c=='i') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_i);
        } else {
          delchanfla2(tmpcp,cm_i);
        }
        continue;
      }
      if (*c=='p') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_p);
        } else {
          delchanfla2(tmpcp,cm_p);
        }
        continue;
      }
      if (*c=='s') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_s);
        } else {
          delchanfla2(tmpcp,cm_s);
        }
        continue;
      }
      if (*c=='c') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_c);
        } else {
          delchanfla2(tmpcp,cm_c);
        }
        continue;
      }
      if (*c=='C') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_C);
        } else {
          delchanfla2(tmpcp,cm_C);
        }
        continue;
      }
      if (*c=='r') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_r);
        } else {
          delchanfla2(tmpcp,cm_r);
        }
        continue;
      }
      if (*c=='D') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_D);
        } else {
          delchanfla2(tmpcp,cm_D);
        }
        continue;
      }
      if (*c=='u') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_u);
        } else {
          delchanfla2(tmpcp,cm_u);
        }
        continue;
      }
      if (*c=='O') {
        if (t2!=1) {
          setchanfla2(tmpcp,cm_O);
        } else {
          delchanfla2(tmpcp,cm_O);
        }
        continue;
      }
      if (*c=='l') {
        if (t2!=1) {
          if (paramcount<=t1) {
            putlog("!!! Failed to parse modes: %s !!!",lastline); return;
          }
          setchanli2(tmpcp,atol(params[t1]));
          setchanfla2(tmpcp,cm_l);
          t1++;
        } else {
          delchanfla2(tmpcp,cm_l);
        }
        continue;
      }
      if (*c=='k') {
        if (paramcount<=t1) {
          putlog("!!! Failed to parse modes: %s !!!",lastline); return;
        }
        setchanke2(tmpcp,params[t1]);
        if (t2!=1) {
          setchanfla2(tmpcp,cm_k);
        } else {
          delchanfla2(tmpcp,cm_k);
        }
        t1++;
        continue;
      }
      if (*c=='F') {
        if (t2!=1) {
	  if (paramcount<=t1) {
           putlog("!!! Failed to parse modes: %s !!!",lastline); return;
          }
          setfwchan2(tmpcp,params[t1]);
          setchanfla2(tmpcp,cm_F);
	  t1++;
        } else {
          delchanfla2(tmpcp,cm_F);
        }
        continue;
      }
      if (*c=='b') {
        t1++;
        if (t2!=1) { tmpcp->canhavebans=1; }
        // We don't really care about channelbans...
      }
    }
  }
}

void handleprivmsg() {
  char tmps2[TMPSSIZE], tmps4[TMPSSIZE], ucmd[TMPSSIZE], lotmp[TMPSSIZE], fnick[NICKLEN+1];
  int res; long unum; char tail[TMPSSIZE]; int iio;
  if (paramcount!=4) {
    putlog("!!! Failed to parse privmsg: %s !!!",lastline);
    return;
  }
  unum=tokentolong(sender);
  strcpy(tail,params[3]);
  res=sscanf(tail,"%s",ucmd);
  if (res!=1) {
    putlog("!!! Failed to parse privmsg: %s !!!",lastline);
    return;
  }
  
  toLowerCase(ucmd);
  iio=isircop(unum);
  if (!iio) {
    tail[250]='\0';  // Truncate to max. 250 characters for non-ircops
  } else {
    tail[450]='\0';  // And 450 for opers to prevent too long lines
  }
  strcpy(tmps4,params[2]);
  if (tmps4[0] == '#') {
   /* chanmsg */
   long ix, iy, xnum;
   channel *c; chanuser *u;
   char xnick[NICKLEN+1];
   toLowerCase(tmps4);
   c=getchanptr(tmps4);
   if (c==NULL) { /* eh this chan must exist! */ return; }   
   u=(chanuser *)(c->users.content);
   /* find fakes on chan */
   for (ix=1;ix<myfakes.cursi;ix++) {
    xnum=((afakeuser *)(myfakes.content))[ix].numeric;
    mystrncpy(xnick,((afakeuser *)(myfakes.content))[ix].nick,NICKLEN);
    toLowerCase(xnick);
    for (iy=0;iy<c->users.cursi;iy++) {
     if (u[iy].numeric==xnum) {
      dofchanmsg(xnick,unum, tmps4, tail);
     }
    }
   } 
   return;
  }else if (ischarinstr('@',tmps4)) {
    toLowerCase(tmps4);
  } else {
    normnum(tmps4);
    sprintf(tmps2,"%sAAB",servernumeric);
    if (strcmp(tmps4,tmps2)!=0) {
      /* That message was not meant for X, but for one of the fakeusers */
      mystrncpy(fnick,unum2nick(tokentolong(tmps4)),NICKLEN);
      dodynfakecmds(ucmd,fnick,unum,tail,1,getauthlevel(unum));
      return;
    }
  }
  
  numtohostmask(unum,lotmp);
  if ((strcmp(ucmd,"password")!=0) && (strcmp(ucmd,"login")!=0) && (strcmp(ucmd,"auth")!=0) && (strcmp(ucmd,"chpasswd")!=0)) {
    putlog("%s !%s!",lotmp,tail);
  }

// Insert all commands that are available to non-ircops here - and a copy below
  if (strcmp(ucmd,"login")==0) { doauth(unum,tail); return; }
  if (strcmp(ucmd,"help")==0) { douserhelp(unum,tail); if (!iio) { return; } }
  if (strcmp(ucmd,"showcommands")==0) { douserhelp(unum,tail); if (!iio) { return; } }
  if (strcmp(ucmd,"create")==0) { dohello(unum,tail); return; }
  if (dodyncmds(ucmd,unum,tail,0,getauthlevel(unum))) { return; }
  if (iio) {
// Commands only available for ircops
    if (strcmp(ucmd,"password")==0) { dochangepwd(unum,tail); return; }
    if (strcmp(ucmd,"noticeme")==0) { donoticeset(unum,tail); return; }
    if (strcmp(ucmd,"whois")==0) { dowhois(unum,tail); return; }
    if (strcmp(ucmd,"status")==0) { dostatus(unum,tail); return; }
    if (strcmp(ucmd,"opchan")==0) { doopchan(unum,tail); return; }
    if (strcmp(ucmd,"help")==0) { dooperhelp(unum,tail); return; } 
    if (strcmp(ucmd,"showcommands")==0) { dooperhelp(unum,tail); return; }
    if (strcmp(ucmd,"channel")==0) { dochancmd(unum,tail); return; }
    if (strcmp(ucmd,"compare")==0) { dochancmp(unum,tail); return; }
    if (strcmp(ucmd,"resync")==0) { doresync(unum,tail); return; }
    if (strcmp(ucmd,"semiresync")==0) { dosemiresync(unum,tail); return; }
    if (strcmp(ucmd,"regexgline")==0) { doregexgline(unum,tail); return; }
    if (strcmp(ucmd,"mfc")==0) { domfc(unum,tail); return; }
    if (strcmp(ucmd,"settime")==0) { dosettimecmd(unum,tail); return; }
    if (strcmp(ucmd,"serverlist")==0) { doserverlist(unum,tail); return; }
    if (strcmp(ucmd,"changelev")==0) { dochangelev(unum,tail); return; }
    if (strcmp(ucmd,"deluser")==0) { dodelusercmd(unum,tail); return; }
    if (strcmp(ucmd,"xkick")==0) { dokickcmd(unum,tail); return; }
    if (strcmp(ucmd,"listauthed")==0) { dolistauthed(unum,tail); return; }
    if (strcmp(ucmd,"save")==0) { dosave(unum,tail); return; }
    if (strcmp(ucmd,"deopall")==0) { dodeopall(unum,tail); return; }
    if (strcmp(ucmd,"regexspew")==0) { doregexspew(unum,tail); return; }
    if (strcmp(ucmd,"fakeuser")==0) { docreatefakeuser(unum,tail); return; }
    if (strcmp(ucmd,"fakeadd")==0) { docreatefakeuser(unum,tail); return; }
    if (strcmp(ucmd,"rnc")==0) { dornc(unum,tail); return; }
    if (strcmp(ucmd,"rngline")==0) { dorngline(unum,tail); return; }
    if (strcmp(ucmd,"rnungline")==0) { dornungline(unum,tail); return; }
    if (strcmp(ucmd,"rnglist")==0) { dornglist(unum,tail); return; }
    if (strcmp(ucmd,"rnglogsearch")==0) { dornglogsearch(unum, tail); return; }
    if (strcmp(ucmd,"rnglogpurge")==0) { dornglogpurge(unum, tail); return; }
    if (strcmp(ucmd,"fakelist")==0) { dofakelist(unum,tail); return; }
    if (strcmp(ucmd,"fakekill")==0) { dofakekill(unum,tail); return; }
    if (strcmp(ucmd,"fakedel")==0) { dofakekill(unum,tail); return; }
    if (strcmp(ucmd,"inttobase64")==0) { dointtobase64(unum,tail); return; }
    if (strcmp(ucmd,"base64toint")==0) { dobase64toint(unum,tail); return; }
    if (strcmp(ucmd,"splitlist")==0) { dosplitlist(unum, tail); return; }
    if (strcmp(ucmd,"splitdel")==0) { dosplitdel(unum, tail); return; }
    if (strcmp(ucmd,"loadmod")==0) { doloadmod(unum, tail); return; }
    if (strcmp(ucmd,"unloadmod")==0) { dounloadmod(unum, tail); return; }
    if (strcmp(ucmd,"reloadmod")==0) { doreloadmod(unum, tail); return; }
    if (strcmp(ucmd,"lsmod")==0) { dolsmod(unum, tail); return; }
    if (strcmp(ucmd,"die")==0) { dodie(unum, tail); return; }
    if (strcmp(ucmd,"noticemask")==0) { donoticemask(unum, tail); return; }
    if (strcmp(ucmd,"chanfix")==0) { dochanfix(unum, tail); return; }
    if (strcmp(ucmd,"showregs")==0) { doshowregs(unum, tail); return; }
    if (strcmp(ucmd,"chanopstat")==0) { dochanopstat(unum, tail); return; }
    if (strcmp(ucmd,"chanoplist")==0) { dochanoplist(unum, tail); return; }
    if (strcmp(ucmd,"rehash")==0) { dorehash(unum, tail); return; }
    if (dodyncmds(ucmd,unum,tail,1,getauthlevel(unum))) { return; }
    if (ucmd[0]==1) { doctcp(unum, tail); return; }
    msgtouser(unum,"Unknown command.");
  } else {
    msgtouser(unum,"Unknown command..");
  }
}

void handleburstmsg() {
  channel *ctmp; long creationtime; int actflags; char *c; long bibabutzelmann;
  int xofs; char tmps2[TMPSSIZE], tmps3[TMPSSIZE]; int i; int k;
  if (paramcount<5) {
    putlog("!!! Failed to parse Burst-Message: %s !!!",lastline);
    return;
  }
  creationtime=strtol(params[3],NULL,10);
  toLowerCase(params[2]);
  ctmp=getchanptr(params[2]);
  if (ctmp==NULL) {
    if (strlen(params[2])>CHANNELLEN) {
      putlog("!!! Channelname exceeds CHANNELLEN: %s !!!",lastline);
      return;
    }
    ctmp=newchan(params[2],creationtime);
  } else if (ctmp->createdat > creationtime) {
    /* BURST message with older timestamp - strip all current channel modes */
    chanuser *cu;
    cu=(chanuser *)(ctmp->users.content);
    for(i=0;i<ctmp->users.cursi;i++) {
      cu[i].flags=0;
    }
    ctmp->flags=0;
    ctmp->createdat=creationtime;
  }
  if (params[4][0]=='+') {
    // that channel has some modes (like +t etc.)
    int j;
    xofs=5;
    c=&params[4][1];
    for (;*c!='\0';c++) {
      if (*c=='k') {
        // Channel has a key
        if (paramcount<=xofs) {
          putlog("!!! Failed to parse BURST (missing arg to chanflag): %s !!!",lastline);
          return;
        }
        setchanke2(ctmp,params[xofs]);
        xofs++;
      }
      if (*c=='l') {
        // Channel has a limit
        if (paramcount<=xofs) {
          putlog("!!! Failed to parse BURST (missing arg to chanflag): %s !!!",lastline);
          return;
        }
        setchanli2(ctmp,strtol(params[xofs],NULL,10));
        xofs++;
      }
      for (j=0;j<numcf;j++) {
        if (chanflags[j].c==*c) { setchanfla2(ctmp,chanflags[j].v); }
      }
    }
  } else {
    xofs=4;
  }
  // modes have been handled if they existed. params[xofs] is the userlist.
  if (paramcount<=xofs) { 
    putlog("IRCU-Bug (channel with no users): %s",lastline);
    if (ctmp->users.cursi==0) { delchan(params[2]); }
    return; /* Means we got a channel without any users - ircu-bug */
  } 
  actflags=0;
  if (strcmp(lastburstchan,params[2])==0) { actflags=lastburstflags; }
  c=&params[xofs][0];
  if (*c!=':') { /* if it begins with :% it's just a banlist! */
    for (;(*c!='\0');c++) {
      i=0;
      for (;((*c!='\0') && (*c!=','));c++) {
        tmps2[i]=*c; i++;
      }
      tmps2[i]='\0';
      if (strlen(tmps2)<3) { break; }
      if (strlen(tmps2)==3) { // V.07 numeric, no modes
        normnum(tmps2); tmps3[0]='\0';
      } else {
        if (tmps2[3]==':') { // V.07 Numeric with modes
          strcpy(tmps3,&tmps2[4]);
          tmps2[3]='\0';
          normnum(tmps2);
        } else {
          if (tmps2[5]==':') {
            strcpy(tmps3,&tmps2[6]);
            tmps2[5]='\0';
          } else {
            tmps3[0]='\0';
          }
        }
      }
      /* tmps2 now contains the normed numeric, tmps3 the new modes if any. */
      if (tmps3[0]!='\0') { actflags=0; }
      for (i=0;i<strlen(tmps3);i++) {
        for (k=0;k<numuf;k++) {
          if (userflags[k].c==tmps3[i]) { actflags=(actflags | userflags[k].v); }
        }
      }
      bibabutzelmann=tokentolong(tmps2);
      if (getudptr(bibabutzelmann)!=NULL) {  // Is this numeric VALID at all?!
        addusertocptr(ctmp,bibabutzelmann);
        changechanmod2(ctmp,bibabutzelmann,0,actflags);
        addchantouser2(bibabutzelmann,ctmp);
      }
      if (*c=='\0') { break; }
    }
  }
  if (paramcount>(xofs+1)) { // There are bans in the burst
    ctmp->canhavebans=1;
  }
  strcpy(lastburstchan,params[2]);
  lastburstflags=actflags;
  if (ctmp->users.cursi==0) { delchan(params[2]); }  // No valid users on the
                                // channel? then kill it...
}

void handlecreatemsg() {
/* [15:28] DAO CREATE #fook,#foo2 971097981 */
  long creattime; char *c; int i; char tmps2[TMPSSIZE]; long num;
  channel *ctmp;
  if (paramcount<4) {
    putlog("!!! Error parsing create message: %s !!!",lastline);
    return;
  }
  toLowerCase(params[2]);
  creattime=strtol(params[3],NULL,10);
  num=tokentolong(sender);
  if (getudptr(num)==NULL) { return; /* Invalid client numeric! */ }
  c=&params[2][0];
  for (;(*c!='\0');c++) {
    for (i=0;((*c!='\0') && (*c!=','));c++) {
      tmps2[i]=*c; i++;
    }
    tmps2[i]='\0'; tmps2[CHANNELLEN]='\0';
    if (chanexists(tmps2)!=1) { newchan(tmps2,creattime); }
    addusertochan(tmps2,num);
    changechanmode(tmps2,num,0,um_o);
    ctmp=getchanptr(tmps2);
    if (ctmp!=NULL) { addchantouser2(num,ctmp); }
    if (*c=='\0') { break; }
  }
}

void handlejoinmsg() {
  long num; char *c; int i; char tmps2[TMPSSIZE]; channel *ctmp;
  if (paramcount<3) {
    putlog("!!! Error parsing join message: %s !!!",lastline);
    return;
  }
  num=tokentolong(sender);
  if (getudptr(num)==NULL) { return; /* Invalid client numeric! */ }
  toLowerCase(params[2]);
  c=&params[2][0];
  for (;(*c!='\0');c++) {
    for (i=0;((*c!='\0') && (*c!=','));c++) {
      tmps2[i]=*c; i++;
    }
    tmps2[i]='\0'; tmps2[CHANNELLEN]='\0';
    if (strcmp(tmps2,"0")==0) {
      deluserfromallchans(num);
    } else {
      if (!chanexists(tmps2)) {
//        putlog("!!! JOIN for nonexistant channel: %s !!!",lastline);
        newchan(tmps2,0);
      }
      addusertochan(tmps2,num);
      ctmp=getchanptr(tmps2);
      if (ctmp!=NULL) { addchantouser2(num,ctmp); }
    }
    if (*c=='\0') { break; }
  }
}

void handlepartmsg() {
  long num; char *parspos;
  if (paramcount<3) {
    putlog("!!! PART failed: too few parameters: %s !!!",lastline);
    return;
  }
  num=tokentolong(sender);
  toLowerCase(params[2]);
  parspos=strtok(params[2],",");
  while (parspos!=NULL) {
    delchanfromuser(num,parspos);
    deluserfromchan(parspos,num);
    parspos=strtok(NULL,",");
  }
}

void handlekickmsg() {
/* [01:11] :Fox_Muld_ KICK #twilightzone DAL :That's gotta hurt! */
  long num; char *c; int i; char tmps2[TMPSSIZE];
  if (paramcount<4) {
    putlog("!!! Error parsing kick message: %s !!!",lastline);
    return;
  }
  toLowerCase(params[2]);
  c=&params[3][0];
  for (;(*c!='\0');c++) {
    i=0;
    for (;((*c!='\0') && (*c!=','));c++) {
      tmps2[i]=*c; i++;
    }
    tmps2[i]='\0';
    normnum(tmps2);
    num=tokentolong(tmps2);
    delchanfromuser(num,params[2]);
    deluserfromchan(params[2],num);
    if (*c=='\0') { break; }
  }
}

void handleglinemsg() {
  char tmps2[TMPSSIZE*2], tmps3[TMPSSIZE]; serverdata *sd;
/*  XX GL * +host 222 :blubb */
  if (paramcount<4) {
    putlog("!!! Error parsing gline message: %s !!!",lastline);
    return;
  }
  if (params[3][0]=='+') {
    if (paramcount!=6) {
      putlog("!!! Failed to add gline from foreign server (Parameters missing): %s !!!",lastline);
      return;
    }
    sd=getsdptr(tokentolong(sender));
    if (sd==NULL) {
      strcpy(tmps2,"Network");
    } else {
      strcpy(tmps2,sd->nick);
      tmps2[65]='\0';
    }
    if (isglineset(&params[3][1])!=1) {
      addgline(&params[3][1],params[5],tmps2,atol(params[4]),0);
    } else {
      mysql_escape_string(tmps3,&params[3][1],strlen(&params[3][1]));
      sprintf(tmps2,"UPDATE glines SET expires='%ld' WHERE hostmask='%s'",
        strtol(params[4],NULL,10)+getnettime(),tmps3);
      mysql_query(&sqlcon,tmps2);
    }
  } else {
    remgline(&params[3][1],0);
  }
}

void handlestatsPreply() {
/* [00:00] :leaf.de.quakenet.eu.org 217 Operserv2 P 6668 0 :0x2000
   [00:00] :leaf.de.quakenet.eu.org 217 Operserv2 P 6667 2 :0x2000 */
  int res; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  long servn; serverdata *s;
  res=sscanf(lastline,"%s %s %s %s %s",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res!=5) {
    putlog("!!! Failed to parse /stats P - reply: %s !!!", lastline);
    return;
  }
  if (tmps2[0]==':') {
    servn=getsrvnum(&tmps2[1]);
  } else {
    servn=tokentolong(tmps2);
  }
  s=sls;
  while (s!=NULL) {
    if (s->numeric==servn) {
      // this is the server we got the stats-reply for
      if (strlen(s->ports)>1000) { return; }
      if (strlen(s->ports)==0) {
        strcpy(s->ports, tmps6);
      } else {
        strcat(s->ports, ",");
        strcat(s->ports, tmps6);
      }
      return;
    }
    s=(void *)s->next;
  }
  putlog("!!! Got /stats P - reply from unknown server! %s !!!",lastline);
}

void handlestatsureply() {
/* [00:32] :leaf.de.quakenet.eu.org 242 Operserv2 :Server Up 65 days, 23:48:14 */
  int res; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE], tmps7[TMPSSIZE];
  char parspos; long secsup; long servn; serverdata *s;
  res = sscanf(lastline,"%s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5);
  if (res!=4) {
    putlog("!!! Failed to parse /stats u - reply(#1): %s !!!",lastline);
    return;
  }
  res=sscanf(tmps5,"%s %s %s %s %[^\n]",tmps6,tmps6,tmps3,tmps6,tmps4);
  if (res!=5) {
    putlog("!!! Failed to parse /stats u - reply(#2): %s !!!",lastline);
    return;
  }
  res=sscanf(tmps4,"%[1234567890]%c%[1234567890]%c%[1234567890]",tmps5,&parspos,tmps6,&parspos,tmps7);
  if (res!=5) {
    putlog("!!! Failed to parse /stats u - reply(#3): %s !!!",lastline);
    return;
  }
  secsup=strtol(tmps7,NULL,10)+strtol(tmps6,NULL,10)*60+strtol(tmps5,NULL,10)*3600+strtol(tmps3,NULL,10)*86400;
  if (tmps2[0]==':') {
    servn=getsrvnum(&tmps2[1]);
  } else {
    servn=tokentolong(tmps2);
  }
  s=sls;
  while (s!=NULL) {
    if (s->numeric==servn) {
      s->createdat=getnettime()-secsup;
      s->pingreply=getnettime();
      if (gotallpingreplys()) {
        if (getmaxping()>5) {
          putlog("Network has more than 5 seconds (%ld) lag, cannot do settime!",getmaxping());
          lastsettime+=5*60; waitingforping=0;
        } else {
          putlog("Network lag: %ld seconds. Sending settime.",getmaxping());
          timestdif=0;  /* _WE_ are authoritative with respect to time. */
          lastsettime=getnettime(); waitingforping=0;
          sendtouplink("%s SE %lu\n",servernumeric,(long int)getnettime());
          fflush(sockout);
        }
      }
      return;
    }
    s=(void *)s->next;
  }
  putlog("!!! Got /stats u - reply from unknown server: %s !!!",lastline);
}

void handlestatsmsg() {
  int res; static char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE]; long num;
  res=sscanf(lastline,"%s %s %s",tmps2,tmps3,tmps4);
  if (res<3) { return; }
  if (tmps2[0]==':') {
    toLowerCase(tmps2);
    num=nicktonum(&tmps2[1]);
  } else {
    normnum(tmps2);
    num=tokentolong(tmps2);
  }
  longtotoken(num,tmps3,5);
  if (tmps3[0]=='\0') { return; }
  if (strcmp(tmps4,"P")==0) {
    /* /stats P requested (open ports) */
    sendtouplink("%s 217 %s P none 0 0x2000\r\n",servernumeric,tmps3);
    sendtouplink("%s 219 %s P :End of /STATS report\r\n",servernumeric,tmps3);
    fflush(sockout);
  }
  if (strcmp(tmps4,"u")==0) {
    /* /stats u requested (uptime) */
    long iamup, days, hours, minutes;
    iamup=getnettime()-starttime;
    days=iamup/(24*3600); iamup%=(24*3600);
    hours=iamup/3600; iamup%=3600;
    minutes=iamup/60; iamup%=60;
    sprintf(tmps2,"%ld days, %02ld:%02ld:%02ld",days,hours,minutes,iamup);
    sendtouplink("%s 242 %s :Server Up %s\r\n",servernumeric,tmps3, tmps2);
    sendtouplink("%s 250 %s :Highest connection count: 2 (1 client)\r\n",servernumeric,tmps3);
    sendtouplink("%s 219 %s u :End of /STATS report\r\n",servernumeric,tmps3);
    fflush(sockout);
  }
}

void handleeoback() {
  serverdata *a; char tmpn[SRVLEN+1]; long num;
  num=tokentolong(sender);
  a=sls;
  while (a!=NULL) {
    if (num==a->numeric) {
      strncpy(tmpn,a->nick,SRVLEN);
      tmpn[SRVLEN]='\0';
      toLowerCase(tmpn);
      delsplit(tmpn);
      return;
    }
    a=(void *)a->next;
  }
  putlog("uups, EOB_ACK from unknown server?!");
}

void handletopic() {
  channel *c;
  if (paramcount<3) { return; /* This line makes no sense to us */ }
  c=getchanptr(params[2]);
  if (c==NULL) { return; /* We don't know that channel, we can't change topic */ }
  if ((paramcount<4) || (strlen(params[paramcount-1])==0)) {
    freeastring(c->topic);
    c->topic=getastring("");
    return;
  }
  if (strlen(params[paramcount-1])>TOPICLEN) {
    putlog("!!! TOPIC exceeds Topiclen! Lastline: %s !!!",lastline);
    return;
  }
  freeastring(c->topic);
  c->topic=getastring(params[paramcount-1]);
}

void handleremotewhois() {
  char tmpnick[NICKLEN+1]; int i;
  afakeuser *fu;
  if (paramcount<4) {
    putlog("!!! Remote WHOIS with insufficient parameters received! !!!");
    return;
  }
  if (strlen(params[3])<1) {
    putlog("!!! Remote WHOIS with illegal parameters received! !!!");
    return;
  }
  toLowerCase(params[3]);
  fu=(afakeuser *)(myfakes.content);
  for (i=0;i<myfakes.cursi;i++) {
    strcpy(tmpnick,fu[i].nick); toLowerCase(tmpnick);
    if (strcmp(&params[3][0],tmpnick)==0) {
      /* Send whois replies */
      sendtouplink("%s 311 %s %s %s %s * :%s\r\n",servernumeric,sender,fu[i].nick,fu[i].ident,fu[i].host,fu[i].realname);
      sendtouplink("%s 319 %s %s :irc.xchannel.org\r\n",servernumeric,sender,fu[i].nick);
      sendtouplink("%s 312 %s %s xevres.xchannel.org :Xevres %s\r\n",servernumeric,sender,fu[i].nick,operservversion);
      sendtouplink("%s 317 %s %s 0 %ld :seconds idle, signon time\r\n",servernumeric,sender,fu[i].nick,fu[i].connectat);
      if ((fu[i].numeric % SRVNUMMULT)<10) {
        sendtouplink("%s 313 %s %s :is an IRC Operator\r\n",servernumeric,sender,fu[i].nick);
      }
      sendtouplink("%s 318 %s %s :End of /WHOIS list.\r\n",servernumeric,sender,fu[i].nick);
      fflush(sockout);
      return;
    }
  }
  putlog("!!! Remote WHOIS for someone not my user received! >%s< !!!",lastline);
  return;
}

void handleeobmsg() {
  if (strcmp(sender,uplnum)==0) {
    putlog("Sending EOB_ACK");
    sendtouplink("%s EA\r\n",servernumeric); fflush(sockout);
    burstcomplete=1;
    putlog("Merge took %ld seconds.",(time(NULL)-starttime));
    putlog("Joining channels and creating fakeusers");
    dochanneljoins();
    fakeuserload();
    putlog("Done.");
    pingallservers(); waitingforping=getnettime(); lastsettime=getnettime()-3600;
    goaway();
  }
}

void handleclearmodemsg() {
	/* FIXME we need to handle clearmode properly! */
  chanuser *b; int j; channel *c;
  if (paramcount<4) { return; }
  toLowerCase(params[2]);
  c=getchanptr(params[2]);
  if (c==NULL) { return; }
  if (ischarinstr('o',params[3])) {
    b=(chanuser *)c->users.content;
    for (j=0;j<c->users.cursi;j++) {
      if (isflagset(b[j].flags,um_o)) {
        changechanmod2(c,b[j].numeric,2,um_o);
      }
    }
  }
  if (ischarinstr('v',params[3])) {
    b=(chanuser *)c->users.content;
    for (j=0;j<c->users.cursi;j++) {
      if (isflagset(b[j].flags,um_v)) {
        changechanmod2(c,b[j].numeric,2,um_v);
      }
    }
  }
  if (ischarinstr('b',params[3])) {
  	c->canhavebans=0;
  }
  for (j=0;j<numcf;j++) {
  	if (ischarinstr(chanflags[j].c,params[3])) {
      delchanfla2(c,chanflags[j].v);
    }
  }
}
