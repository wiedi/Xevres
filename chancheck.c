/* chancheck.c: the guts of chanfix */

#include <stdio.h>
#include "globals.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* Internal functions */
int geteligiblecount(channel *c);

/*
 * checkallchans: scan all channels on the network
 *                score all users as appropriate
 */
void checkallchans() {
  static int i=0,j;
  channel *c;
  /* Check we don't have a major netsplit */  
  if (splitservers.cursi>CFMAXSPLITSERVERS) { return; }
  /* Main loop - check the next CFSCANBUCKETS  */
  for (j=0;j<CFSCANBUCKETS;j++,i=((i+1)%SIZEOFCL)) {
    for (c=cls[i];c;c=(void *)c->next) {
      if (c->users.cursi>=CFMINUSERS) {
        checkchan(c);
      }
    }
  }
}

/*
 * checkchan: check an individual channel 
 */
void checkchan(channel *c) {
  int i,j;
  userdata *up;
  time_t now;
  chanuser *cu;
  regop *ro;
  int matched;
  int opcount=0;
  regop tempro;
  /* Check we have a regchannel structure */
  if (c->reg==NULL) {
    c->reg=createregchannel(c->name);
  }
  cu=(chanuser *)(c->users.content);
  ro=(regop *)(c->reg->regops.content);
  now=time(NULL);
  c->reg->lastexamined=now;
  for(i=0;i<c->users.cursi;i++) {
    if (cu[i].flags & um_o) {
      /* channel op */
      up=getudptr(cu[i].numeric);
      if (!up)
        /* erk.. non existent user */
        continue;
      opcount++;
      for(j=0,matched=0;j<c->reg->regops.cursi;j++) {
        if (!memcmp(ro[j].md5,up->md5,16)) {
          /* if the lastopped value==now, it's a clone */
          if (ro[j].lastopped<now) {
            /* Matched the user; increase count */
            ro[j].score++;
            ro[j].lastopped=now;
          }
          matched=1;
          break;
        }
      }  
      if (!matched) {
        /* CARE: need to update ro pointer after calling array_getfreeslot */
        j=array_getfreeslot(&(c->reg->regops));
        ro=(regop *)(c->reg->regops.content);
        ro[j].score=1;
        ro[j].lastopped=now;
        memcpy(ro[j].md5,up->md5,16);
      }
    }
  }
  /* Sort/tidy array of regops */
  /* This sort should be very fast if the array is already in order */
  for (j=0;j<c->reg->regops.cursi;j++) {
    /* New expiry logic: each point lasts a certain amount of time,
     * so an op with one point can be deopped for 1 hour before the
     * point is lost, an op with two points lasts 2 hours etc.
     * This should weed out users on dynamic IPs who don't auth
     * pretty quickly.
     *
     * There is also an overall cap as before.
     */
    if ((ro[j].lastopped < (now-CFREMEMBEROPSMAX)) ||
        (ro[j].lastopped < (now-(CFREMEMBERPERPOINT*ro[j].score)))) {
      /* It has expired.. delete it and continue with the next one
       * CARE: Need to reset the ro pointer after doing this 
       * in case it got moved in memory */
      array_delslot(&(c->reg->regops),j);
      ro=(regop *)(c->reg->regops.content);
      j--;
      continue;
    }          
    if (j>0) { /* Don't "sort" first element.. */
      /* Count backwards until we find an op with
       * larger (or equal) score... */
      for (i=j;i>0;i--) {
        if (ro[i-1].score >= ro[j].score) {
          break;
        }
      }
      if (i<j) {
        /* Save the item we're moving */
        memcpy(&tempro,&(ro[j]),sizeof(regop));
        /* Move the others down one to make room */
        memmove(&(ro[i+1]),&(ro[i]),sizeof(regop)*(j-i));      
        /* Copy the item to move into place */
        memcpy(&(ro[i]),&tempro,sizeof(regop));
      }
    }
  }
  if ((opcount==0) && (splitservers.cursi==0)) {
    doreop(c);
  }
}

/*
 * doreop: reop a user in a channel
 */
