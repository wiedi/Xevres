/*
xevres channel module
coded by Wiedi

TODO: 
	* request cmd
	* testing !!!
	* bugfixing
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef WE_R_ON_BSD
#include <malloc.h>
#endif
#ifdef USE_DOUGLEA_MALLOC
#include "dlmalloc/malloc.h"
#endif
#include "globals.h"
#include "chan.h"
#define MODNAM "chan"

int donej=0;
int uplinkup;

flags tuflags[] = {
 {' ',	9},
 {'g',	UF_GIVE},
 {'v',	UF_VOICE},
 {'a',	UF_AUTO},
 {'o',	UF_OP},
 {'i',	UF_INVITE},
 {'l',	UF_LOG},
 {'c',	UF_COOWNER},
 {'n',	UF_OWNER}
};

flags tcflags[] = {
 {' ', 12},
 {'v',	CF_AUTOVOICE},
 {'p',	CF_PROTECT},
 {'f',	CF_FORCE},
 {'s',	CF_SECURE},
 {'l',	CF_LIMIT},
 {'w',	CF_WELCOME},
 {'i',	CF_INVITE},
 {'h',	CF_HIDDEN},
 {'k',	CF_KEY},
 {'S',	CF_SUSPENDED},
 {'F',	CF_FORWARD}
};

void ch_op(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res, i, found; long un;
 userdata *ux; channel **cx;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res != 3 && res != 2) {
  msgtouser(unum,"op usage:");
  newmsgtouser(unum,"/msg %s op <#channel> [nick]",mynick);
  return;
 }
 
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum)))) {
   newmsgtouser(unum,"You are not known on %s",tmps3);
   return;
 }  
 if (!UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission for this");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }  
 if (CIsSecuremode(cp) && !UHasOwner(up)) {
   msgtouser(unum,"Channel is in securedmode!");
   return;
 }  
 if (res == 2) {
   longtotoken(unum,tmps5,5);
 } else {
   un=nicktonum(tmps4);
   if (un<0) {
     newmsgtouser(unum,"User %s is not on the network",tmps4);
     return;
   }
   ux=getudptr(un);
   found=0;
   cx=(channel **)(ux->chans.content);
   for (i=0;i<ux->chans.cursi;i++) {
     if (strcmp(tmps3,cx[i]->name)==0) {
       found=1;
       break;
     }
   }
   if (found==0) {
     newmsgtouser(unum,"User %s is not on %s",tmps4,tmps3);
     return;
   }    
   longtotoken(un,tmps5,5); 
 }
 sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,tmps3,tmps5); 
 sim_mode(tmps3,"+o",tokentolong(tmps5));
 fflush(sockout);
 longtotoken(unum,tmps2,5);
 ch_msgtolog(cp,"%s(%s) gave op to %s",unum2nick(unum),unum2auth(unum),(strcmp(tmps2,tmps5)==0) ? "him self" : tmps4);
 msgtouser(unum,"Done"); 
}

void ch_voice(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res, i, found; long un;
 userdata *ux; channel **cx;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res != 3 && res != 2) {
  msgtouser(unum,"voice usage:");
  newmsgtouser(unum,"/msg %s voice <#channel> [nick]",mynick);
  return;
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum)))) {
   newmsgtouser(unum,"You are not known on %s",tmps3);
   return;
 }  
 if (!UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !UHasVoice(up) && !checkauthlevel(unum,200)) {
   msgtouser(unum,"You don't have the permission for this");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 if (res == 2) {
   longtotoken(unum,tmps5,5);
 } else {
   un=nicktonum(tmps4);
   if (un<0) {
     newmsgtouser(unum,"User %s is not on the network",tmps4);
     return;
   }
   ux=getudptr(un);
   found=0;
   cx=(channel **)(ux->chans.content);
   for (i=0;i<ux->chans.cursi;i++) {
     if (strcmp(tmps3,cx[i]->name)==0) {
       found=1;
       break;
     }
   }
   if (found==0) {
     newmsgtouser(unum,"User %s is not on %s",tmps4,tmps3);
     return;
   }    
   longtotoken(un,tmps5,5); 
 }
 sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,tmps3,tmps5); 
 sim_mode(tmps3,"+v",tokentolong(tmps5));
 fflush(sockout);
 longtotoken(unum,tmps2,5);
 ch_msgtolog(cp,"%s(%s) gave voice to %s",unum2nick(unum),unum2auth(unum),(strcmp(tmps2,tmps5)==0) ? "him self" : tmps4);
 msgtouser(unum,"Done"); 
}

void ch_invite(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res; long ux=-1;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res != 3 && res != 2) {
  msgtouser(unum,"invite usage:");
  newmsgtouser(unum,"/msg %s invite <#channel> [nick]",mynick);
  return;
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasInvite(up) && !UHasVoice(up) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,200)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 if (res == 2) {
   numtonick(unum,tmps4);
 } else {
   if (!UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,300)) {
    msgtouser(unum,"You don't have the permission to invite other users");
    return;
   } 
   ux=nicktonum(tmps4);
   if (ux<0) {
    newmsgtouser(unum,"User %s is not on the network",tmps4);
    return;
   }
  }
  sendtouplink("%sAAB I %s :%s\r\n",servernumeric,tmps4,tmps3); 
  fflush(sockout);
  ch_msgtolog(cp,"%s(%s) invited %s",unum2nick(unum),unum2auth(unum),(unum==ux || res==2) ? "him self" : tmps4);
  msgtouser(unum,"Done"); 
}

void ch_topic(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %[^\n]",tmps2,tmps3,tmps4);
 if (res != 3) {
  msgtouser(unum,"topic usage:");
  newmsgtouser(unum,"/msg %s topic <#channel> <topic>",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,300)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 sendtouplink("%sAAB T %s :%s\r\n",servernumeric,tmps3,tmps4); 
 sim_topic(tmps3,tmps4);
 fflush(sockout);
 ch_msgtolog(cp,"%s(%s) changed topic",unum2nick(unum),unum2auth(unum));
 msgtouser(unum,"Done");
}

void ch_kick(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], uhost[TMPSSIZE];
 int res, i; unsigned long j; 
 channel *a; chanuser *b; userdata *c;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5);
 if (res < 3) {
   msgtouser(unum,"kick usage:");
   newmsgtouser(unum,"Operator:  /msg %s kick <#channel> <nick> [reason]",mynick);
   newmsgtouser(unum,"[Co]Owner: /msg %s kick <#channel> <nick>[!ident@host] [reason] (Note: you can use wildcards)",mynick);
   return;
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 if (res == 3) { 
   numtonick(unum,tmps2);
   sprintf(tmps5,"requested by %s",tmps2); 
 }
 if (UHasCoowner(up) || UHasOwner(up) || checkauthlevel(unum,500)) {
   /* rock n roll */
   a=getchanptr(tmps3);
   if (a==NULL) { return; /* should never happen */ }
   b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; i=0;
   for (j=0;j<a->users.cursi;j++) {
     c=getudptr(b[j].numeric);
     if (ischarinstr('!',tmps4)) {
       /* nick!ident@host */
       sprintf(uhost,"%s!%s@%s",c->nick,c->ident,c->host);
       if (match2strings(tmps4,uhost)) {
         if (!ischarinstr('k',c->umode) && !ischarinstr('X',c->umode)) {
           longtotoken(c->numeric,tmps2,5);
           sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
           sim_part(tmps3,c->numeric);
           i++;
         } 
       } 
     } else { 
       /* nick */
       numtonick(b[j].numeric,tmps2);
       if (match2strings(tmps4,tmps2)) {
         if (!ischarinstr('k',c->umode) && !ischarinstr('X',c->umode)) {
           longtotoken(c->numeric,tmps2,5);
           sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
           sim_part(tmps3,c->numeric);
           i++;
         } 
       }
     } 
     res++;
   }
   newmsgtouser(unum,"Kicked %i users from %s",i,tmps3); 
 } else {
   /* normal kick */
   a=getchanptr(tmps3);
   if (a==NULL) { return; /* should never happen */ }
   b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; i=0;
   for (j=0;j<a->users.cursi;j++) {
     c=getudptr(b[j].numeric);
     numtonick(b[j].numeric,tmps2);
     if (match2strings(tmps4,tmps2)) {
       if (!ischarinstr('k',c->umode) && !ischarinstr('X',c->umode)) {
         longtotoken(c->numeric,tmps2,5);
         sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
         sim_part(tmps3,c->numeric);
         msgtouser(unum,"Done");
         break;
       } 
     } 
   } 
   msgtouser(unum,"User not on channel");
 } 
 ch_msgtolog(cp,"%s(%s) kicked %s",unum2nick(unum),unum2auth(unum),tmps4);
}

