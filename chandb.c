/* update/maintain registered channel database */

/*
 * findregchannelifexists:
 *  Returns the pointer to the regchannel structure for the named channel
 *  if it already exists.  Used to set the regchannel * when new channels
 *  are created.
 *
 * ASSUMES THAT channel IS A VALIDATED CHANNEL NAME
 */

#include <stdio.h>
#include "globals.h"
#include <string.h>
#include <time.h>

#include <stdlib.h>
#ifndef WE_R_ON_BSD
#include <malloc.h>
#endif
#ifdef USE_DOUGLEA_MALLOC
#include "dlmalloc/malloc.h"
#endif

/* Local functions */
void savechan(regchannel *c, FILE *fp);

regchannel *findregchannelifexists(char *channel) {
  /* Find a registered channel if it exists */
  int i;
  unsigned long hash;
  regchannel **rc;
  
  hash=clhash(channel);
  rc=(regchannel **)(reg[hash].content);
  
  for(i=0;i<reg[hash].cursi;i++) {
    if (!strcmp(rc[i]->name,channel)) {
      /* Got it */
      return rc[i];
    }
  }  
  return NULL;
}      

/*
 * createregchannel:
 *  Returns the pointer to a new regchannel structure for the named channel.
 *  The pointer will also be registered in the global reg[] hash.
 *  
 * ASSUMES THAT channel IS A VALIDATED CHANNEL NAME
 */
regchannel *createregchannel(char *channel) {
  regchannel *rc;
  unsigned long hash;
  regchannel **rcp;
  int i;
  
  if ((rc=findregchannelifexists(channel))!=NULL) 
    /* It already exists; return it */
    return rc;
  
  /* Allocte new structure */
  if ((rc=(regchannel *)malloc(sizeof(regchannel)))==NULL) {
    putlog("!!! Couldn't allocate memory for regchan %s !!!",channel);
    exit(1);
  }
  
  /* Set up the structure. */
  rc->name=getastring(channel);
  array_init(&(rc->regops),sizeof(regop));
  array_setlim1(&(rc->regops),5);
  array_setlim2(&(rc->regops),10);
  rc->lastexamined=time(NULL);
  
  /* Insert new structure into hash */
  hash=clhash(channel);
  i=array_getfreeslot(&(reg[hash]));
  rcp=(regchannel **)(reg[hash].content);
  rcp[i]=rc;
    
  return rc;
}

/*
 * saveallchans:
 *  Saves all the channels in the database to disk. Also tidies up the list
 *  by deleting stale channels.
 */
void saveallchans() {
  int i;
  int j;
  regchannel **cl;
  channel *c;
  time_t now;
  FILE *fp;
  
  now=time(NULL);

  if ((fp=fopen(CFFILENAME,"w"))==NULL) {
    putlog("!!! Unable to open %s for writing !!!","chandb");
    return;
  }
  
  for(i=0;i<SIZEOFCL;i++) {
    cl=(regchannel **)(reg[i].content);
    for (j=0;j<reg[i].cursi;j++) {
      if (cl[j]->lastexamined < (now-CFREMEMBERCHAN)) {
        printf("Deleting expired channel %s\n",cl[j]->name);
        /* Expired channel.  Delete */
        c=getchanptr(cl[j]->name);
        if (c)
          c->reg=NULL;
        array_free(&(cl[j]->regops));
        freeastring(cl[j]->name);
        free(cl[j]);
        array_delslot(&(reg[i]),j);
        cl=(regchannel **)(reg[i].content);
        j--;
      } else {
        savechan(cl[j], fp);
      }
    }
  } 
  
  fclose(fp);   
}

/*
 * savechan: save a single channel to the file 
 */
void savechan(regchannel *c, FILE *fp) {
  char tmps2[TMPSSIZE];
  int i;
  regop *ro;
  unsigned long *lp;
  
  fprintf(fp,"%s %lu %ld\n",c->name,c->lastexamined,c->regops.cursi);
  
  ro=(regop *)(c->regops.content);
  
  for(i=0;i<c->regops.cursi;i++) {
    lp=(unsigned long *)ro[i].md5;
    sprintf(tmps2,"%08lx%08lx%08lx%08lx",lp[0],lp[1],lp[2],lp[3]);
    fprintf(fp,"%s %lu %d\n",tmps2,ro[i].lastopped,ro[i].score);
  }
}  

/*
 * loadallchans: loads in the channel database
 */
void loadallchans() {
  int i,j;
  regop *ro;
  unsigned long *ulp;
  FILE *fp;
  regchannel *rc;
  char buf[512];
  char channame[CHANNELLEN+1];
  time_t lastexamined;
  int numusers;
  
  if ((fp=fopen(CFFILENAME,"r"))==NULL) {
    putlog("!!! Unable to open %s for reading !!!",CFFILENAME);
    return;
  }
  
  while (!feof(fp)) {
    fgets(buf,512,fp);
    if (feof(fp))
      break;
      
    delchar(buf,'\n');
    delchar(buf,'\r');
    
    if (sscanf(buf,"%s %ld %d",channame,&lastexamined,&numusers)<3) {
      putlog("!!! Corrupt channel logfile %s !!!",buf);
      continue;
    }
    
    rc=createregchannel(channame);
    rc->lastexamined=lastexamined;
    
    for (i=0;i<numusers;i++) {
      fgets(buf,512,fp);
      if (feof(fp)) {
        putlog("!!! Unexpected EOF in channel logfile !!!");
        fclose(fp);
        return;
      }
      
      j=array_getfreeslot(&(rc->regops));
      ro=(regop *)(rc->regops.content);
      
      ulp=(unsigned long *)(ro[j].md5);
            
      if (sscanf(buf,"%08lx%08lx%08lx%08lx %lu %d",&ulp[0],&ulp[1],&ulp[2],&ulp[3],&(ro[j].lastopped),&(ro[j].score))<6) {
        putlog("!!! Corrupt channel logfile %s !!!",buf);
        continue;
      }
    }
  }        
}  
