/*
xevres channel module
coded by Wiedi

*/

#include <stdio.h>
#include <stdlib.h>
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

#define DEFCHANMODES "s"
#define DEFOWNERMODE "cao"

int donej=0;
int uplinkup;

flags tuflags[] = {
 { ' ',	9 },
 { 'g',	UF_GIVE },
 { 'v',	UF_VOICE },
 { 'a',	UF_AUTO },
 { 'o',	UF_OP },
 { 'i',	UF_INVITE },
 { 'l',	UF_LOG },
 { 'c',	UF_COOWNER },
 { 'n',	UF_OWNER }
};

void xdummy(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

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
 msgtouser(unum,"Done"); 
}

void ch_invite(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res; long ux;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res != 3 && res != 2) {
  msgtouser(unum,"invite usage:");
  newmsgtouser(unum,"/msg %s invite <#channel> [nick]",mynick);
  return;
 }
 
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasInvite(up) && !UHasVoice(up) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,200)) {
   msgtouser(unum,"You don't have the permission");
   return;
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
 
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,300)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 sendtouplink("%sAAB T %s :%s\r\n",servernumeric,tmps3,tmps4); 
 sim_topic(tmps3,tmps4);
 fflush(sockout);
 msgtouser(unum,"Done");
}

void ch_kick(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], uhost[TMPSSIZE];
 int res, i; unsigned long j; 
 channel *a; chanuser *b; userdata *c;
 cuser *up; rchan *cp;
 
 res=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4, tmps5);
 if (res < 3) {
   msgtouser(unum,"kick usage:");
   newmsgtouser(unum,"Operator:  /msg %s kick <#channel> <nick> [reason]",mynick);
   newmsgtouser(unum,"[Co]Owner: /msg %s kick <#channel> <nick>[!ident@host] [reason] (Note: you can use wildcards)",mynick);
   return;
 }
 
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 if (!(up=ch_getchanuser(cp,unum2auth(unum))) && !UHasOp(up) && !UHasCoowner(up) && !UHasOwner(up) && !checkauthlevel(unum,400)) {
   msgtouser(unum,"You don't have the permission");
   return;
 }
 toLowerCase(tmps3);
 toLowerCase(tmps4);
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
}

