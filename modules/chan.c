/*
xevres channel module
coded by Wiedi

known bugs:
 o-> there are some commands that send userinput direct to the sql server! *security problem* 
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
#include "chan.h"
#define MODNAM "chan"

#define DEFCHANMODES "s"
#define DEFOWNERMODE "c"

int donej=0;
// int uplinkup;

void xdummy(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_op(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res2;
 long ux;
 
 res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res2 != 3 && res2 != 2) {
  msgtouser(unum,"op usage:");
  msgtouser(unum,"/msg X op <channel> [nick]");
  return;
 }
 if (uhaccoc(unum2auth(unum),tmps3,'o')==0 || uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,300)) {
  if (res2 == 2) {
   longtotoken(unum,tmps5,5);
  } else {
   ux=nicktonum(tmps4);
   if (ux<0) {
    newmsgtouser(unum,"User %s is not on the network.",tmps4);
    return;
   }
   longtotoken(ux,tmps5,5); 
  }
  sendtouplink("%sAAB M %s +o %s\r\n",servernumeric,tmps3,tmps5); 
  ch_simmode(tmps3,"+o",tokentolong(tmps5));
  fflush(sockout);
  msgtouser(unum,"Here is your op");
 }  
}

void ch_voice(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res2;
 long ux;
 
 res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res2 != 3 && res2 != 2) {
  msgtouser(unum,"voice usage:");
  msgtouser(unum,"/msg X voice <channel> [nick]");
  return;
 }
 if (uhaccoc(unum2auth(unum),tmps3,'v')==0 || uhaccoc(unum2auth(unum),tmps3,'o')==0 || uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,100)) {
  if (res2 == 2) {
   longtotoken(unum,tmps5,5);
  } else {
   if (uhaccoc(unum2auth(unum),tmps3,'o')!=0 && uhaccoc(unum2auth(unum),tmps3,'c')!=0 && !checkauthlevel(unum,300)) {
    msgtouser(unum,"You can only voice yourself!");
    return;
   } 
   ux=nicktonum(tmps4);
   if (ux<0) {
    newmsgtouser(unum,"User %s is not on the network.",tmps4);
    return;
   }
   longtotoken(ux,tmps5,5); 
  }
  sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,tmps3,tmps5); 
  ch_simmode(tmps3,"+v",tokentolong(tmps5));
  fflush(sockout);
  msgtouser(unum,"Here is your voice");
 }  
}

void ch_invite(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
 int res2;
 long ux;
 
 res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
 if (res2 != 3 && res2 != 2) {
  msgtouser(unum,"invite usage:");
  msgtouser(unum,"/msg X invite <channel> [nick]");
  return;
 }
 if (uhaccoc(unum2auth(unum),tmps3,'v')==0 || uhaccoc(unum2auth(unum),tmps3,'o')==0 || uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,300)) {
  if (res2 == 2) {
   numtonick(unum,tmps4);
  } else {
   if (uhaccoc(unum2auth(unum),tmps3,'o')!=0 && uhaccoc(unum2auth(unum),tmps3,'c')!=0 && !checkauthlevel(unum,300)) {
    msgtouser(unum,"You can only invite yourself!");
    return;
   } 
   ux=nicktonum(tmps4);
   if (ux<0) {
    newmsgtouser(unum,"User %s is not on the network.",tmps4);
    return;
   }
  }
  sendtouplink("%sAAB I %s :%s\r\n",servernumeric,tmps4,tmps3); 
  fflush(sockout);
  msgtouser(unum,"Please come in.");
 }  
}

void ch_topic(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
 int res2;
 res2=sscanf(tail,"%s %s %[^\n]",tmps2,tmps3,tmps4);
 if (res2 != 3) {
  msgtouser(unum,"topic usage:");
  msgtouser(unum,"/msg X topic <channel> <topic>");
  return;
 }
 if (uhaccoc(unum2auth(unum),tmps3,'v')==0 || uhaccoc(unum2auth(unum),tmps3,'o')==0 || uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,300)) {
  mysql_escape_string(tmps5,tmps4,strlen(tmps4));
  sprintf(tmps2,"UPDATE Xchannels SET topic='%s' WHERE xchan='%s'", tmps5, tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  sendtouplink("%sAAB T %s :%s\r\n",servernumeric,tmps3,tmps4); 
  ch_simtopic(tmps3,tmps4);
  fflush(sockout);
  msgtouser(unum,"Topic set");
 } else {
  msgtouser(unum,"Sorry, you don't have the permission on this chan.");
 } 
}

void ch_kick(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], uhost[TMPSSIZE];
 int res2, res, i;
 channel *a; chanuser *b; userdata *c;
 unsigned long j; 
 
 res2=sscanf(tail,"%s %s %s %[^\n]",tmps2,tmps3,tmps4, tmps5);
 if (res2 < 3) {
  msgtouser(unum,"kick usage:");
  msgtouser(unum,"Operator: /msg X kick <channel> <nick> [reason]");
  msgtouser(unum,"Creator:  /msg X kick <channel> <nick>[!ident@host] [reason] (Note: you can use wildcards)");
  return;
 }
 toLowerCase(tmps4);
 if (res2 == 3) { 
  numtonick(unum,tmps2);
  sprintf(tmps5,"requested by %s",tmps2); }
 if (uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,500)) {
  /* rock n roll */
  toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) { return; /* should never happen */ }
  b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; i=0;
  for (j=0;j<a->users.cursi;j++) {
   c=getudptr(b[j].numeric);
   if (ischarinstr('!',tmps4)) {
    /* nick!ident@host */
    sprintf(uhost,"%s!%s@%s",c->nick,c->ident,c->host);
    if (match2strings(tmps4,uhost)) {
     if (ischarinstr('k',c->umode) || ischarinstr('X',c->umode)) {
      /* do not much */
     } else {
      longtotoken(c->numeric,tmps2,5);
      sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
      ch_simpart(tmps3,c->numeric);
      i++;
     } 
    } 
   } else { 
    /* nick */
    numtonick(b[j].numeric,tmps2);
    if (match2strings(tmps4,tmps2)) {
     if (ischarinstr('k',c->umode) || ischarinstr('X',c->umode)) {
      /* do even less */
     } else {
      longtotoken(c->numeric,tmps2,5);
      sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
      ch_simpart(tmps3,c->numeric);
      i++;
     } 
    }
   } 
   res++;
  }
  newmsgtouser(unum,"Kicked %i users from %s",i,tmps3); 
 } else if  (uhaccoc(unum2auth(unum),tmps3,'o')==0 || checkauthlevel(unum,300)) {
  /* normal kick */
 toLowerCase(tmps3);
  a=getchanptr(tmps3);
  if (a==NULL) { return; /* should never happen */ }
  b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; i=0;
  for (j=0;j<a->users.cursi;j++) {
   c=getudptr(b[j].numeric);
   numtonick(b[j].numeric,tmps2);
   if (match2strings(tmps4,tmps2)) {
    if (ischarinstr('k',c->umode) || ischarinstr('X',c->umode)) {
     /* do even less */
    } else {
     longtotoken(c->numeric,tmps2,5);
     sendtouplink("%sAAB K %s %s :%s\r\n",servernumeric,tmps3,tmps2,tmps5); 
     ch_simpart(tmps3,c->numeric);
     msgtouser(unum,"Kicked away!");
     break;
    } 
   } 
  } 
  msgtouser(unum,"User not on channel.");
 } else {
  msgtouser(unum,"Sorry, you don't have the permission on this chan.");
 } 
}