void ch_chanmode(long unum, char *tail) {
 char *c, tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], p[5][TMPSSIZE], token[5];
 int res, t1=0, t2=0, pcount; long num;
 cuser *up, *up2; rchan *cp;
 channel *ncp;
 
 res=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5);
 if (res!=3 && res!=4) {
  msgtouser(unum,"chanmode usage:");
  newmsgtouser(unum,"/msg %s chanmode <#channel> <+|-><modes> [params]",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 pcount=sscanf(tmps5,"%s %s %s %s %s %s",p[0],p[1],p[2],p[3],p[4],p[5]);
 c=&tmps4[0];
 ncp=cp->cptr;
 for (;(*c!='\0');c++) {
   if (*c=='+') { t2=0; continue; }
   if (*c=='-') { t2=1; continue; }
   if (*c=='o') {
     if (pcount<=t1) {
       msgtouser(unum,"Need more parameters (o)");
       return;
     } else {
       num=nicktonum(p[t1]);
       if (num==0) {
         newmsgtouser(unum,"User %s is not on the network",p[t1]);
         return;
       } 
       longtotoken(num,token,5);
       up2=ch_getchanuserbynick(cp,p[t1]);
       if (t2==1) {
         /* deop */
         if ((CIsProtect(cp) || CIsForce(cp)) && UHasOp(up)) {
	   newmsgtouser(unum,"User %s is protected!",p[t1]);
	   return;
	 }  
         sendtouplink("%sAAB M %s -o %s\r\n",servernumeric,tmps3,token);
	 fflush(sockout);
         changechanmod2(ncp,tokentolong(p[t1]),2,um_o);
       } else {
         /* op */
         if (CIsSecuremode(cp) && !UHasOwner(up)) {
	   msgtouser(unum,"Channel is in securedmode!");
	   return;
	 }
	 if (CIsForce(cp) && !(UHasOp(up2) || UHasOwner(up2))) {
	   msgtouser(unum,"Channel is in forcemode!");
	   return;
	 }
	 sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,tmps3,token);
	 fflush(sockout);
         changechanmod2(ncp,nicktonum(p[t1]),1,um_o);
       }
       t1++;
     }
     continue;
   }
   if (*c=='v') {
     if (pcount<=t1) {
       msgtouser(unum,"Need more parameters (v)");
       return;
     } else {
       num=nicktonum(p[t1]);
       if (num==0) {
         newmsgtouser(unum,"User %s is not on the network",p[t1]);
         return;
       } 
       longtotoken(num,token,5);
       up2=ch_getchanuserbynick(cp,p[t1]);
       if (t2==1) {
         /* devoice */
	 if (CIsForce(cp) && UHasVoice(up)) {
	   msgtouser(unum,"Channel is in forcemode!");
	   return;
	 }	 
         sendtouplink("%sAAB M %s -v %s\r\n",servernumeric,tmps3,token);
	 fflush(sockout);
	 changechanmod2(ncp,tokentolong(params[t1]),2,um_v);
       } else {
	 /* voice */ 
	 if (CIsForce(cp) && !(UHasVoice(up) || UHasOwner(up) || CIsAutovoice(cp))) {
	   msgtouser(unum,"Channel is in forcemode!");
	   return;
	 }
	 sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,tmps3,token);
	 fflush(sockout);
         changechanmod2(ncp,tokentolong(params[t1]),1,um_v);
       }
          t1++;
        }
        continue;
      }
      if (*c=='m') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +m\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_m);
        } else {
          sendtouplink("%sAAB M %s -m\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_m);
        }
        continue;
      }
      if (*c=='n') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +n\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_n);
        } else {
          sendtouplink("%sAAB M %s -n\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_n);
        }
        continue;
      }
      if (*c=='t') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +t\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_t);
        } else {
          sendtouplink("%sAAB M %s -t\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_t);
        }
        continue;
      }
      if (*c=='i') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +i\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_i);
        } else {
          if (CIsInvite(cp)) {
            msgtouser(unum,"Channel has invite flag!");
            return;
          }    
          sendtouplink("%sAAB M %s -i\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_i);
        }
        continue;
      }
      if (*c=='p') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +p\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_p);
        } else {
          sendtouplink("%sAAB M %s -p\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_p);
        }
        continue;
      }
      if (*c=='s') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +s\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_s);
        } else {
          if (CIsHidden(cp)) {
            msgtouser(unum,"Channel has hidden flag!");
            return;
          }  
          sendtouplink("%sAAB M %s -s\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_s);
        }
        continue;
      }
      if (*c=='c') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +c\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_c);
        } else {
          sendtouplink("%sAAB M %s -c\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_c);
        }
        continue;
      }
      if (*c=='C') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +C\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_C);
        } else {
          sendtouplink("%sAAB M %s -C\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_C);
        }
        continue;
      }
      if (*c=='r') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +r\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_r);
        } else {
          sendtouplink("%sAAB M %s -r\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_r);
        }
        continue;
      }
      if (*c=='D') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +D\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_D);
        } else {
          sendtouplink("%sAAB M %s -D\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_D);
        }
        continue;
      }
      if (*c=='u') {
        if (t2!=1) {
          sendtouplink("%sAAB M %s +u\r\n",servernumeric,tmps3);
          fflush(sockout);
          setchanfla2(ncp,cm_u);
        } else {
          sendtouplink("%sAAB M %s -u\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_u);
        }
        continue;
      }
      
      if (*c=='l') {
        if (t2!=1) {
          if (pcount<=t1) {
            msgtouser(unum,"Need more parameters (l)");
            return;
          }
          sendtouplink("%sAAB M %s +l %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout);
          setchanli2(ncp,atol(p[t1]));
          setchanfla2(ncp,cm_l);
          t1++;
        } else {
          if (CIsLimit(cp)) {
            msgtouser(unum,"Channel has limit flag!");
            return;
          } 
          sendtouplink("%sAAB M %s -l\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_l);
        }
        continue;
      }
      if (*c=='k') {
        if (pcount<=t1) {
          msgtouser(unum,"Need more parameters (k)");
          return;
        }
        setchanke2(ncp,p[t1]);
        if (t2!=1) {
          sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout);
          setchanfla2(ncp,cm_k);
        } else {
          if (CIsKey(cp)) {
            msgtouser(unum,"Channel has key flag!");
            return;
          } 
          sendtouplink("%sAAB M %s -k %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout);
          delchanfla2(ncp,cm_k);
        }
        t1++;
        continue;
      }
      if (*c=='F' && isircop(unum)) {
        if (t2!=1) {
          if (pcount<=t1) {
            msgtouser(unum,"Need more parameters (F)");
            return;
          }
          sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout);
          setfwchan2(ncp,params[t1]);
          setchanfla2(ncp,cm_F);
	  t1++;
        } else {
          if (CIsForward(cp)) {
            msgtouser(unum,"Channel has forward flag!");
            return;
          } 
          sendtouplink("%sAAB M %s -F\r\n",servernumeric,tmps3);
          fflush(sockout);
          delchanfla2(ncp,cm_F);
        }
        continue;
      }
      if (*c=='b') {
        if (pcount<=t1) {
          msgtouser(unum,"Need more parameters");
          return;
        }
        if (t2!=1) { 
          ncp->canhavebans=1; 
          sendtouplink("%sAAB M %s +b %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout); 
        } else {
          sendtouplink("%sAAB M %s -b %s\r\n",servernumeric,tmps3,p[t1]);
          fflush(sockout);  
        } 
        t1++;
      }
    }
 msgtouser(unum,"Done");    
 return;  
}

void ch_unbanall(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
  msgtouser(unum,"unbanall usage:");
  newmsgtouser(unum,"/msg %s unbanall <#channel>",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 sendtouplink("%s CM %s b\r\n",servernumeric,tmps3);
 fflush(sockout);
 ch_msgtolog(cp,"%s(%s) used unbanall",unum2nick(unum),unum2auth(unum));
 msgtouser(unum,"Done");
 return;  
}

void ch_deopall(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
  msgtouser(unum,"deopall usage:");
  newmsgtouser(unum,"/msg %s deopall <#channel>",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 deopall(cp->cptr);
 sendtouplink("%s M %s +o %sAAB\r\n",servernumeric,tmps3,servernumeric);
 fflush(sockout);
 sim_mode(tmps3,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
 ch_msgtolog(cp,"%s(%s) used deopall",unum2nick(unum),unum2auth(unum));
 msgtouser(unum,"Done");
 return; 
}

void ch_clearchan(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
  msgtouser(unum,"clearmodes usage:");
  newmsgtouser(unum,"/msg %s clearmodes <#channel>",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 sendtouplink("%s M %s -rilkm *\r\n",servernumeric,tmps3);
 fflush(sockout);
 delchanflag(tmps3,cm_r);
 delchanflag(tmps3,cm_i);
 delchanflag(tmps3,cm_k);
 delchanflag(tmps3,cm_l);
 delchanflag(tmps3,cm_m);
 if (CIsLimit(cp)) {
   ch_setlimit(tmps3,cp->flag_limit);
 }  
 if (CIsInvite(cp)) {
   sendtouplink("%sAAB M %s +i\r\n",servernumeric,tmps3);
   setchanflag(tmps3,cm_i);
 }
 if (CIsHidden(cp)) {
   sendtouplink("%sAAB M %s +s\r\n",servernumeric,tmps3);
   setchanflag(tmps3,cm_s);
 }
 if (CIsKey(cp) && cp->flag_key[0]!='\0') {
   sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,tmps3,cp->flag_key);
   setchanflag(tmps3,cm_k);
   setchankey(tmps3,cp->flag_key);
 }
 if (CIsForward(cp) && cp->flag_fwchan[0]!='\0') {
   sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,tmps3,cp->flag_fwchan);
   setchanflag(tmps3,cm_F);
   setfwchan(tmps3,cp->flag_fwchan);
 }
 fflush(sockout);
 ch_msgtolog(cp,"%s(%s) used clearchan",unum2nick(unum),unum2auth(unum));
 msgtouser(unum,"Done");
 return; 
}

void ch_recover(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
  msgtouser(unum,"recover usage:");
  newmsgtouser(unum,"/msg %s recover <#channel>",mynick);
  return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 sendtouplink("%s CM %s brilkm\r\n",servernumeric,tmps3);
 deopall(cp->cptr);
 sendtouplink("%s M %s +o %sAAB\r\n",servernumeric,tmps3,servernumeric);
 fflush(sockout);
 sim_mode(tmps3,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
 delchanfla2(cp->cptr,cm_r);
 delchanfla2(cp->cptr,cm_i);
 delchanfla2(cp->cptr,cm_k);
 delchanfla2(cp->cptr,cm_l);
 delchanfla2(cp->cptr,cm_m);
 
 if (CIsLimit(cp)) {
   ch_setlimit2(cp->cptr,cp->flag_limit);
 }  
 if (CIsInvite(cp)) {
   sendtouplink("%sAAB M %s +i\r\n",servernumeric,tmps3);
   setchanflag(tmps3,cm_i);
 }
 if (CIsHidden(cp)) {
   sendtouplink("%sAAB M %s +s\r\n",servernumeric,tmps3);
   setchanflag(tmps3,cm_s);
 }
 if (CIsKey(cp) && cp->flag_key[0]!='\0') {
   sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,tmps3,cp->flag_key);
   setchanflag(tmps3,cm_k);
   setchankey(tmps3,cp->flag_key);
 }
 if (CIsForward(cp) && cp->flag_fwchan[0]!='\0') {
   sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,tmps3,cp->flag_fwchan);
   setchanflag(tmps3,cm_F);
   setfwchan(tmps3,cp->flag_fwchan);
 }
 fflush(sockout);
 ch_msgtolog(cp,"%s(%s) used recover",unum2nick(unum),unum2auth(unum));
 msgtouser(unum,"Done");
 return;   
}

void ch_chaninfo(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %[^\n]",tmps2,tmps3,tmps4);
 if ((res!=3) && (res!=2)) {
   msgtouser(unum,"chaninfo usage:");
   newmsgtouser(unum,"/msg %s chaninfo <#channel> [info]",mynick);
   msgtouser(unum,"You can view/set contact information, channel url");
   msgtouser(unum,"or a small text about the channel.");
   return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (res==2) {
   if (cp->info[0]=='\0') {
     msgtouser(unum,"No information avalible");
     return;
   }  
   newmsgtouser(unum,"Channel information about %s:",tmps3);
   newmsgtouser(unum,"%s",cp->info);
   return;
 } else {
   if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,500)) {
     newmsgtouser(unum,"You are not known on %s",tmps3);
     return;
   }    
   mystrncpy(cp->info,tmps4,TOPICLEN);
   newmsgtouser(unum,"Channel information about %s changed to:",tmps3);
   newmsgtouser(unum,"%s",cp->info);
 }
 return;  
}

void ch_set(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
 if ((res !=4) && (res !=3)) {
   msgtouser(unum,"chanset usage:");
   newmsgtouser(unum,"/msg %s chanset <#channel> <param> [value]",mynick);
   newmsgtouser(unum,"For more information do /msg %s help chanset",mynick);
   return;
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) || !checkauthlevel(unum,500)) {
   newmsgtouser(unum,"You are not known on %s",tmps3);
   return;
 }
 
 toLowerCase2(tmps3);
 
 if (res == 3) {  
   /* view */
   if (strcmp(tmps4,"limit")==0) {
     sprintf(tmps5,"%i",cp->flag_limit);
   } else if (strcmp(tmps4,"welcome")==0) {
     sprintf(tmps5,"%s",cp->flag_welcome);
   } else if (strcmp(tmps4,"key")==0 && (UHasOwner(up) || UHasCoowner(up) || UHasOp(up))) {
     sprintf(tmps5,"%s",cp->flag_key);
   } else if (strcmp(tmps4,"suspendlevel")==0 && checkauthlevel(unum,600)) {
     sprintf(tmps5,"%i",cp->flag_suspendlevel);
   } else if (strcmp(tmps4,"fwchan")==0 && checkauthlevel(unum,600)) {
     sprintf(tmps5,"%s",cp->flag_fwchan);       
   } else {
     msgtouser(unum,"Unknown flag or no permission.");
     return;
   }     
   newmsgtouser(unum,"Flag %s = %s",tmps4,tmps5);
 } else {
   /* modify */
   if (strcmp(tmps4,"limit")==0 && (UHasOwner(up) || UHasCoowner(up) || UHasOp(up))) {
     if (atoi(tmps5)>10 || atoi(tmps5)<1) {
       msgtouser(unum,"limit has to be between 1 and 10");
       return;
     }  
     cp->flag_limit=atoi(tmps5);
     ch_setlimit(tmps3,cp->flag_limit);
   } else if (strcmp(tmps4,"welcome")==0 && (UHasOwner(up) || UHasCoowner(up))) {
     mystrncpy(cp->flag_welcome,tmps5,TOPICLEN+1);
   } else if (strcmp(tmps4,"key")==0 && (UHasOwner(up) || UHasCoowner(up))) {
     mystrncpy(cp->flag_key,tmps5,CHANKEYLEN+1);
     if (CIsKey(cp)) {
       sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,tmps3,tmps5);
       setchanflag(tmps3,cm_k);
       setchankey(tmps3,tmps5);
       fflush(sockout);
     }  
   } else if (strcmp(tmps4,"suspendlevel")==0 && checkauthlevel(unum,600)) {
     cp->flag_suspendlevel=atoi(tmps5);
   } else if (strcmp(tmps4,"fwchan")==0 && checkauthlevel(unum,600)) {
     toLowerCase2(tmps5);
     mystrncpy(cp->flag_fwchan,tmps5,CHANNELLEN+1);       
   } else {
     msgtouser(unum,"Unknown flag or no permission.");
     return;
   }     
   newmsgtouser(unum,"Changed flag %s to %s",tmps4,tmps5);
 }
   
}

