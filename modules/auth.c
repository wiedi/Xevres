/*
xevres auth module
based on QmOd by EZ_Target.
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

#define MODNAM "auth"

void doxauth(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  userdata *reqrec;
  int res2;
  MYSQL_RES *myres; MYSQL_ROW myrow;

  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  
  if (res2 != 3) {
    msgtouser(unum,"auth usage:");
    msgtouser(unum,"/msg X auth <nick> <password>");
    return;
  }

  sprintf(tmps2,"SELECT * from Xusers where username ='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  if (res2!=0) {
    putlog("!!! Could not read X user database !!!");
    msgtouser(unum,"An error occured.");
    return;
  }
  myres=mysql_store_result(&sqlcon);
 
  while ((myrow=mysql_fetch_row(myres))) {
    if (strcmp(myrow[2],"0")==0) {
     msgtouser(unum,"Your account has been suspended!");
     return;
    } 
    if (strcmp(myrow[1],tmps4)==0) {
     userdata *up;
     up=getudptr(unum);
     if (!ischarinstr('r',up->umode)) {
      long nettime;
      nettime=getnettime();
      reqrec=getudptr(unum);
      longtotoken(reqrec->numeric,tmps5,5);
      fprintf(sockout,"%s AC %s %s\r\n",servernumeric,tmps5,tmps3);
      dointernalevents("AC","%s %s",tmps5,tmps3);
      sprintf(tmps2,"UPDATE Xusers SET lastauthed=%ld WHERE username='%s'",nettime,tmps3);
      res2=mysql_query(&sqlcon,tmps2);
      strcpy(up->authname,tmps3);
      strcat(up->umode,"r");
      changemd5(up);
      msgtouser(unum,"Done.");
     } else {
      msgtouser(unum,"You are already authed.");
     }
    } else {
     msgtouser(unum,"Incorrect password");
    }
  }
  mysql_free_result(myres);
}

void doxhello(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE];
  userdata *reqrec;
  int res2;
  long nettime;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  reqrec=getudptr(unum);
  mysql_escape_string(tmps5,reqrec->nick,strlen(reqrec->nick));
  sprintf(tmps2,"SELECT count(*) from Xusers where username='%s'",tmps5);
  res2=mysql_query(&sqlcon,tmps2);
  myres=mysql_store_result(&sqlcon);
  myrow=mysql_fetch_row(myres);
  mysql_free_result(myres);
  if (strcmp(myrow[0],"0")==0) {
    userdata *up;
    up=getudptr(unum);
    if (!ischarinstr('r',up->umode)) {
     createrandompw(tmps4,8);
     nettime=getnettime();
     sprintf(tmps3,"INSERT INTO Xusers (username, password, authlevel, lastauthed) VALUES ('%s','%s',1,%ld)",tmps5,tmps4,nettime);
     res2=mysql_query(&sqlcon,tmps3);
     msgtouser(unum,"Hi, welcome in our Network! Your account has been created");
     sprintf(tmps3,"Your username is %s",tmps5);
     msgtouser(unum,tmps3);
     sprintf(tmps3,"Your password is %s",tmps4);
     msgtouser(unum,tmps3);
     msgtouser(unum,"You should change your password as soon as possible");
     msgtouser(unum,"Use: /msg X auth <nick> <password> to authenticate");
    } else {
     msgtouser(unum,"You are already authed.");
    }
  } else {
    msgtouser(unum,"Sorry, your name already exists");
  }
}

void doxpasswd(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE], tmps5[TMPSSIZE], tmps6[TMPSSIZE];
  userdata *reqrec;
  int res2;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  reqrec=getudptr(unum);
  res2=sscanf(tail,"%s %s %s %s %s",tmps2,tmps3,tmps4,tmps5,tmps6);
  if (res2 != 5) {
    msgtouser(unum,"chpasswd usage:");
    msgtouser(unum,"/msg X chpassword <authnick> <oldpassword> <newpassword> <newpassword>");
    return;
  }
  if (strcmp(tmps5,tmps6)!=0) {
    msgtouser(unum,"You should type your new password two times the same. ;-)");
    return;
  }
  sprintf(tmps2,"SELECT count(*) from Xusers where username ='%s' and password='%s'",tmps3,tmps4);
  res2=mysql_query(&sqlcon,tmps2);
  myres=mysql_store_result(&sqlcon);
  myrow=mysql_fetch_row(myres);
  mysql_free_result(myres);
  if (strcmp(myrow[0],"0")==0) {
    msgtouser(unum,"Your authname and/or password don't match our database.");
    return;
  }
  sprintf(tmps2,"UPDATE Xusers SET password='%s' WHERE username='%s'",tmps5,tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  msgtouser(unum,"Well Done.");
}

void docreatexdb(long unum, char *tail) {
  if (!checkauthlevel(unum,999)) { return; }
  msgtouser(unum,"Attempting to make the X user database");
  mysql_query(&sqlcon,"CREATE TABLE Xusers (username varchar(40) NOT NULL, password varchar(40) NOT NULL, authlevel int(11) DEFAULT '0' NOT NULL, lastauthed bigint, PRIMARY KEY (username), UNIQUE username (username))");
  msgtouser(unum,"Yeah Done");
}

void dogetxpasswd(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
/*  userdata *reqrec; */
  int res2;
  MYSQL_RES *myres; MYSQL_ROW myrow;

  if (!checkauthlevel(unum,900)) { return; }
  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);

  if (res2 != 2) {
    msgtouser(unum,"getpass usage:");
    msgtouser(unum,"/msg X getpass <nick>");
    return;
  }

  sprintf(tmps2,"SELECT * from Xusers where username ='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  if (res2!=0) {
    putlog("!!! Could not read X user database !!!");
    msgtouser(unum,"An error occured.");
    return;
  }
  myres=mysql_store_result(&sqlcon);
  while ((myrow=mysql_fetch_row(myres))) {
    sprintf(tmps2,"Password is currently: %s\n",myrow[1]);
    msgtouser(unum,tmps2);
    return;
  }

}