int doreop(channel *c) {
  int i,j;
  int numtotest;
  regop *rop;
  chanuser *cu;
  userdata *udp;
  int opped=0;
  int opcount=0;
  char usertoken[6];
  char numlist[40];
  if (c->reg==NULL) { return REOP_NOINFO; }
  /* 
   * Work out how many of the "regular ops" list are eligible for reopping
   *
   * Note that the array is held sorted, so we just check the first n entries
   * in the array.
   */
  numtotest=geteligiblecount(c);    
  if (numtotest<1) { return REOP_NOREGOPS; }
  rop=(regop *)(c->reg->regops.content);
  cu=(chanuser *)(c->users.content);
  if (rop[0].score < CFMINSCORE) {
    /* "top" user doesn't have enough points: no eligible ops */
    return REOP_NOREGOPS;
  }
  for (i=0;i<numtotest;i++) {
    /* Check they have enough points */
    if (rop[i].score >= CFMINSCORE) {
      /* We have an eligible regop.  Look for any users on 
       * the channel who match */
      for (j=0;j<c->users.cursi;j++) {
        udp=getudptr(cu[j].numeric);
        if (udp && !memcmp(udp->md5,rop[i].md5,16)) {
          /* Match */
          putlog("[ReOp] Reopped user %s on %s",udp->nick,c->name);
          longtotoken(udp->numeric,usertoken,5);
          opped=1;
          if (opcount==0) {
            strcpy(numlist,usertoken);
            strcat(numlist," ");
            opcount=1;
          } else {
            strcat(numlist,usertoken);
            if (opcount==5) {
              sendtouplink("%s M %s +oooooo %s\r\n",servernumeric,c->name,numlist);
              opcount=0;
            } else {
              strcat(numlist," ");
              opcount++;
            }
          }
          cu[j].flags |= um_o;
          /* NO break here: if there are eligible clones op them all */
        }
      }
    }
  }
  if (opped==1) {
    if (opcount>0) {
      char oplist[7];
      /* Outstanding ops */
      oplist[0]='\0';
      for (i=0;i<opcount;i++) { strcat(oplist,"o"); }
      sendtouplink("%s M %s +%s %s\r\n",servernumeric,c->name,oplist,numlist);
    }
    fflush(sockout);  
    return REOP_OPPED;
  } else {
    /* There was no match, so clear any protection modes */
    putlog("[ReOp] No eligible ops, clearing modes on %s",c->name);
    if (c->canhavebans>0) {
      sendtouplink("%s M %s +b-b *!*@* *!*@*\r\n",servernumeric,c->name);
      c->canhavebans=0;
    }
    if ((isflagset(c->flags,cm_i)) || (isflagset(c->flags,cm_l)) || (isflagset(c->flags,cm_k))) {
      sendtouplink("%s M %s -ikl *\r\n",servernumeric,c->name);
      delchanfla2(c,cm_i); delchanfla2(c,cm_l); delchanfla2(c,cm_k);
    }
    fflush(sockout);
    return REOP_NOMATCH; 
  }
} 

/*
 * showchan: display channel info to user 
 * 
 * The "all" parameter selects all regops, not 
 * just "interesting" ones.
 *
 * "interesting" == people eligible for reop 
 *                  + people who are opped now
 */
void showchan(long unum, channel *c, int all) {
  regop *ro; int i,j; int k=0; int eligible;
  char buf[TMPSSIZE];
  userdata *udp=NULL;
  chanuser *cu;
  if (c->reg==NULL || c->reg->regops.cursi==0) {
    sprintf(buf,"Channel %s has no registered ops.",c->name);
    msgtouser(unum,buf);
    return;
  }
  eligible=geteligiblecount(c);
  ro=(regop *)(c->reg->regops.content);
  cu=(chanuser *)(c->users.content);
  msgtouser(unum,"Pos Score User/Last seen");
  for(i=0;i<c->reg->regops.cursi;i++) {
    /* See if the user is on the channel atm (the hard way :( ) */
    for (j=0;j<c->users.cursi;j++) {
      udp=getudptr(cu[j].numeric);
      if (udp && !memcmp(udp->md5,ro[i].md5,16)) {
        /* Hit */
        break;
      }
    }
    if (all || (i<eligible) || (j<c->users.cursi && (cu[j].flags&um_o))) {
      if (j<c->users.cursi) {
        sprintf(buf,"%3d %5d %c%-15s %s@%s %s",i+1,ro[i].score,(cu[j].flags&um_o)?'@':' ',udp->nick,
          udp->ident,udp->host,(udp->authname[0])?udp->authname:"");
      } else {
        sprintf(buf,"%3d %5d Last seen: %s",i+1,ro[i].score,ctime(&(ro[i].lastopped)));
      }
      msgtouser(unum,buf);
      k++;
    }
  }
  sprintf(buf,"--- End of list - %d listed ---",k);
  msgtouser(unum,buf);
} 