void ch_access(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], acc[NICKLEN+1];
 int res, t1, t2, a=0, d=0, i; long un;
 cuser *up, *up2; rchan *cp;
 userdata *ux;
 
 res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
 if ((res !=4) && (res !=2)) {
   msgtouser(unum,"access usage:");
   newmsgtouser(unum,"/msg %s access <#channel> [<nickname|#account> <+|-><flags>]",mynick);
   newmsgtouser(unum,"For more information do /msg %s help access",mynick);
   return;
 }
 toLowerCase2(tmps3);
 toLowerCase2(tmps4);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!((up=ch_getchanuser(cp,unum2auth(unum))) || checkauthlevel(unum,500))) {
   newmsgtouser(unum,"You are not known on %s",tmps3);
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 if (res == 4) {
   t1=0; t2=0;
   while (tmps5[t2]!='\0') {
     if ((tmps5[t2]=='+') || (tmps5[t2]=='-')) {
       if (tmps5[t2]=='+') { t1=1; } else { t1=0; }
     } else {
       if (t1==1) {
         a|=flagstoint(tuflags,&tmps5[t2]);
       } else {
         d|=flagstoint(tuflags,&tmps5[t2]);
       }
     }
     t2++;
   }
   if (!a && !d) {
     msgtouser(unum,"No valid access flags given");
     return;
   }
   if (tmps4[0] == '#') {
     /* account */
     res=sscanf(tmps4,"#%s",acc);
     if (uhacc(acc)!=0) {
       msgtouser(unum,"User has no account!");
       return;
     }
   } else {
     /* need to get account */
     un=nicktonum(tmps4);
     if (un<0) {
       newmsgtouser(unum,"User %s is not on the network",tmps4);
       return;
     }
     ux=getudptr(un);
     if (!ischarinstr('r',ux->umode)) {
       newmsgtouser(unum,"User %s is not authed",tmps4);
       return;
     }  
     mystrncpy(acc,ux->authname,NICKLEN);       
   }
   up2=ch_getchanuser(cp,acc);
   /* permissions checks */     
   if (!UHasOwner(up) && ((d & (UF_OWNER|UF_COOWNER)) || (a & (UF_OWNER|UF_COOWNER)))) {
     newmsgtouser(unum, "You don't have permission to add or remove (co)owners on %s",tmps3);
     return;
   }  
   if (up != up2 && ((d & UF_LOG) || (a & UF_LOG))) {
     msgtouser(unum, "You can only set or remove the +l flag on your self");
     return;
   }
   if ((!UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up)) && (a & UF_LOG)) {
     msgtouser(unum, "You need the +o flag or higher to set +l");
     return;
   }  
   if (!(UHasCoowner(up) || UHasOwner(up) || checkauthlevel(unum,800) || ((d == UF_LOG) && (a == 0)) || (UHasOp(up) && ((d == 0) && (a == UF_LOG))))) {
     msgtouser(unum, "You don't have the permission");
     return;
   }
   /* add or remove user? */
   if (!up2 && (a>0)) {
     /* add */
     /* we should ask auth.c if user has an account.... */
     up2=(cuser*)malloc(sizeof(cuser));
     mystrncpy(up2->name,acc,NICKLEN+1);
     up2->aflags=a;
     if (!cp->cusers)
       up2->next=NULL;
     else
       up2->next=cp->cusers;
     cp->cusers=up2;
   } else if (!up2 && a==0) {
     /* nothing */
     newmsgtouser(unum, "User %s is not known on %s", acc, tmps3);
     return;
   } else {
     /* update */
     up2->aflags |= a;
   }
   up2->aflags &= ~d;
   if (up2->aflags==0) {
     /* delete */
     cuser *upl=0;
     for(up=cp->cusers;up;up=up->next) {
       if (strcmp(up->name,acc)==0) {
         if (upl) 
           upl->next=up->next;
         else
           cp->cusers=up->next;
	 free(up);  
         newmsgtouser(unum, "Removed %s from %s", acc, tmps3);
	 return;
       }
       upl=up;  
     }
     /* this SHOULD never happen */
     return;
   }    
   newmsgtouser(unum, "New Accessflags for %s on %s: +%s", acc, tmps3, flagstostr(tuflags,up2->aflags));
   return;
 } else { 
   /* show accessflags on a chan */
   newmsgtouser(unum,"--- Accessflags on %s ---",tmps3);   
   i=0;
   for(up2=cp->cusers;up2;up2=up2->next) { 
    sprintf(tmps2,"»  %s\t+%s",up2->name,flagstostr(tuflags,up2->aflags));
    printhelp(unum,tmps2);
    i++;
   }
   newmsgtouser(unum,"--- End of List (%i user%s)---",i,(i>1) ? "s" : "");
   return;
 } 
}