void doauthsuspend(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  userdata *reqrec;
  int res2;
  if (!checkauthlevel(unum,800)) { return; }
  MYSQL_RES *myres; MYSQL_ROW myrow;
  reqrec=getudptr(unum);
  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res2 != 2) {
    msgtouser(unum,"authsuspend usage:");
    msgtouser(unum,"/msg X authsuspend <authname>");
    return;
  }
  /* Set authlevel to 0 */
  sprintf(tmps2,"SELECT count(*) from Xusers where username ='%s' and authlevel='1'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  myres=mysql_store_result(&sqlcon);
  myrow=mysql_fetch_row(myres);
  mysql_free_result(myres);
  if (strcmp(myrow[0],"0")==0) {
    msgtouser(unum,"Your authname dosn't match our database or is suspended already.");
    return;
  }
  sprintf(tmps2,"UPDATE Xusers SET authlevel='0' WHERE username='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  /* Disconnect all User authed with tmps4 */
  int hcount;
  int kcount;
  userdata *udata;
  char target[10];
  kcount=0;
  for (hcount = 0; hcount < SIZEOFUL; hcount++) {
   udata = uls[hcount];
   while (udata != NULL) {
    if ((strcmp(udata->authname, tmps4)) && (ischarinstr('r', udata->umode) == 1)) {
     longtotoken(udata->numeric, target, 5);
     fprintf(sockout, "%s D %s :%s Your auth has been suspended. Cyaaa\r\n", servernumeric, target, myname);
     kcount++;
    }
    udata = (void *)udata->next;
   } 
  }
  newmsgtouser(unum,"Suspend hit %i online users. Well Done.",kcount);
}
void doauthunsuspend(long unum, char *tail) {
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE], tmps4[TMPSSIZE];
  userdata *reqrec;
  int res2;
  if (!checkauthlevel(unum,800)) { return; }
  MYSQL_RES *myres; MYSQL_ROW myrow;
  reqrec=getudptr(unum);
  res2=sscanf(tail,"%s %s %s",tmps2,tmps3,tmps4);
  if (res2 != 2) {
    msgtouser(unum,"authunsuspend usage:");
    msgtouser(unum,"/msg X authunsuspend <authname>");
    return;
  }
  /* Set authlevel to 0 */
  sprintf(tmps2,"SELECT count(*) from Xusers where username ='%s' and authlevel='0'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  myres=mysql_store_result(&sqlcon);
  myrow=mysql_fetch_row(myres);
  mysql_free_result(myres);
  if (strcmp(myrow[0],"0")==0) {
    msgtouser(unum,"Your authname dosn't match our database or is no suspended.");
    return;
  }
  sprintf(tmps2,"UPDATE Xusers SET authlevel='1' WHERE username='%s'",tmps3);
  res2=mysql_query(&sqlcon,tmps2);
  msgtouser(unum,"Well Done.");
}

void auth_init() {
  setmoduledesc(MODNAM,"Xevres Auth module");
  registercommand(MODNAM,"auth", doxauth, 0,"auth\tAuth's you with X");
  registercommand(MODNAM,"register", doxhello, 0,"register\tRegister a new Account");
  registercommand(MODNAM,"chpasswd", doxpasswd, 0, "chpasswd\tAlters your X password");
  registercommand(MODNAM,"getpass", dogetxpasswd, 1, "getpass\tfor use with the Relay bot");
  registercommand(MODNAM,"authsuspend", doauthsuspend, 1, "authsuspend\tSuspend a users auth");
  registercommand(MODNAM,"authunsuspend", doauthunsuspend, 1, "authunsuspend\tUnsuspend a users auth");
  registercommand(MODNAM,"createdb_auth", docreatexdb, 1, "createdb_auth\tCreates the database which holds X useinfo");
}

void auth_cleanup() {
  deregistercommand("auth");
  deregistercommand("register");
  deregistercommand("chpasswd");
  deregistercommand("getpass");
  deregistercommand("authsuspend");
  deregistercommand("authunsuspend");
  deregistercommand("createdb_auth");
}

