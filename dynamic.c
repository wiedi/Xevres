/*
Operservice 2 - dynamic.c
Functions for loading/unloading/handling dynamic modules
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
#include <dlfcn.h>
#include "globals.h"

/* Loads and initializes a dynamic module. name = modulename to load, without
   path or filesuffix (like .so)
   Return codes:
   -1 : dlopen failed
    0 : module loaded successfully
    1 : module was already loaded
*/
int loadandinitmodule(char *name) {
  char tmps2[TMPSSIZE], fn[FILENAME_MAX+600]; void *actmod;
  char tmps3[TMPSSIZE]; void(*infu)(void); long i; loadedmod *lm;
  strcpy(tmps2,name);
  toLowerCase(tmps2);
  delchar(tmps2,'/');
  delchar(tmps2,'\\');
  delchar(tmps2,'.');
  tmps2[MODNAMELEN]='\0';
  if (isloaded(tmps2)==1) { return 1; }
  strcpy(fn,modpath);
  strcat(fn,tmps2);
  strcat(fn,modend);
//  putlog("Trying to load module: %s",fn);
  actmod=dlopen(fn,RTLD_NOW|RTLD_GLOBAL);
  if (actmod==NULL) {
    putlog("Loading module failed: %s",dlerror());
    return -1;
  }
  /* Now add that babe to our module list */
  i=array_getfreeslot(&modulelist);
  lm=(loadedmod *)modulelist.content;
  strcpy(lm[i].name,tmps2);
  lm[i].handle=actmod;
  strcpy(lm[i].shortdesc,"- no description -"); /* The module should set that
                                                   in the init function */
  strcpy(tmps3,tmps2); strcat(tmps3,"_init");
  infu=dlsym(actmod,tmps3);
  if (infu==NULL) {
    return 0;
  }
  infu();
  return 0;
}

/* isloaded(modulename)
   Returns: 0 if module is not loaded
            1 if module is loaded
*/
int isloaded(char *name) {
  long i; loadedmod *lm;
  lm=(loadedmod *)modulelist.content;
  for (i=0;i<modulelist.cursi;i++) {
    if (strcmp(lm[i].name,name)==0) {
      return 1;
    }
  }
  return 0;
}

/* removemodule(modulename)
   Tries to remove the module.
   Returncodes:
   <0 : some error (cannot currently happen)
    0 : All OK
    1 : Module was not loaded
*/
int removemodule(char *name) {
  char tmps2[TMPSSIZE]; int res; char tmps3[TMPSSIZE];
  void(*infu)(void); long i; loadedmod *lm;
  long j; dyncommands *dc;
  strcpy(tmps2,name);
  toLowerCase(tmps2);
  delchar(tmps2,'/');
  delchar(tmps2,'\\');
  delchar(tmps2,'.');
  tmps2[MODNAMELEN]='\0';
  if (isloaded(tmps2)!=1) { return 1; }
  lm=(loadedmod *)modulelist.content;
  for (i=0;i<modulelist.cursi;i++) {
    if (strcmp(lm[i].name,tmps2)==0) {
      strcpy(tmps3,tmps2);
      strcat(tmps3,"_cleanup");
      infu=dlsym(lm[i].handle,tmps3);
      if (infu!=NULL) { infu(); }
      for (j=0;j<commandlist.cursi;j++) {
        dc=(dyncommands *)commandlist.content;
        if (strcmp(dc[j].providedby,tmps2)==0) {
          putlog("WARNING: Potential buggy module %s: Did not remove command %s in cleanup!",tmps2,dc[j].name);
          array_delslot(&commandlist,j);
          j--;
        }
      }
      res=dlclose(lm[i].handle);
      array_delslot(&modulelist,i);
      return 0;
    }
  }
  return 1;
}

/* Sets the description for a module (shown on lsmod) */
void setmoduledesc(char *name, char *txt) {
  loadedmod *lm; long i;
  lm=(loadedmod *)modulelist.content;
  for (i=0;i<modulelist.cursi;i++) {
    if (strcmp(lm[i].name,name)==0) {
      strncpy(lm[i].shortdesc,txt,MODDESCLEN);
      lm[i].shortdesc[MODDESCLEN]='\0';
      return;
    }
  }
}