void ch_chanflags(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res, t1, t2, a=0, d=0, y=0;
 cuser *up, *up2; rchan *cp;
 channel *ncp; chanuser  *cu; userdata *u;
 
 res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if ((res !=3) && (res !=2)) {
   msgtouser(unum,"chanflags usage:");
   newmsgtouser(unum,"/msg %s chanflags <#channel> [<+|-><flags>]",mynick);
   newmsgtouser(unum,"For more information do /msg %s help chanflags",mynick);
   return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) || !checkauthlevel(unum,500)) {
   newmsgtouser(unum,"You don't have permission on %s",tmps3);
   return;
 }
 if (CIsSuspended(cp)) {
   msgtouser(unum,"Channel is suspended!");
   if (!isircop(unum)) { return; }
 }
 if (res == 3) {
   t1=0; t2=0;
   while (tmps4[t2]!='\0') {
     if ((tmps4[t2]=='+') || (tmps4[t2]=='-')) {
       if (tmps4[t2]=='+') { t1=1; } else { t1=0; }
     } else {
       if (t1==1) {
         a|=flagstoint(tcflags,&tmps4[t2]);
       } else {
         d|=flagstoint(tcflags,&tmps4[t2]);
       }
     }
     t2++;
   }
   if (!a && !d) {
     msgtouser(unum,"No valid channel flags given");
     return;
   }
 
   if (!(UHasOwner(up) || UHasCoowner(up) || checkauthlevel(unum,600))) {
     newmsgtouser(unum, "You don't have permission on %s",tmps3);
     return;
   }  
   
   if (!checkauthlevel(unum,800) && ((d & (CF_SUSPENDED|CF_FORWARD)) || (a & (CF_SUSPENDED|CF_FORWARD)))) {
     newmsgtouser(unum, "You don't have permission on %s",tmps3);
     return;
   }  
   
   if (a & CF_FORCE) {
     d|=CF_PROTECT+CF_SECURE;
   }
   if (a & CF_PROTECT) {
     d|=CF_FORCE+CF_SECURE;
   }
   if (a & CF_SECURE) {
     d|=CF_FORCE+CF_PROTECT;
   }
   if (a & (CF_FORWARD|CF_SUSPENDED)) {
     a|=CF_HIDDEN;
   }
     
   cp->cflags |= a;
   cp->cflags &= ~d;
 
   ncp=cp->cptr;
     
   if (CIsForce(cp)) {
     cu=(chanuser *)ncp->users.content;
     for (y=0; y < ncp->users.cursi; y++) {
       if (isflagset(cu[y].flags,um_o)) {
         up2=ch_getchanuser(cp,unum2auth(cu[y].numeric));
         u=getudptr(cu[y].numeric);
         if (ischarinstr('k',u->umode) || ischarinstr('X',u->umode)) { continue; }
         if (!UHasOp(up2)) {
           longtotoken(cu[y].numeric,tmps2,5);
           sendtouplink("%sAAB M %s -o %s\r\n",servernumeric,tmps3,tmps2);
           fflush(sockout);
	   sim_mode(tmps3,"-o",cu[y].numeric);
         }
       }
       if (isflagset(cu[y].flags,um_v)) {
         up2=ch_getchanuser(cp,unum2auth(cu[y].numeric));
         if (!UHasVoice(up2)) {
           longtotoken(cu[y].numeric,tmps2,5);
           sendtouplink("%sAAB M %s -v %s\r\n",servernumeric,tmps3,tmps2);
           fflush(sockout);
	   sim_mode(tmps3,"-v",cu[y].numeric);
         }
       }
     }  
   }  
   if (CIsSecuremode(cp)) {
     sendtouplink("%s CM %s o\r\n",servernumeric,tmps3);
     sendtouplink("%s M %s +o %sAAB\r\n",servernumeric,tmps3,servernumeric);
     fflush(sockout);
     ncp=getchanptr(tmps3);
     cu=(chanuser *)ncp->users.content;
     for (y=0;y<ncp->users.cursi;y++) {
       if (isflagset(cu[y].flags,um_o)) {
         changechanmod2(ncp,cu[y].numeric,2,um_o);
       }
     }
     sim_mode(tmps3,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
   }
   if (CIsLimit(cp)) {
     ch_setlimit2(cp->cptr,cp->flag_limit);
   }  
   if (CIsInvite(cp)) {
     sendtouplink("%sAAB M %s +i\r\n",servernumeric,tmps3);
     setchanflag(tmps3,cm_i);
   }
   if (CIsHidden(cp)) {
     sendtouplink("%sAAB M %s +s\r\n",servernumeric,tmps3);
     setchanflag(tmps3,cm_s);
   }
   if (CIsKey(cp) && cp->flag_key[0]!='\0') {
     sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,tmps3,cp->flag_key);
     setchanflag(tmps3,cm_k);
     setchankey(tmps3,cp->flag_key);
   }
   if (CIsForward(cp) && cp->flag_fwchan[0]!='\0') {
     sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,tmps3,cp->flag_fwchan);
     setchanflag(tmps3,cm_F);
     setfwchan(tmps3,cp->flag_fwchan);
   }
   fflush(sockout);
   newmsgtouser(unum, "New Channelflags on %s: +%s", tmps3, flagstostr(tcflags,cp->cflags));
 } else {
   newmsgtouser(unum, "Channelflags on %s: +%s", tmps3, flagstostr(tcflags,cp->cflags));
 }
 return;
}