void ch_ban(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_unban(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_unbanall(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_deopall(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_clearchan(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_recover(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_chanflags(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_chaninfo(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_set(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_access(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], acc[NICKLEN+1];
 int res, t1, t2, a=0, d=0, i;
 cuser *up, *up2; rchan *cp;
 
 res=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);
 if ((res !=4) && (res !=2)) {
   msgtouser(unum,"access usage:");
   newmsgtouser(unum,"/msg %s access <#channel> [<nickname|#account> <+|-><flags>]",mynick);
   newmsgtouser(unum,"For more information do /msg %s help access",mynick);
   return;
 }
 
 if (!(cp=ch_getchan(tmps3))) {
   newmsgtouser(unum,"I am not on %s",tmps3);
   return;
 }
 
 if (!(up=ch_getchanuser(cp,unum2auth(unum)))) {
   newmsgtouser(unum,"You are not known on %s",tmps3);
   return;
 }
 
 if (res == 4) {
   if (UHasOp(up) || UHasCoowner(up) || UHasOwner(up) || checkauthlevel(unum,800)){
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
     if (tmps3[0] == '#') {
       /* account */
     } else {
       /* need to get account */
     }
     /* change flags */      
   }  
 } else { 
   /* show access levels on a chan */
   newmsgtouser(unum,"--- Accesslevels on %s ---",tmps3);   
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

void ch_chanmode(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
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
  if (uhacc(tmps4)!=0) {
   msgtouser(unum,"Owner has no account!");
   return;
  } 
  reqrec=getudptr(unum);
  if (!ch_getchan(tmps3)) {
   nettime=getnettime();
   channel *c;
   toLowerCase(tmps3);
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
   mystrncpy(cp->flag_welcome,"",TOPICLEN+1);
   mystrncpy(cp->flag_key,"",CHANKEYLEN+1);
   mystrncpy(cp->flag_fwchan,"",CHANNELLEN+1);
   mystrncpy(cp->info,"",MAXINFOLEN+1);
   cp->suspended_since=0;
   cp->suspended_until=0;
   mystrncpy(cp->suspended_by,"",NICKLEN+1);
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
  userdata *reqrec;
  int res2;
  reqrec=getudptr(unum);
  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res2 != 2) {
    msgtouser(unum,"delchan usage:");
    msgtouser(unum,"/msg X delchan <chan>");
    return;
  }
  sprintf(tmps2,"delete from Xchannels where xchan='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  sprintf(tmps2,"delete from Xacclevs where xchan='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  sendtouplink("%sAAB L %s\r\n",servernumeric,tmps3);
  sim_part(tmps3,(tokentolong(servernumeric)<<SRVSHIFT)+1);
  newmsgtouser(unum,"Left %s",tmps3);
}

void ch_suspend(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_unsuspend(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

/* Channel module Server Handlers */

void ch_domode() {
 int t1, t2;
 if (paramcount<4) { return; }
 if ((params[2][0]!='#') && (params[2][0]!='+') && (params[2][0]!='&')) {
  /* we don't give a fuck about usermodes */
 } else {
  /* this is interesting */
  channel *tmpcp; char *c;
  toLowerCase(params[2]);
  tmpcp=getchanptr(params[2]);
  if (tmpcp==NULL) { return; }
  t2=0;
  c=&params[3][0]; t1=4;
  for (;(*c!='\0');c++) {
   if (*c=='+') { t2=0; continue; }
   if (*c=='-') { t2=1; continue; }
   if (*c=='o') {
    if (paramcount<=t1) {
     return;
    } else {
     normnum(params[t1]);
     if (t2==1) {
      /* Here we need to handel a DEOP! 
      sendtouplink("%s P #xchannel :WHoa %s deopped %s\r\n",servernumeric,params[0],params[t1]);
      fflush(sockout);
      */
     } else {
      /* Here we need to handel a +OP!
      sendtouplink("%s P #xchannel :WHoa %s opped %s\r\n",servernumeric,params[0],params[t1]);
      fflush(sockout);
      */
     }
     t1++;
    }
    continue; 
   }
  }
 }
}

void ch_dojoin() {
 char auth[TMPSSIZE];
 long num; 
 if (paramcount<3) { return; }
 toLowerCase(params[2]);
 num=tokentolong(sender);
 strcpy(auth,unum2auth(num));
 if (uhaccoc(auth,params[2],'a')==0) {
  if (uhaccoc(auth,params[2],'o')==0) {
   sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,params[2],sender);
   sim_mode(params[2],"+o",tokentolong(sender));
  }
  if (uhaccoc(auth,params[2],'v')==0) {
   sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,params[2],sender);
   sim_mode(params[2],"+v",tokentolong(sender));
  }
 } 
 fflush(sockout);
 return;
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
  }
} 

void ch_partall() {
  sendtouplink("%sAAB J 0\r\n",servernumeric);
  sim_join("0",(tokentolong(servernumeric)<<SRVSHIFT)+1);
} 

/* User info Functions */

/* unum2auth */
char *unum2auth(long unum) {
 userdata *reqrec;
 reqrec=getudptr(unum);
 return reqrec->authname;
}

/* user has access on channel function (-; */
int uhaccoc (char *xuser, char *xchan, char flag) {
 char tmps[TMPSSIZE], uflags[TMPSSIZE];
 int res2;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 sprintf(tmps,"SELECT * from Xacclevs where xuser='%s' and xchan='%s'",xuser, xchan);
 res2=mysql_query(&sqlcon,tmps);
 if (res2!=0) {
  putlog("!!! Could not read X acclevs database !!!");
  return -1;
 }
 myres=mysql_store_result(&sqlcon);
 while ((myrow=mysql_fetch_row(myres))) {
  strcpy(uflags,myrow[3]);
  if (ischarinstr(flag,uflags)) {
   return 0;
  } else {
   return 1; 
  } 
 }
 return 1;
}

/* user has account?! */
int uhacc (char *xuser) {
 char tmps[TMPSSIZE];
 int res2;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 sprintf(tmps,"SELECT * from Xusers where username='%s'",xuser);
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

/* user is known on channel function*/
int uikoc (char *xuser, char *xchan) {
 char tmps[TMPSSIZE];
 int res2;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 sprintf(tmps,"SELECT * from Xacclevs where xuser='%s' and xchan='%s'",xuser, xchan);
 res2=mysql_query(&sqlcon,tmps);
 if (res2!=0) {
  putlog("!!! Could not read X acclevs database !!!");
  return -1;
 }
 myres=mysql_store_result(&sqlcon);
 while ((myrow=mysql_fetch_row(myres))) {
  return 0;
 }
 return 1;
}

/* Simulation Functions */

/* Internal Events */

void ch_ieac(char *xarg) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
 long unum; int res, um, i;
 userdata *ux; channel **cx;
 
 res=sscanf(xarg,"%s %s",tmps2,tmps3);
 unum=tokentolong(tmps2);
 ux=getudptr(unum);
 if (ux->chans.cursi>0) {
  for (i=0;i<(ux->chans.cursi);i++) {
   cx=(channel **)(ux->chans.content);
   if ((strlen(cx[i]->name)+strlen(tmps2))>400) {
    break;
   } else {
    if (uhaccoc(tmps3,cx[i]->name,'a')==0) {
     um=getchanmode2(cx[i],ux->numeric);
     if (um<0) {
      /* eh? */
     } else {
      if (isflagset(um,um_o)) { /* has op */ } else { 
       if (uhaccoc(tmps3,cx[i]->name,'o')==0) {
        sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,cx[i]->name,tmps2);
        sim_mode(cx[i]->name,"+o",tokentolong(tmps2));
       }	
      }
      if (isflagset(um,um_v)) { /* has voice */ } else { 
       if (uhaccoc(tmps3,cx[i]->name,'v')==0) {
        sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,cx[i]->name,tmps2);
        sim_mode(cx[i]->name,"+v",tokentolong(tmps2));
       }
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
  mystrncpy(cp->name,myrow[0],CHANNELLEN+1);
  mystrncpy(cp->creator,myrow[1],NICKLEN+1);
  cp->cdate=atol(myrow[2]);
  cp->cflags=atoi(myrow[3]);
  cp->cptr=getchanptr(cp->name);
  cp->flag_limit=atoi(myrow[4]);
  cp->flag_suspendlevel=atoi(myrow[5]);
  mystrncpy(cp->flag_welcome,myrow[6],TOPICLEN+1);
  mystrncpy(cp->flag_key,myrow[7],CHANKEYLEN+1);
  mystrncpy(cp->flag_fwchan,myrow[8],CHANNELLEN+1);
  mystrncpy(cp->info,myrow[9],MAXINFOLEN+1);
  cp->suspended_since=atol(myrow[10]);
  cp->suspended_until=atol(myrow[11]);
  mystrncpy(cp->suspended_by,myrow[12],NICKLEN+1);
  cp->lastused=atol(myrow[13]);
  /* user flags */
  sprintf(tmps,"SELECT * from access_db where xchan='%s'",cp->name);
  res=mysql_query(&sqlcon,tmps);
  if (res!=0) {
    putlog("!!! Could not read channel access database !!!");
    return;
  }
  myres2=mysql_store_result(&sqlcon);
  while ((myrow2=mysql_fetch_row(myres2))) {
    up=(cuser*)malloc(sizeof(cuser));
    mystrncpy(up->name,myrow2[1],NICKLEN+1);
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

void ch_savedb(void) {
  char tmps[TMPSSIZE];
  rchan *cp;
  cuser *up;
  int c=0,u=0;
  /* empty old db */
  mysql_query(&sqlcon,"TRUNCATE TABLE chan_db");
  mysql_query(&sqlcon,"TRUNCATE TABLE access_db");
  /* write new */
  for(cp=rchans;cp;cp=cp->next) {
    sprintf(tmps,"INSERT INTO chan_db (xchan, creator, cdate, cflags, flimit, fsuspendlvl, fwelcome, "
    		"fkey, ffwchan, info, susp_since, susp_until, susp_by) VALUES ('%s','%s',%ld,'%d','%d',"
		"'%d','%s','%s','%s','%ld','%ld','%s','%ld')",cp->name,cp->creator,cp->cdate,cp->cflags,
		cp->flag_limit,cp->flag_suspendlevel,cp->flag_welcome,cp->flag_key,cp->info,cp->suspended_since,
		cp->suspended_until,cp->suspended_by,cp->lastused);
putlog("%s",tmps);
    mysql_query(&sqlcon,tmps);
    for(up=cp->cusers;up;up=up->next) {
      sprintf(tmps,"INSERT INTO access_db (xuser, xchan, aflags) VALUES ('%s','%s','%i')",up->name,
      		cp->name,up->aflags);
      mysql_query(&sqlcon,tmps);
      u++;
    }
    c++;
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
	CHANNELLEN,MAXINFOLEN,NICKLEN);	
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
  return 0;
}     

/* Module Things */

void chan_init() {
  setmoduledesc(MODNAM,"Xevres Channel module");
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
  
  registercommand2(MODNAM,"op", ch_op, 0, 0, "op\tGives you channel operator status");
  registercommand2(MODNAM,"voice", ch_voice, 0, 0,"voice\tGives you voice");
  registercommand2(MODNAM,"invite", ch_invite, 0, 0,"invite\tInvites you into a channel");
  registercommand2(MODNAM,"topic", ch_topic, 0, 0,"topic\tSets the Topic on your channel");
  registercommand2(MODNAM,"kick", ch_kick, 0, 0, "kick\tKicks a user from your channel");
  registercommand2(MODNAM,"chanmode", ch_kick, 0, 0, "chanmode\tSet channel modes");
  registercommand2(MODNAM,"ban", ch_ban, 0, 0, "ban\tBans an user from your channel");
  registercommand2(MODNAM,"unban", ch_unban, 0, 0, "unban\tRemoves a ban on an user from your channel");
  registercommand2(MODNAM,"unbanall", ch_unbanall, 0, 0, "unbanall\tRemoves all ban on your channel");
  registercommand2(MODNAM,"deopall", ch_deopall, 0, 0, "deopall\tDeop everyone on the channl");
  registercommand2(MODNAM,"clearchan", ch_clearchan, 0, 0, "clearchan\tRemoves all modes from your channel");
  registercommand2(MODNAM,"recover", ch_recover, 0, 0, "recover\tDo unbannall, deopall and clearchan");
  registercommand2(MODNAM,"access", ch_access, 0, 0, "access\tManage users on a channel");
  registercommand2(MODNAM,"chanflags", ch_chanflags, 0, 0, "chanflags\tSet modes how X should act on a channel");
  registercommand2(MODNAM,"chaninfo", ch_chaninfo, 0, 0, "chaninfo\tShows or sets information about a channel");
  registercommand2(MODNAM,"chanset", ch_set, 0, 0, "chanset\tSet parameters for chanflags");
  registercommand2(MODNAM,"addchan", ch_add, 1, 700, "addchan\tAdd a new channel.");
  registercommand2(MODNAM,"delchan", ch_del, 1, 700, "delchan\tRemove X from a channel");
  registercommand2(MODNAM,"suspendchan", ch_suspend, 1, 900, "suspendchan\tSuspend a channel");
  registercommand2(MODNAM,"unsuspendchan", ch_unsuspend, 1, 900, "unsuspendchan\tUnsuspend a channel");
  registercommand2(MODNAM,"suspendlist", xdummy, 1, 500, "suspendlist\tShow suspended channels");
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
  deregistercommand("op");
  deregistercommand("voice");
  deregistercommand("invite");
  deregistercommand("topic");
  deregistercommand("kick");
  deregistercommand("chanmode");
  deregistercommand("ban");
  deregistercommand("unban");
  deregistercommand("unbanall");
  deregistercommand("deopall");
  deregistercommand("clearchan");
  deregistercommand("recover");
  deregistercommand("access");
  deregistercommand("chanflags");
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