/* Registers a command provided by a module.
   mod - name of the module providing it
   name - name of the function
   func - pointer to the function (needs to be of type: void f(long unum, char *tail) )
   operonly - means that command can only be executed by ircops
   minlev - Minimum authlevel of the user calling the command
   hlptxt - pointer to some helptext
*/
void registercommand2(char *mod, char *name, void *func, int operonly,
                      int minlev, char *hlptxt) {
  long i; dyncommands *dc;
  i=array_getfreeslot(&commandlist);
  dc=(dyncommands *)commandlist.content;
  dc[i].operonly=operonly;
  dc[i].func=func;
  dc[i].minlev=minlev;
  mystrncpy(dc[i].providedby,mod,MODNAMELEN);
  mystrncpy(dc[i].name,name,COMMANDLEN);
  toLowerCase(dc[i].name);
  toLowerCase(dc[i].providedby);
  dc[i].hlptxt=hlptxt;
}

/* For compatibility with older versions of O that did not support the
   "minlev" parameter */
void registercommand(char *mod, char *name, void *func, int operonly, char *hlptxt) {
  registercommand2(mod,name,func,operonly,0,hlptxt);
}

/* Registers a serverhandler provided by a module (or the core, for simplicity)
   mod - name of the module providing it
   name - which command this thingy binds to
   func - pointer to the function (needs to be of type: void f(void) )
*/
void registerserverhandler(char *mod, char *name, void *func) {
  long i; aserverhandler *sh; unsigned int hasch;
  hasch=shlhash(name);
  i=array_getfreeslot(&serverhandlerlist[hasch]);
  sh=(aserverhandler *)serverhandlerlist[hasch].content;
  sh[i].func=func;
  mystrncpy(sh[i].providedby,mod,MODNAMELEN);
  toLowerCase(sh[i].providedby);
  mystrncpy(sh[i].name,name,COMMANDLEN);
}

/* Deregisters a command provided by a module.
   this HAS to be done before the corresponding module gets unloaded, else the
   next call to this function will generate a "nice" little crash!
*/
void deregistercommand(char *name) {
  long i; dyncommands *dc;
  for (i=0;i<commandlist.cursi;i++) {
    dc=(dyncommands *)commandlist.content;
    if (strcmp(name,dc[i].name)==0) {
      array_delslot(&commandlist,i);
      i--;
    }
  }
}

/* Deregisters a serverhandler */
void deregisterserverhandler(char *name) {
  long i; aserverhandler *sh; unsigned int hasch;
  hasch=shlhash(name);
  for (i=0;i<serverhandlerlist[hasch].cursi;i++) {
    sh=(aserverhandler *)serverhandlerlist[hasch].content;
    if (strcmp(name,sh[i].name)==0) {
      array_delslot(&serverhandlerlist[hasch], i);
      i--;
    }
  }
}

/* Runs through all loaded commands and executes it if it is the one we search
   for
*/
int dodyncmds(char *cmd, long unum, char *tail, int oper, int authlev) {
  long i; dyncommands *dc;
  dc=(dyncommands *)commandlist.content;
  if (authlev<0) { return 0; }
  for (i=0;i<commandlist.cursi;i++) {
    if (strcmp(cmd,dc[i].name)==0) {
      if (oper>=dc[i].operonly) {
        if (authlev<dc[i].minlev) {
          newmsgtouser(unum,"This command requires at least authlevel %d (your level: %d)",
            dc[i].minlev,authlev);
          return 1;
        }
        dc[i].func(unum,tail);
        return 1;
      }
    }
  }
  return 0;
}

/* Runs through all serverhandlers and executes those that registered for the
   command */