void ch_add(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  int res;
  rchan *cp;
  cuser *up;
  userdata *reqrec;
  long nettime;
  
  res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res != 3) {
   msgtouser(unum,"addchan usage:");
   newmsgtouser(unum,"/msg %s addchan <channel> <owner>",mynick);
   return;
  }
  toLowerCase2(tmps3);
  toLowerCase2(tmps4);
  if (uhacc(tmps4)!=0) {
    msgtouser(unum,"Owner has no account!");
    return;
  } 
  reqrec=getudptr(unum);
  if (!ch_getchan(tmps3)) {
   nettime=getnettime();
   channel *c;
   c=getchanptr(tmps3);
   if (c==NULL) { 
    msgtouser(unum,"Sorry, nobody is on that channel!");
    return;
   }
   cp=(rchan*)malloc(sizeof(rchan));
   mystrncpy(cp->name,c->name,CHANNELLEN+1);
   mystrncpy(cp->creator,tmps4,NICKLEN+1);
   cp->cdate=nettime;
   cp->cflags=CM_DEFAULT;
   cp->cptr=c;
   cp->flag_limit=DEF_LIMIT;
   cp->flag_suspendlevel=0;
   cp->flag_welcome[0]='\0';
   cp->flag_key[0]='\0';
   cp->flag_fwchan[0]='\0';
   cp->info[0]='\0';
   cp->suspended_since=0;
   cp->suspended_until=0;
   cp->suspended_by[0]='\0';
   cp->lastused=nettime;
   up=(cuser*)malloc(sizeof(cuser));
   mystrncpy(up->name,tmps4,NICKLEN+1);
   up->aflags=UA_DEFAULT;
   if (!cp->cusers)
     up->next=NULL;
   else
     up->next=cp->cusers;
   cp->cusers=up;
   if (!rchans)
     cp->next=NULL;
   else
     cp->next=rchans;
   rchans=cp;
   sendtouplink("%sAAB J %s %ld\r\n",servernumeric,c->name,nettime);
   sim_join(tmps3,(tokentolong(servernumeric)<<SRVSHIFT)+1);
   sendtouplink("%s OM %s +o %sAAB\r\n",servernumeric,tmps3,servernumeric);  
   sim_mode(tmps3,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
   msgtouser(unum,"Well Done");
  } else {
   msgtouser(unum,"Sorry, I am already on this channel.");
  }

}

void ch_del(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  userdata *reqrec; rchan *cp;
  int res2;
  reqrec=getudptr(unum);
  res2=sscanf(tail,"%s %s %[^\n]",tmps2,tmps3,tmps4);
  if (res2!=2 && res2!=3) {
    msgtouser(unum,"delchan usage:");
    msgtouser(unum,"/msg X delchan <chan> [reason]");
    return;
  }
  toLowerCase2(tmps3);
  if (!(cp=ch_getchan(tmps3))) {
    newmsgtouser(unum,"I am not on %s",tmps3);
    return;
  }
  if (res2==2) 
    strcpy(tmps4,"Requested");
  ch_chandel(tmps3, tmps4);
  newmsgtouser(unum,"Left %s",tmps3);
}

void ch_suspend(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE], tmps7[TMPSSIZE];
 int res, lvl=0; long d, j;
 channel *c; array chancopy; chanuser *u; long i;
 clearhelp *ch; userdata *ud; rchan *cp;
 
 res=sscanf(tail,"%s %s %s %s %[^\n]",tmps2,tmps3,tmps4,tmps5,tmps6);
 if (res != 4 && res != 5) {
   msgtouser(unum,"suspendchan usage:");
   newmsgtouser(unum,"/msg %s suspendchan <channel> <level> <duration> [reason]",mynick);
   msgtouser(unum,"levels:");
   msgtouser(unum,"        1 = only X is disabled");
   msgtouser(unum,"        2 = badchan");
   msgtouser(unum,"        3 = on join kick");
   msgtouser(unum,"        4 = on join kill");
   msgtouser(unum,"        5 = on join gline user@host");
   msgtouser(unum,"        6 = on join gline *@host");
   newmsgtouser(unum,"the glines at level 5 and 6 last %i minutes",SUSPGL_DUR/60);
   return;
 }
 toLowerCase2(tmps3);
 if (!(cp=ch_getchan(tmps3))) {
    newmsgtouser(unum,"I am not on %s",tmps3);
    return;
  }
 if (res == 4)
   strcpy(tmps6,"Requested");
 if (strcmp("1",tmps4)==0) { lvl=1; }
 if (strcmp("2",tmps4)==0) { lvl=2; }
 if (strcmp("3",tmps4)==0) { lvl=3; }
 if (strcmp("4",tmps4)==0) { lvl=4; }
 if (strcmp("5",tmps4)==0) { lvl=5; }
 if (strcmp("6",tmps4)==0) { lvl=6; }
 if (!lvl) { 
   msgtouser(unum,"Invalid gline level");
   return;
 }
 d=durationtolong(tmps5);
 if (d<=0) {
   msgtouser(unum,"The duration you gave is invalid.");
   return;
 }
 cp->flag_suspendlevel=lvl;
 mystrncpy(cp->suspended_by,unum2auth(unum),NICKLEN);
 mystrncpy(cp->flag_welcome,tmps6,TOPICLEN);
 cp->suspended_since=getnettime();
 cp->suspended_until=getnettime()+d;
 cp->cflags|=(CF_SUSPENDED+CF_HIDDEN);
 /* clear chan */
 if (lvl==2) {
   addgline(tmps3, tmps6, unum2auth(unum), d, 1);
 } else if (lvl==3) {
   /* small hack to kick everybody out of the chan */
   addgline(tmps3, tmps6, unum2auth(unum), 5, 1);
 } else if (lvl==4 || lvl==5 || lvl==6) {
   c=cp->cptr;
   array_init(&chancopy,sizeof(clearhelp)); /* We copy the important data for */
   u=(chanuser *)c->users.content;          /* the chan so that we don't have */
   for (i=0;i<c->users.cursi;i++) {         /* to worry about NULL pointers   */
     ud=getudptr(u[i].numeric);             /* later */
     if (ud!=NULL) {
       if (!ischarinstr('o',ud->umode)) { /* Not an oper -> fuck him */
         if ((ud->numeric>>SRVSHIFT)!=tokentolong(servernumeric)) { /* One of our fakeusers */
           j=array_getfreeslot(&chancopy);
           ch=(clearhelp *)chancopy.content;
           ch[j].num=ud->numeric;
           strcpy(ch[j].host,ud->host);
           strcpy(ch[j].ident,ud->ident);
         }
       }
     }
   }
   if (lvl==4) {
     ch=(clearhelp *)chancopy.content;
     for (i=0;i<chancopy.cursi;i++) {
       longtotoken(ch[i].num,tmps2,5);
       fprintf(sockout,"%s D %s :%s %s\r\n",servernumeric,tmps2,myname,tmps6);
       delchanfromuser(ch[i].num,tmps3);
       deluserfromchan(tmps3,ch[i].num);
       killuser(ch[i].num);
     }
     newmsgtouser(unum,"%ld users killed for being in %s.",chancopy.cursi,tmps3);
     return;
   } else {
     ch=(clearhelp *)chancopy.content;
     for (i=0;i<chancopy.cursi;i++) {
       if (lvl==5) {
         sprintf(tmps2,"%s@%s",ch[i].ident,ch[i].host);
       } else {
         sprintf(tmps2,"*@%s",ch[i].host);
       }
       addgline(tmps2,tmps6,unum2auth(unum),SUSPGL_DUR,1);
       sprintf(tmps7,"GLINE %s, expires in %ds, set by %s: %s (suspended)",tmps2,SUSPGL_DUR,unum2auth(unum),tmps6);
       sendtonoticemask(NM_GLINE,tmps7);
     }
     sprintf(tmps2,"%ld users glined for being in %s. (may have hurt innocent people too)",chancopy.cursi,tmps3);
     msgtouser(unum,tmps2);
   }  
 }     
 msgtouser(unum,"Done");
}  

void ch_unsuspend(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res;
 rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
   msgtouser(unum,"unsuspendchan usage:");
   newmsgtouser(unum,"/msg %s unsuspendchan <#channel>",mynick);
   return;
 }  
 toLowerCase2(tmps3);
 cp=ch_getchan(tmps3); 
 if (CIsSuspended(cp)) {
   cp->suspended_since=0;
   cp->suspended_until=0;
   cp->suspended_by[0]='\0';
   cp->flag_welcome[0]='\0';
   cp->cflags &= ~CF_SUSPENDED;
 } else {
   newmsgtouser(unum,"%s is not suspended",tmps3);
   return;
 }
 msgtouser(unum,"Done");
}

void ch_suspendlist(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 int res, i=0;
 rchan *cp;
 
 res=sscanf(tail,"%s %s",tmps2,tmps3);
 if (res != 2) {
   msgtouser(unum,"suspendlist usage:");
   newmsgtouser(unum,"/msg %s suspendlist <mask>",mynick);
   return;
 }
 toLowerCase2(tmps3);
 newmsgtouser(unum,"%-45s %-25s (%s) by %s: %s","channel","expires in","level","whom","reason");
 for(cp=rchans;cp;cp=cp->next) {
   if (CIsSuspended(cp)) {
     if (match2strings(tmps3, cp->name)) {
       longtoduration(tmps2,cp->suspended_until-getnettime());
       newmsgtouser(unum,"%-45s %-25s (%i) by %s: %s",cp->name,tmps2,cp->flag_suspendlevel,
		cp->suspended_by,cp->flag_welcome);
       i++;
     }
   }
 }
 newmsgtouser(unum,"--- End of list - %d matches ---",i);    	
}