int geteligiblecount(channel *c) {
  int numtotest;
  if (c==NULL || c->reg==NULL) { return -1; }
  /* Use the percentage.. */
  numtotest=(c->reg->regops.cursi*CFCANDIDATEPERCENT)/100;
  /* But make sure we have at least a certain number.. */
  if (numtotest<CFMINCANDIDATES) { numtotest=CFMINCANDIDATES; }
  /* But make sure we have that many ops to start with :) */
  if (numtotest>c->reg->regops.cursi) { numtotest=c->reg->regops.cursi; }
  return numtotest;
}
 
/*
 * showchansummary: just lists the scores for 
 * "best ops" and "current ops".
 * 
 * Quick two-line indication of whether a takeover just
 * happened or not.
 */
void showchansummary(long unum, channel *c) {
  regop *ro;
  int i,j;
  int eligible;
  char buf[512];
  userdata *udp;
  chanuser *cu;
  char tmps2[TMPSSIZE],tmps3[TMPSSIZE],tmps4[TMPSSIZE];
  
  if (c->reg==NULL || c->reg->regops.cursi==0) {
    sprintf(buf,"Channel %s has no registered ops.",c->name);
    msgtouser(unum,buf);
    return;
  }
  
  strcpy(tmps2,""); strcpy(tmps3,"");  
  eligible=geteligiblecount(c);
  
  ro=(regop *)(c->reg->regops.content);
  cu=(chanuser *)(c->users.content);
  
  for (i=0;i<c->reg->regops.cursi;i++) {
    /* Add to "top op" list if necessary */
    if (i<eligible && strlen(tmps2)<100 && (ro[i].score>=CFMINSCORE)) {
      sprintf(tmps4,"%d ",ro[i].score);
      strcat(tmps2,tmps4);
      if (strlen(tmps2)>=100)
        strcat(tmps2,"...");
    }  
    /* See if the user is on the channel atm (the hard way :( ) */
    for (j=0;j<c->users.cursi;j++) {
      udp=getudptr(cu[j].numeric);
      if (udp && !memcmp(udp->md5,ro[i].md5,16)) {
        /* Hit */
        break;
      }
    }
    /* Add to "current op" list if necessary */
    if (j<c->users.cursi && (cu[j].flags&um_o) && strlen(tmps3)<100) {
      sprintf(tmps4,"%d ",ro[i].score);
      strcat(tmps3,tmps4);
      if (strlen(tmps3)>=100) 
        strcat(tmps3,"...");
    }
  }
  sprintf(tmps4,"Scores of \"best ops\" on %s are  : %s",c->name,tmps2);
  msgtouser(unum,tmps4);
  sprintf(tmps4,"Scores of current ops on %s are : %s",c->name,tmps3);
  msgtouser(unum,tmps4);
} 

/*
 * showregs: lists the nicks of the the users on the channel 
 * who are on the chan and would be eligible for reop 
 */ 
void showregs(long unum, channel *c) {
  regop *ro;
  int i,j;
  int eligible;
  char buf[512];
  userdata *udp;
  chanuser *cu;
  char tmps2[TMPSSIZE];
  int count=0;
  if (c->reg==NULL || c->reg->regops.cursi==0) {
    sprintf(buf,"Channel %s has no registered ops.",c->name);
    msgtouser(unum,buf);
    return;
  }
  eligible=geteligiblecount(c);
  ro=(regop *)(c->reg->regops.content);
  cu=(chanuser *)(c->users.content);
  for (i=0;i<eligible;i++) {
    if (ro[i].score < CFMINSCORE) {
      eligible=i;
      break;
    }
    /* See if the user is on the channel atm (the hard way :( ) */
    for (j=0;j<c->users.cursi;j++) {
      udp=getudptr(cu[j].numeric);
      if (udp && !memcmp(udp->md5,ro[i].md5,16)) {
        /* Hit */
        break;
      }
    }
    /* If we found that user, print the nick */
    if (j<c->users.cursi) {
      if (count==0) {
        sprintf(tmps2,"Nicks eligible for reop on %s:",c->name);
        msgtouser(unum,tmps2);
      }
      msgtouser(unum,udp->nick);
      count++;
    }
  }
  if (count==0) {
    sprintf(tmps2,"--- No users eligible for reop on %s (out of %d)",c->name,eligible);
  } else {
    sprintf(tmps2,"--- End of list: %d (of %d eligible ops) listed",count,eligible);
  }
  msgtouser(unum,tmps2);
} 