int doserverhandlers(char *cmd) {
  long i; aserverhandler *sh; int foundone=0; unsigned int hasch;
  hasch=shlhash(cmd);
  sh=(aserverhandler *)serverhandlerlist[hasch].content;
  for (i=0;i<serverhandlerlist[hasch].cursi;i++) {
    if (strcmp(cmd,sh[i].name)==0) {
      sh[i].func(); foundone=1;
    }
  }
  return foundone;
}

void printhelp(long unum, char *hlptxt) {
  char *a; int b; char tmps2[TMPSSIZE];
  if (hlptxt==NULL) { return; }
  a=hlptxt; b=0;
  while (*a!='\0') {
    if ((*a!='\t') && (*a!='\n') && (b<200)) {
      tmps2[b]=*a;
    } else {
      if (*a=='\t') {
        while (b<25) { tmps2[b]=' '; b++; }
        tmps2[b]=' ';
      }
      if (*a=='\n') {
        tmps2[b]='\0';
        msgtouser(unum,tmps2);
        b=-1;
      }
    }
    a++; b++;
  }
  if (b>0) {
    tmps2[b]='\0';
    msgtouser(unum,tmps2);
  }
}

void dyncmdhelp(long unum, int oper, int authlev) {
  long i; dyncommands *dc;
  dc=(dyncommands *)commandlist.content;
  for (i=0;i<commandlist.cursi;i++) {
    if ((oper==dc[i].operonly) && (authlev>=dc[i].minlev)) {
      printhelp(unum,dc[i].hlptxt);
    }
  }
}

void doloadmod(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,995)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: loadmod modulename");
    return;
  }
  res=loadandinitmodule(tmps3);
  if (res<0) {
    msgtouser(unum,"There was some error loading the module.");
  } else if (res==0) {
    msgtouser(unum,"Module was loaded successfully");
  } else if (res==1) {
    msgtouser(unum,"That module was already loaded, use reloadmod to reload it");
  } else {
    msgtouser(unum,"Unknown returncode from loadmod");
  }
}

void dounloadmod(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,995)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: unloadmod modulename");
    return;
  }
  res=removemodule(tmps3);
  if (res<0) {
    msgtouser(unum,"There was some error unloading the module.");
  } else if (res==0) {
    msgtouser(unum,"Module was unloaded successfully");
  } else if (res==1) {
    msgtouser(unum,"That module was not loaded");
  } else {
    msgtouser(unum,"Unknown returncode from loadmod");
  }
}

void dolsmod(long unum, char *tail) {
  long i; char tmps2[TMPSSIZE]; loadedmod *lm;
  if (!checkauthlevel(unum,900)) { return; }
  if (modulelist.cursi==0) {
    msgtouser(unum,"No modules loaded.");
    return;
  }
  lm=(loadedmod *)modulelist.content;
  for (i=0;i<modulelist.cursi;i++) {
    sprintf(tmps2,"%-30s %s",lm[i].name,lm[i].shortdesc);
    msgtouser(unum,tmps2);
  }
  msgtouser(unum,"--- End of list ---");
}

void doreloadmod(long unum, char *tail) {
  int res; char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  if (!checkauthlevel(unum,995)) { return; }
  res=sscanf(tail,"%s %s",tmps2,tmps3);
  if (res<2) {
    msgtouser(unum,"Syntax: unloadmod modulename");
    return;
  }
  res=removemodule(tmps3);
  if (res<0) {
    msgtouser(unum,"There was some error unloading the module.");
  } else if (res==0) {
    msgtouser(unum,"Module was unloaded successfully");
  } else if (res==1) {
    msgtouser(unum,"That module was not loaded");
  } else {
    msgtouser(unum,"Unknown returncode from loadmod");
  }
  res=loadandinitmodule(tmps3);
  if (res<0) {
    msgtouser(unum,"There was some error loading the module.");
  } else if (res==0) {
    msgtouser(unum,"Module was loaded successfully");
  } else if (res==1) {
    msgtouser(unum,"That module was already loaded (this should not happen - internal error?)");
  } else {
    msgtouser(unum,"Unknown returncode from loadmod");
  }
}