/* Channel module Server Handlers */
void ch_domode() {
 int t1, t2;
 if (paramcount<4) { return; }
 if (params[2][0]=='#') {
  rchan *cp=0; cuser *up; char *c;
  long ux=0;
  toLowerCase2(params[2]);
  cp=ch_getchan(params[2]);
  if (cp==NULL) { return; }
  t2=0;
  c=&params[3][0]; t1=4;
  for (;(*c!='\0');c++) {
   if (*c=='+') { t2=0; continue; }
   if (*c=='-') { t2=1; continue; }
   if ((*c=='k') || (*c=='b') || ((t2==0) && ((*c=='l') || (*c=='F')))) { t1++; }
   if (*c=='o') {
     if (paramcount<=t1) {
       return;
     } else {
       normnum(params[t1]);
       ux=tokentolong(params[t1]);
       if (ux==0) { continue; }
       up=ch_getchanuser(cp,unum2auth(ux));
       if (t2==1) {
         /* deop */
         if ((CIsProtect(cp) || CIsForce(cp)) && UHasOp(up)) {
           sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,params[2],params[t1]);
           sim_mode(params[2],"+o",ux);
         }
       } else {
         /* op */
         if (CIsForce(cp) && !(UHasOp(up) || UHasOwner(up))) {
           sendtouplink("%sAAB M %s -o %s\r\n",servernumeric,params[2],params[t1]);
           sim_mode(params[2],"-o",ux);
         }
         if (CIsSecuremode(cp) && !UHasOwner(up)) {
           sendtouplink("%sAAB M %s -o %s\r\n",servernumeric,params[2],params[t1]);
           sim_mode(params[2],"-o",ux);
         }
       }
       fflush(sockout); 
       t1++;
     }
   } else if (*c=='v') {
     if (paramcount<=t1) {
       return;
     } else {
       normnum(params[t1]);
       ux=tokentolong(params[t1]);
       if (ux==0) { continue; }
       up=ch_getchanuser(cp,unum2auth(ux));
       if (t2==1) {
         if (CIsForce(cp) && UHasVoice(up)) {
           sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,params[2],params[t1]);
           sim_mode(params[2],"+v",ux);
         }
       } else {
         if (CIsForce(cp) && !(UHasVoice(up) || UHasOwner(up) || CIsAutovoice(cp))) {
           sendtouplink("%sAAB M %s -v %s\r\n",servernumeric,params[2],params[t1]);
           sim_mode(params[2],"-v",ux);
         }
       }
       fflush(sockout);
       t1++;
     }
   } else if (*c=='l' && t2==1 && CIsLimit(cp)) {
     ch_setlimit(params[2],cp->flag_limit);
   } else if (*c=='i' && t2==1 && CIsInvite(cp)) {
     sendtouplink("%sAAB M %s +i\r\n",servernumeric,params[2]);
     setchanflag(params[2],cm_i);  
   } else if (*c=='s' && t2==1 && CIsHidden(cp)) {
     sendtouplink("%sAAB M %s +s\r\n",servernumeric,params[2]);
     setchanflag(params[2],cm_s);
   } else if (*c=='k' && t2==1 && CIsKey(cp)) {
     sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,params[2],cp->flag_key);
     setchanflag(params[2],cm_k);
     setchankey(params[2],cp->flag_key);
   } else if (*c=='F' && t2==1 && CIsForward(cp)) {
     sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,params[2],cp->flag_fwchan);
     setchanflag(params[2],cm_F);
     setchankey(params[2],cp->flag_fwchan);
   }  
   fflush(sockout);
  }     
 }
}

void ch_dojoin() {
 long num, ntime;
 int op=0,voice=0; 
 userdata *ux;
 cuser *up; rchan *cp;
 
 if (paramcount<3) { return; }
 ntime=getnettime();
 num=tokentolong(sender);
 toLowerCase2(params[2]);
 if (!(cp=ch_getchan(params[2]))) {
   return;
 }
 if (CIsSuspended(cp)) {
   /* groove on */
   if (cp->flag_suspendlevel==3) {
     /* kick */
     sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,params[2],sender,cp->flag_welcome);
     fflush(sockout);
     delchanfromuser(num,params[2]);
     deluserfromchan(params[2],num);
   } else if (cp->flag_suspendlevel==4) {
     /* kill */
     sendtouplink("%s D %s :%s %s\r\n",servernumeric,sender,mynick,cp->flag_welcome);
     deluserfromallchans(num);
     fflush(sockout);
     killuser(num);
   }  
   return;
 } 
 if (cp->had_s == 0) {
   if (!CIsHidden(cp)) {
     sendtouplink("%sAAB M %s -s\r\n",servernumeric,cp->name);
     fflush(sockout);
     delchanfla2(cp->cptr,cm_s);
   }
   cp->had_s=-1;
 }
 ux=getudptr(num);
 if (!ischarinstr('r',ux->umode)) {
   return; 
 }
  
 if (CIsAutovoice(cp)) {
   voice=1;
 }
 
 if ((up=ch_getchanuser(cp,unum2auth(num)))) {
   if (ntime) 
     cp->lastused=ntime;
   if (UHasVoice(up) && (UHasGive(up) || UHasAuto(up))) {
     voice=1;
   }
   if (UHasOp(up) && UHasAuto(up) && !CIsSecuremode(cp)) {
     op=1;
   }    
 }

 if (CIsWelcome(cp) && cp->flag_welcome[0]!='\0' && !CIsSuspended(cp)) {
   sendtouplink("%sAAB O %s :(%s) %s\r\n",servernumeric,sender,params[2],cp->flag_welcome);
 }
 
 if (voice && op) {
   sendtouplink("%sAAB M %s +ov %s %s\r\n",servernumeric,params[2],sender,sender);
   sim_mode(params[2],"+o",num);
   sim_mode(params[2],"+v",num);
 } else if (voice) {
   sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,params[2],sender);
   sim_mode(params[2],"+v",num);
 } else if (op) {
   sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,params[2],sender);
   sim_mode(params[2],"+o",num);
 }
 fflush(sockout);
 return;
}

/* Autolimit helper */
void ch_setlimit(char *chan, int lim) {
 channel *c;
 c=getchanptr(chan);
 if (c==NULL) { return; }
 sendtouplink("%sAAB M %s +l %i\r\n",servernumeric,chan,c->users.cursi+lim);
 fflush(sockout);
 setchanfla2(c,cm_l);
 setchanli2(c,c->users.cursi+lim);
 return;
}

void ch_setlimit2(channel *c, int lim) {
 if (c==NULL) { return; }
 sendtouplink("%sAAB M %s +l %i\r\n",servernumeric,c->name,c->users.cursi+lim);
 fflush(sockout);
 setchanfla2(c,cm_l);
 setchanli2(c,c->users.cursi+lim);
 return;
}

/* log flag function */

void ch_msgtolog(rchan *cp, const char *template, ...) {
 char tmps[TMPSSIZE], vtmps[TMPSSIZE], target[6];
 int hcount; va_list ap;
 cuser *up; userdata *udata;
 
 va_start(ap,template);
 vsprintf(vtmps,template,ap);
 va_end(ap);
 sprintf(tmps,"[LOG: %s] %s",cp->name,vtmps);
 for(up=cp->cusers;up;up=up->next) { 
   if (UHasLog(up)) {
     /* find all users with that auth */
     for (hcount = 0; hcount < SIZEOFUL; hcount++) {
       udata = uls[hcount];
       while (udata != NULL) {
         if ((strcmp(udata->authname, up->name)==0) && (ischarinstr('r', udata->umode)==1)) {
           longtotoken(udata->numeric, target, 5);
           sendtouplink("%sAAB O %s :%s\r\n",servernumeric,target,tmps);
         }
         udata = (void *)udata->next;
       } 
     }
   }
 }
 fflush(sockout);
}