void ch_ban(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_clear(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_recover(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_access(long unum, char *tail) {
 char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps7[TMPSSIZE], uflags[TMPSSIZE];
 int res2, t1, t2, xist;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 res2=sscanf(tail,"%s %s %s %s",tmps2,tmps3,tmps4,tmps5);

 if (res2 == 4) {
  if (uhaccoc(unum2auth(unum),tmps3,'c')==0 || checkauthlevel(unum,800)){
   if (uhacc(tmps4)==0) {
    /* Get access modes for user */
    sprintf(tmps2,"SELECT * from Xacclevs where xuser='%s' and xchan='%s'",tmps4, tmps3);
    res2=mysql_query(&sqlcon,tmps2);
    if (res2!=0) {
     putlog("!!! Could not read X acclevs database !!!");
     msgtouser(unum,"An error occured.");
     return;
    }
    myres=mysql_store_result(&sqlcon);
    xist=-1;
    strcpy(uflags,"");
    while ((myrow=mysql_fetch_row(myres))) {
     strcpy(uflags,myrow[3]);
     xist++;
    }
    strcpy(tmps7,uflags);
    /* parse new flags */  
    t1=0; t2=0;
    while (tmps5[t2]!='\0') {
     if (tmps5[t2]!='+' && tmps5[t2]!='-' && tmps5[t2]!='c' && tmps5[t2]!='o' && tmps5[t2]!='v' &&  tmps5[t2]!='b' && tmps5[t2]!='a'){
      msgtouser(unum,"Unknown accessflag!");
      return;
     }    
     if ((tmps5[t2]=='+') || (tmps5[t2]=='-')) {
      if (tmps5[t2]=='+') { t1=1; } else { t1=0; }
     } else {
      if (t1==1) {
       if (strlen(uflags)<(MAXUMODES-1)) {
        if (!ischarinstr(tmps5[t2],uflags)) { appchar(uflags,tmps5[t2]); }
       }
      } else {
       delchar(uflags,tmps5[t2]);
      }
     }
     t2++;
    }
    /* update flags to sql db */
    if (strcmp(uflags,"")==0) {
     if (strcmp(tmps7,"")!=0) {
      sprintf(tmps2,"delete from Xacclevs where xchan='%s' and xuser='%s'",tmps3, tmps4);
      res2=mysql_query(&sqlcon,tmps2);
      newmsgtouser(unum,"User %s removed from channel",tmps4);
      return;
     } else {
      msgtouser(unum,"User not on access list!");
      return;
     }  
    } else {
     if (strcmp(tmps7,"")!=0) {
      /* update */
      sprintf(tmps2,"UPDATE Xacclevs SET accesslevels='%s' WHERE xuser='%s' and xchan='%s'",uflags, tmps4, tmps3);
      res2=mysql_query(&sqlcon,tmps2);
      msgtouser(unum,"Accesslevel updated");
      return;
     } else {
      /* add new */
      sprintf(tmps2,"INSERT INTO Xacclevs (xuser, xchan, accesslevels) VALUES ('%s','%s','%s')",tmps4, tmps3, uflags);
      res2=mysql_query(&sqlcon,tmps2);
      msgtouser(unum,"User added to accesslist");
      return;
     }  
    } 
   } else {
    msgtouser(unum,"Unknown user account!");
   }
  }   
 } else if (res2 == 2) { 
  if (uikoc(unum2auth(unum),tmps3)==0 || checkauthlevel(unum,500)){
   /* show access levels on a chan */
   newmsgtouser(unum,"--- Accesslevels on %s ---",tmps3);   
   sprintf(tmps2,"SELECT * from Xacclevs where xchan ='%s'",tmps3);
   res2=mysql_query(&sqlcon,tmps2);
   if (res2!=0) {
    putlog("!!! Could not read X acclevs database !!!");
    msgtouser(unum,"An error occured.");
    return;
   }
   myres=mysql_store_result(&sqlcon);
   while ((myrow=mysql_fetch_row(myres))) {
    sprintf(tmps2,"»  %s\t+%s",myrow[1],myrow[3]);
    printhelp(unum,tmps2);
   }
   mysql_free_result(myres);
   msgtouser(unum,"--- End of List ---");
   return;
  } else {
   msgtouser(unum,"Sorry, you don't have permission to view the accesslevels here");
  } 
 } else {
  msgtouser(unum,"access usage:");
  msgtouser(unum,"/msg X access <channel> <userauth> [+|-]<flags>");
  msgtouser(unum,"Flags can be: c=creator o=operator v=voiced b=banned a=auto");
  return;
 }
}

void ch_chanmode(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_add(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  userdata *reqrec;
  int res2;
  long nettime;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  
  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res2 != 3) {
   msgtouser(unum,"addchan usage:");
   msgtouser(unum,"/msg X addchan <channel> <owner>");
   return;
  }
  if (uhacc(tmps4)!=0) {
   msgtouser(unum,"Owner has no account!");
   return;
  } 
  reqrec=getudptr(unum);
  sprintf(tmps2,"SELECT count(*) from Xchannels where xchan='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  myres=mysql_store_result(&sqlcon);
  myrow=mysql_fetch_row(myres);
  mysql_free_result(myres);
  if (strcmp(myrow[0],"0")==0) {
   nettime=getnettime();
   channel *c;
   c=getchanptr(tmps3);
   if (c==NULL) { 
    msgtouser(unum,"Sorry, that channel does not exist!");
    return;
   }
   sprintf(tmps5,"INSERT INTO Xchannels (xchan, owner, suspendlevel, cdate, topic, modes) VALUES ('%s','%s',0,%ld,'%s','%s')",tmps3,tmps4,nettime,c->topic,DEFCHANMODES);
   res2=mysql_query(&sqlcon,tmps5);
   sprintf(tmps5,"INSERT INTO Xacclevs (xuser, xchan,  accesslevels) VALUES ('%s','%s','%s')",tmps4,tmps3,DEFOWNERMODE);
   res2=mysql_query(&sqlcon,tmps5);
   sendtouplink("%sAAB J %s %ld\r\n",servernumeric,tmps3,nettime);
   ch_simjoin(tmps3,(tokentolong(servernumeric)<<SRVSHIFT)+1);
   sendtouplink("%s OM %s +o %sAAB\r\n",servernumeric,tmps3,servernumeric);  
   ch_simmode(tmps3,"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
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
  ch_simpart(tmps3,(tokentolong(servernumeric)<<SRVSHIFT)+1);
  newmsgtouser(unum,"Left %s",tmps3);
}

void ch_suspend(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_unsuspend(long unum, char *tail) {
  msgtouser(unum,"Sorry, this command is not finished yet!");
}

void ch_createdb(long unum, char *tail) {
  msgtouser(unum,"Attempting to make the X channel database");
  mysql_query(&sqlcon,"CREATE TABLE Xchannels (xchan varchar(200) NOT NULL, owner varchar(40) NOT NULL, suspendlevel int(11) DEFAULT '0' NOT NULL, cdate bigint, topic varchar(255), modes varchar(15), PRIMARY KEY (xchan), UNIQUE xchan (xchan))");
  mysql_query(&sqlcon,"CREATE TABLE Xacclevs (aid int(11) UNSIGNED NOT NULL AUTO_INCREMENT, xuser varchar(40) NOT NULL, xchan varchar(200) NOT NULL, accesslevels varchar(15), PRIMARY KEY (aid), UNIQUE aid (aid))");
  msgtouser(unum,"Yeah Done");
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
   ch_simmode(params[2],"+o",tokentolong(sender));
  }
  if (uhaccoc(auth,params[2],'v')==0) {
   sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,params[2],sender);
   ch_simmode(params[2],"+v",tokentolong(sender));
  }
 } 
 fflush(sockout);
 return;
}

/* Load X to the Channels */

void ch_joinall() {
 char tmps2[TMPSSIZE];
 int res2;
 long xtime;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 sprintf(tmps2,"SELECT * from Xchannels");
 res2=mysql_query(&sqlcon,tmps2);
 if (res2!=0) {
  putlog("!!! Could not read X channel database !!!");
  return;
 }
 myres=mysql_store_result(&sqlcon);
 xtime=getnettime();
 while ((myrow=mysql_fetch_row(myres))) {
  sendtouplink("%sAAB J %s %ld\r\n",servernumeric,myrow[0],xtime);
  ch_simjoin(myrow[0],(tokentolong(servernumeric)<<SRVSHIFT)+1);
  sendtouplink("%s OM %s +o %sAAB\r\n",servernumeric,myrow[0],servernumeric);
  ch_simmode(myrow[0],"+o",(tokentolong(servernumeric)<<SRVSHIFT)+1);
  sendtouplink("%sAAB T %s :%s\r\n",servernumeric,myrow[0],myrow[4]);
  ch_simtopic(myrow[0],myrow[4]);
 }
 mysql_free_result(myres);
} 

void ch_partall(char *xwhy) {
 char tmps2[TMPSSIZE];
 int res2;
 MYSQL_RES *myres; MYSQL_ROW myrow;
 
 sprintf(tmps2,"SELECT * from Xchannels");
 res2=mysql_query(&sqlcon,tmps2);
 if (res2!=0) {
  putlog("!!! Could not read X channel database !!!");
  return;
 }
 myres=mysql_store_result(&sqlcon);
 while ((myrow=mysql_fetch_row(myres))) {
  sendtouplink("%sAAB L %s :%s\r\n",servernumeric,myrow[0],xwhy);
  ch_simpart(myrow[0],(tokentolong(servernumeric)<<SRVSHIFT)+1);
 }
 mysql_free_result(myres);
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

void ch_simjoin(char *xchan, long num) {
 channel *ctmp;
 
 if (!chanexists(xchan)) {
  newchan(xchan,0);
 }
 addusertochan(xchan,num);
 ctmp=getchanptr(xchan);
 if (ctmp!=NULL) { addchantouser2(num,ctmp); }
} 

void ch_simpart(char *xchan, long num) {
 toLowerCase(xchan);
 delchanfromuser(num,xchan);
 deluserfromchan(xchan,num);
}

void ch_simtopic(char *xchan, char *topic) {
 channel *c;
 
 c=getchanptr(xchan);
 if (c==NULL) { return; }
 /* freeastring(c->topic); */
 strcpy(c->topic,topic);
}

void ch_simmode(char *xchan, char *mode, long num) {
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
        ch_simmode(cx[i]->name,"+o",tokentolong(tmps2));
       }	
      }
      if (isflagset(um,um_v)) { /* has voice */ } else { 
       if (uhaccoc(tmps3,cx[i]->name,'v')==0) {
        sendtouplink("%sAAB M %s +v %s\r\n",servernumeric,cx[i]->name,tmps2);
        ch_simmode(cx[i]->name,"+v",tokentolong(tmps2));
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
  ch_joinall(); 
 }
}

/* Module Things */

void chan_init() {
  setmoduledesc(MODNAM,"Xevres Channel module");
  registerserverhandler(MODNAM,"M",ch_domode);
  registerserverhandler(MODNAM,"J",ch_dojoin);
  if (uplinkup==1) { 
   donej=1;
   ch_joinall(); 
  } 
  registerinternalevent(MODNAM,"AC",ch_ieac);
  registerinternalevent(MODNAM,"EB",ch_ieeb);
  registercommand2(MODNAM,"op", ch_op, 0, 0, "op\tGives you channel operator status");
  registercommand2(MODNAM,"voice", ch_voice, 0, 0,"voice\tGives you voice");
  registercommand2(MODNAM,"invite", ch_invite, 0, 0,"invite\tInvites you into a channel");
  registercommand2(MODNAM,"topic", ch_topic, 0, 0,"topic\tSets the Topic on your channel");
  registercommand2(MODNAM,"kick", ch_kick, 0, 0, "kick\tKicks a user from you channel");
  registercommand2(MODNAM,"ban", ch_ban, 0, 0, "chanmode\tBans a user from you channel");
  registercommand2(MODNAM,"clear", ch_clear, 0, 0,"clear\tClears all channel modes");
  registercommand2(MODNAM,"recover", ch_recover, 0, 0, "recover\tClears all channel modes and deops everybody");
  registercommand2(MODNAM,"access", ch_access, 0, 0, "access\tManage users on a channel");
  registercommand2(MODNAM,"chanmode", ch_chanmode, 0, 0, "chanmode\tSet modes how X should act on a channel");
  registercommand2(MODNAM,"addchan", ch_add, 1, 700, "addchan\tAdd a new channel.");
  registercommand2(MODNAM,"delchan", ch_del, 1, 700, "delchan\tRemove X from a channel");
  registercommand2(MODNAM,"suspendchan", ch_suspend, 1, 900, "suspendchan\tSuspend a channel");
  registercommand2(MODNAM,"unsuspendchan", ch_unsuspend, 1, 900, "unsuspendchan\tUnsuspend a channel");
  registercommand2(MODNAM,"createdb_chan", ch_createdb, 1, 999, "createdb_chan\tCreates the Channel database.");

}

void chan_cleanup() {
  ch_partall("Channel Modul Unloaded");
  deregisterserverhandler2("M",MODNAM);
  deregisterserverhandler2("J",MODNAM);
  deregisterinternalevent("AC",MODNAM);
  deregisterinternalevent("EB",MODNAM);
  deregistercommand("op");
  deregistercommand("voice");
  deregistercommand("invite");
  deregistercommand("topic");
  deregistercommand("kick");
  deregistercommand("ban");
  deregistercommand("clear");
  deregistercommand("recover");
  deregistercommand("access");
  deregistercommand("chanmode");
  deregistercommand("addchan");
  deregistercommand("delchan");
  deregistercommand("suspendchan");
  deregistercommand("unsuspendchan");
  deregistercommand("createdb_chan");
}