/* Load X to the Channels */
void ch_joinall() {
  rchan *cp;
  long xtime;

  xtime=getnettime();
  for(cp=rchans;cp;cp=cp->next) {
    sendtouplink("%sAAB J %s %ld\r\n",servernumeric,cp->name,xtime);
    sim_join(cp->name,(tokentolong(servernumeric)<<SRVSHIFT)+1);
    if (!cp->cptr) {
      cp->cptr=getchanptr(cp->name);
    }
    sendtouplink("%s OM %s +o %sAAB\r\n",servernumeric,cp->name,servernumeric);
    sim_mode(cp->name,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
    if (CIsLimit(cp)) {
      ch_setlimit(cp->name,cp->flag_limit);
    }  
    if (CIsInvite(cp)) {
      sendtouplink("%sAAB M %s +i\r\n",servernumeric,cp->name);
      setchanflag(cp->name,cm_i);
    }
    if (CIsHidden(cp)) {
      sendtouplink("%sAAB M %s +s\r\n",servernumeric,cp->name);
      setchanflag(cp->name,cm_s);
    }
    if (CIsKey(cp) && cp->flag_key[0]!='\0') {
      sendtouplink("%sAAB M %s +k %s\r\n",servernumeric,cp->name,cp->flag_key);
      setchanflag(cp->name,cm_k);
      setchankey(cp->name,cp->flag_key);
    }
    if (CIsForward(cp) && cp->flag_fwchan[0]!='\0') {
      sendtouplink("%sAAB M %s +F %s\r\n",servernumeric,cp->name,cp->flag_fwchan);
      sendtouplink("%sAAB T %s :%s\r\n",servernumeric,cp->name,cp->flag_welcome);
      sim_topic(cp->name,cp->flag_welcome);
      setchanflag(cp->name,cm_F);
      setfwchan(cp->name,cp->flag_fwchan);
    }  
  }
  fflush(sockout);
} 

void ch_partall() {
  sendtouplink("%sAAB J 0\r\n",servernumeric);
  sim_join("0",(tokentolong(servernumeric)<<SRVSHIFT)+1);
} 

void ch_chandel(char *chan, char *msg) {
 rchan *cp,*cp2,*xcp=0;
 cuser *up,*up2=0;
 
 cp2=ch_getchan(chan);
 if (cp2==NULL) { return; }
 ch_msgtolog(cp2,"Channel removed: %s",msg);
 sendtouplink("%sAAB L %s :%s\r\n",servernumeric,chan,msg);
 fflush(sockout);
 sim_part(chan,(tokentolong(servernumeric)<<SRVSHIFT)+1);
 for(up=cp2->cusers;up;up=up->next) {
   if (up2) 
     free(up2);
   up2=up;
 }  
 free(up2);
 for(cp=rchans;cp;cp=cp->next) {
   if (cp==cp2) {
     if (xcp)
       xcp->next=cp->next;
     else
       rchans=cp->next;
     free(cp);
     return;
   }
   xcp=cp;
 }
}          
 
/* user has account?! */
/* will be replaced with new auth module */
int uhacc (char *xuser) {
 char tmps[TMPSSIZE],tmps2[TMPSSIZE];
 int res2=0;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 mysql_escape_string(tmps2,xuser,strlen(xuser));
 sprintf(tmps,"SELECT * from Xusers where username='%s'",tmps2);
 res2=mysql_query(&sqlcon,tmps);
 if (res2!=0) {
  putlog("!!! Could not read X users database !!!");
  return -1;
 }
 myres=mysql_store_result(&sqlcon);
 while ((myrow=mysql_fetch_row(myres))) {
  return 0;
 }
 return 1;
}

/* Internal Events */

void ch_ieac(char *xarg) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 long unum; int res, um, i;
 userdata *ux; channel **cx;
 cuser *up; rchan *cp;
 
 res=sscanf(xarg,"%s %s",tmps2,tmps3);
 unum=tokentolong(tmps2);
 ux=getudptr(unum);
 if (ux->chans.cursi>0) {
   for (i=0;i<(ux->chans.cursi);i++) {
     cx=(channel **)(ux->chans.content);
     if ((strlen(cx[i]->name)+strlen(tmps2))>400) {
       break;
     } else {
       if (!(cp=ch_getchan(cx[i]->name))) { continue; }
       if (!(up=ch_getchanuser(cp,tmps3)) && !UHasAuto(up) && !UHasGive(up)) { continue; }  
       if (UHasAuto(up)) {
         um=getchanmode2(cx[i],ux->numeric);
         if (um<0) { continue; }
         if (!isflagset(um,um_o)) { 
           if (UHasOp(up) && UHasAuto(up)) {
             sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,cx[i]->name,tmps2);
	     fflush(sockout);
             sim_mode(cx[i]->name,"+o",tokentolong(tmps2));
          }	
         }
         if (!isflagset(um,um_v)) { 
           if (UHasVoice(up) && (UHasAuto(up) || UHasGive(up))) {
             sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,cx[i]->name,tmps2);
	     fflush(sockout);
             sim_mode(cx[i]->name,"+v",tokentolong(tmps2));
           }
         }
       }
     }
   }
 }
}

void ch_ieeb(char *xarg) {
 if (donej==0 && uplinkup==1) { 
  donej=1;
  ch_readdb();
  ch_joinall(); 
 }
}

void ch_ieonemin(char *xarg) {
  /* go through channels and check for users<2 or limit needs to be updated */
  rchan *rcp; channel *ncp;
  for(rcp=rchans;rcp;rcp=rcp->next) {  
    ncp=rcp->cptr;
    if (CIsSuspended(rcp)) {
      if (rcp->suspended_until<getnettime()) {
        rcp->suspended_since=0;
        rcp->suspended_until=0;
        rcp->suspended_by[0]='\0';
        rcp->flag_welcome[0]='\0';
        rcp->cflags &= ~CF_SUSPENDED;
      }	
    }
    if (CIsLimit(rcp)) {
      ch_setlimit2(ncp,rcp->flag_limit);
    }
    if (ncp->users.cursi<2 && rcp->had_s<0) {
      if (ncp->flags & cm_s) {
        rcp->had_s=1;
      } else {
        rcp->had_s=0;
	sendtouplink("%sAAB M %s +s\r\n",servernumeric,rcp->name);
	setchanfla2(ncp,cm_s);
      }
    }  
  }
}

void ch_iesixtymin(char *xarg) {
  /* let's save the database */
  ch_savedb(0, 0);
}

void ch_iek(char *xarg) {
  ch_joinall(); 
}

/* database things */
void ch_readdb(void) {
 char tmps[TMPSSIZE];
 int res, res2;
 long xtime;
 rchan *cp;
 cuser *up;
 MYSQL_RES *myres, *myres2; MYSQL_ROW myrow, myrow2;
 
 res2=mysql_query(&sqlcon,"SELECT * from chan_db");
 if (res2!=0) {
  putlog("!!! Could not read channel database !!!");
  return;
 }
 myres=mysql_store_result(&sqlcon);
 xtime=getnettime();
 while ((myrow=mysql_fetch_row(myres))) {
  cp=(rchan*)malloc(sizeof(rchan));
  if (cp==NULL) {
    putlog("!!! BUY MORE MEM !!!");
    return;
  }  
  mystrncpy(cp->name,myrow[0],strlen(myrow[0]));
  mystrncpy(cp->creator,myrow[1],strlen(myrow[1]));
  cp->cdate=atol(myrow[2]);
  cp->cflags=atoi(myrow[3]);
  cp->cptr=getchanptr(cp->name);
  cp->had_s=-1;
  cp->flag_limit=atoi(myrow[4]);
  cp->flag_suspendlevel=atoi(myrow[5]);
  mystrncpy(cp->flag_welcome,myrow[6],strlen(myrow[6]));
  mystrncpy(cp->flag_key,myrow[7],strlen(myrow[7]));
  mystrncpy(cp->flag_fwchan,myrow[8],strlen(myrow[8]));
  mystrncpy(cp->info,myrow[9],strlen(myrow[9]));
  cp->suspended_since=atol(myrow[10]);
  cp->suspended_until=atol(myrow[11]);
  mystrncpy(cp->suspended_by,myrow[12],strlen(myrow[12]));
  cp->lastused=atol(myrow[13]);
  /* user flags */
  sprintf(tmps,"SELECT * from access_db where xchan='%s'",myrow[0]);
  res=mysql_query(&sqlcon,tmps);
  if (res!=0) {
    putlog("!!! Could not read channel access database !!!");
    return;
  }
  myres2=mysql_store_result(&sqlcon);
  while ((myrow2=mysql_fetch_row(myres2))) {
    up=(cuser*)malloc(sizeof(cuser));
    mystrncpy(up->name,myrow2[1],strlen(myrow2[1]));
    up->aflags=atoi(myrow2[3]);
    if (!cp->cusers)
      up->next=NULL;
    else
      up->next=cp->cusers;
    cp->cusers=up;
  }
  mysql_free_result(myres2);
  if (!rchans)
    cp->next=NULL;
  else
    cp->next=rchans;
  rchans=cp;
 }
 mysql_free_result(myres);
}

void ch_savedb(long unum, char *tail) {
  char tmps[TMPSSIZE], chna[TMPSSIZE], crna[TMPSSIZE], fwel[TMPSSIZE];
  char fkey[TMPSSIZE], ffwc[TMPSSIZE], cinf[TMPSSIZE], spna[TMPSSIZE];
  rchan *cp;
  cuser *up;
  int c=0,u=0;
  /* empty old db */
  mysql_query(&sqlcon,"TRUNCATE TABLE chan_db");
  mysql_query(&sqlcon,"TRUNCATE TABLE access_db");
  /* write new */
  for(cp=rchans;cp;cp=cp->next) {
    mysql_escape_string(chna,cp->name,strlen(cp->name));
    mysql_escape_string(crna,cp->creator,strlen(cp->creator));
    mysql_escape_string(fwel,cp->flag_welcome,strlen(cp->flag_welcome));
    mysql_escape_string(fkey,cp->flag_key,strlen(cp->flag_key));
    mysql_escape_string(ffwc,cp->flag_fwchan,strlen(cp->flag_fwchan));
    mysql_escape_string(cinf,cp->info,strlen(cp->info));
    mysql_escape_string(spna,cp->suspended_by,strlen(cp->suspended_by));
    sprintf(tmps,"INSERT INTO chan_db (xchan, creator, cdate, cflags, flimit, fsuspendlvl, fwelcome, "
    		"fkey, ffwchan, info, susp_since, susp_until, susp_by, lastused) VALUES ('%s','%s',%ld,'%d','%d',"
		"'%d','%s','%s','%s','%s','%ld','%ld','%s','%ld')",chna,crna,cp->cdate,cp->cflags,
		cp->flag_limit,cp->flag_suspendlevel,fwel,fkey,ffwc,cinf,
		cp->suspended_since,cp->suspended_until,spna,cp->lastused);
    mysql_query(&sqlcon,tmps);
    for(up=cp->cusers;up;up=up->next) {
      mysql_escape_string(crna,up->name,strlen(up->name));
      mysql_escape_string(chna,cp->name,strlen(cp->name));
      sprintf(tmps,"INSERT INTO access_db (xuser, xchan, aflags) VALUES ('%s','%s','%i')",crna,chna,up->aflags);
      mysql_query(&sqlcon,tmps);
      u++;
    }
    c++;
  }
  if (unum) {
    newmsgtouser(unum,"Wrote %i channels and %i users into the db",c,u);
  }
  putlog("Wrote %i channels and %i users into the db",c,u); 
}

void ch_createdb(long unum, char *tail) {
  char tmps[TMPSSIZE];
  
  msgtouser(unum,"Attempting to make the channel database");
  sprintf(tmps,"CREATE TABLE chan_db (xchan varchar(%i) NOT NULL, creator varchar(%i) NOT NULL,"
  	" cdate bigint NOT NULL, cflags int(11) DEFAULT '%i', flimit int(11) DEFAULT '%i',"
	" fsuspendlvl int(11) DEFAULT '0', fwelcome varchar(%i),"
	" fkey varchar(%i), ffwchan varchar(%i), info varchar(%i), susp_since bigint, "
	" susp_until bigint, susp_by varchar(%i), lastused bigint, PRIMARY KEY (xchan), "
	"UNIQUE xchan (xchan))",CHANNELLEN,NICKLEN,CM_DEFAULT,DEF_LIMIT,TOPICLEN,CHANKEYLEN,
	CHANNELLEN,TOPICLEN,NICKLEN);	
  mysql_query(&sqlcon,tmps);
  sprintf(tmps,"CREATE TABLE access_db (aid int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
  	" xuser varchar(%i) NOT NULL, xchan varchar(%i) NOT NULL, aflags int(11),"
	" PRIMARY KEY (aid), UNIQUE aid (aid))",NICKLEN,CHANNELLEN);
  mysql_query(&sqlcon,tmps);
  msgtouser(unum,"Yeah Done");
}

/* help functions */

rchan *ch_getchan(char *chan) {
  rchan *cp;
  for(cp=rchans;cp;cp=cp->next) {
    if (strcmp(cp->name,chan)==0) 
      return cp;
  }
  return 0;
}      

cuser *ch_getchanuser(rchan *cp, char *account) {
  cuser *up;
  for(up=cp->cusers;up;up=up->next) {
    if (strcmp(up->name,account)==0) 
      return up;
  }
  return &nulluser;
}     

cuser *ch_getchanuserbynick(rchan *cp, char *nick) {
  cuser *up;
  for(up=cp->cusers;up;up=up->next) {
    if (strcmp(up->name,unum2auth(nicktonum(nick)))==0) 
      return up;
  }
  return 0;
}

/* Module Things */
void chan_init() {
  setmoduledesc(MODNAM,"Xevres Channel module");
  nulluser.aflags=0;
  registerserverhandler(MODNAM,"M",ch_domode);
  registerserverhandler(MODNAM,"J",ch_dojoin);
  if (uplinkup==1) { 
    donej=1;
    ch_readdb();
    ch_joinall(); 
  }

  registerinternalevent(MODNAM,"AC",ch_ieac);
  registerinternalevent(MODNAM,"EB",ch_ieeb);
  registerinternalevent(MODNAM,"K SELF",ch_iek);
  registerinternalevent(MODNAM,"1MIN",ch_ieonemin);
  registerinternalevent(MODNAM,"60MIN",ch_iesixtymin);
  registercommand2(MODNAM,"op", ch_op, 0, 0, "op\tGives you channel operator status");
  registercommand2(MODNAM,"voice", ch_voice, 0, 0,"voice\tGives you voice");
  registercommand2(MODNAM,"invite", ch_invite, 0, 0,"invite\tInvites you into a channel");
  registercommand2(MODNAM,"topic", ch_topic, 0, 0,"topic\tSets the Topic on your channel");
  registercommand2(MODNAM,"kick", ch_kick, 0, 0, "kick\tKicks a user from your channel");
  registercommand2(MODNAM,"chanmode", ch_chanmode, 0, 0, "chanmode\tSet channel modes");
  registercommand2(MODNAM,"unbanall", ch_unbanall, 0, 0, "unbanall\tRemoves all ban on your channel");
  registercommand2(MODNAM,"deopall", ch_deopall, 0, 0, "deopall\tDeop everyone on the channl");
  registercommand2(MODNAM,"clearmodes", ch_clearchan, 0, 0, "clearmodes\tRemoves all modes from your channel");
  registercommand2(MODNAM,"recover", ch_recover, 0, 0, "recover\tDo unbannall, deopall and clearmodes");
  registercommand2(MODNAM,"access", ch_access, 0, 0, "access\tManage users on a channel");
  registercommand2(MODNAM,"chanflag", ch_chanflags, 0, 0, "chanflag\tSet X specific channel flags");
  registercommand2(MODNAM,"chanset", ch_set, 0, 0, "chanset\tSet parameters for chanflags");
  registercommand2(MODNAM,"chaninfo", ch_chaninfo, 0, 0, "chaninfo\tShows or sets information about a channel");
  registercommand2(MODNAM,"addchan", ch_add, 1, 700, "addchan\tAdd a new channel.");
  registercommand2(MODNAM,"delchan", ch_del, 1, 700, "delchan\tRemove X from a channel");
  registercommand2(MODNAM,"suspendchan", ch_suspend, 1, 900, "suspendchan\tSuspend a channel");
  registercommand2(MODNAM,"unsuspendchan", ch_unsuspend, 1, 900, "unsuspendchan\tUnsuspend a channel");
  registercommand2(MODNAM,"suspendlist", ch_suspendlist, 1, 100, "suspendlist\tShow suspended channels");
  registercommand2(MODNAM,"createdb_chan", ch_createdb, 1, 999, "createdb_chan\tCreates the Channel database.");
  registercommand2(MODNAM,"savedb_chan", ch_savedb, 1, 700, "savedb_chan\tSaves the Channel database.");
}

void chan_cleanup() {
  ch_partall();
  deregisterserverhandler2("M",MODNAM);
  deregisterserverhandler2("J",MODNAM);
  deregisterinternalevent("AC",MODNAM);
  deregisterinternalevent("EB",MODNAM);
  deregisterinternalevent("K SELF",MODNAM);
  deregisterinternalevent("1MIN",MODNAM);
  deregisterinternalevent("60MIN",MODNAM);
  deregistercommand("op");
  deregistercommand("voice");
  deregistercommand("invite");
  deregistercommand("topic");
  deregistercommand("kick");
  deregistercommand("chanmode");
  deregistercommand("unbanall");
  deregistercommand("deopall");
  deregistercommand("clearmodes");
  deregistercommand("recover");
  deregistercommand("access");
  deregistercommand("chanflag");
  deregistercommand("chaninfo");
  deregistercommand("chanset");
  deregistercommand("addchan");
  deregistercommand("delchan");
  deregistercommand("suspendchan");
  deregistercommand("unsuspendchan");
  deregistercommand("suspendlist");
  deregistercommand("createdb_chan");
  deregistercommand("savedb_chan");
}
