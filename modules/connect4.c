/*
xevres 4wins module

  based on bewares connect4 bot and 4inarow 
  by Davide Corrado (also known as Mr. Dave konrad)
  
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
#define MODNAM "connect4"

/* Some options */

/* Mirc Color Code Char */
#define MC '\003' 
/* Player 1 Color */
#define P1C '7'
/* Player 2 Color */
#define P2C '3'
/* Background Color */
#define BC '1'
/* Empty Color */
#define EC '12'
/* Text Color */
#define TC '0'
/* Heighth */
#define PH 5
/* Width */
#define PW 7
/* Won */
#define WIN 4

typedef struct data
{
  int nmoves;
  int pieces_in_a_column[PW];
  long pl1;
  long pl2;
  char matrix[PH][PW];
} fourinarow;

fourinarow * gamez[SIZEOFCL+1];

/* here is code */

/* proto */
int getcid(char *chan);


void c4_help(long unum, char *tail) {
 msgffake(unum,"c","This module is not finished yet ;)");
}

void c4_chmsg(long unum, char *chan, char *tail) {
  /* cmsgffake(chan,"c","Debug: %s - %s", tail, chan);  */
  
  char tmps2[TMPSSIZE], tmps3[TMPSSIZE];
  channel *a; chanuser *b; userdata *c;
  long ux;
  int res2, res, cid;
  unsigned long j;
  toLowerCase(tail);
  cid=getcid(chan);
  if (cid==-1) {return; /* huh? */}
  if (tail[0] == '!') {
    res2=sscanf(tail,"%s %s",tmps2,tmps3);
    if (res2 == 1) {
      /* !1 - !7 */
      if (gamez[cid]->pl1==unum || gamez[cid]->pl2==unum) {
        /*play*/
	printf("hi");
      }	
    } else if (res2 == 2) {
      /* !play <player> */
      if (strcmp(tmps2,"!play")==0) {
        if (gamez[cid]->pl1!=1) {
          //msgffake(unum,"c","This module is not finished yet ;)");
	  printf("hi");
        }	
        return;
        /* get unum of <player> */
	a=getchanptr(chan);
        if (a==NULL) { return; /* should never happen */ }
	b=(chanuser *)a->users.content; strcpy(tmps2,""); res=0; ux=-1;
	for (j=0;j<a->users.cursi;j++) {
          c=getudptr(b[j].numeric);
          numtonick(b[j].numeric,tmps2);
          if (strcmp(tmps3,tmps2)==0) {
	    ux=c->numeric;
            break;
	  }  
        }
	if (ux==-1) {
	  cmsgffake(chan,"c","User %s not on channel", tmps3, chan);
	} else {
	  cmsgffake(chan,"c","Playing against %s", tmps3, chan);
	} 
      }	
    }  
  }  
}

void c4_kick() {
 if (paramcount!=3) { return; }
 if (tokentolong(params[2])==fake2long("c")) {
  sim_part(params[1],fake2long("c"));
 } 
}

void c4_invite() {
 char tmpnum[6]; 
 long xtime;
 if (paramcount!=4) { return; }
 toLowerCase(params[2]);
 if (strcmp(params[2],"c")==0) {
  longtotoken(fake2long("c"),tmpnum,5);
  xtime=getnettime();
  sendtouplink("%s J %s :%ld\r\n",tmpnum,params[3],xtime);
  sim_join(params[3],fake2long("c"));
  fflush(sockout);
 } 
}

/* help functions */

int getcid(char *chan) {
  int i;
  for (i=0;i<SIZEOFCL;i++){
    if (cls[i]!=NULL) {
      if (strcmp(cls[i]->name,chan)==0) {
        return i;
      }  
      printf("E %s :: %s",cls[i]->name,chan);
    }
  }  
  return -1;
}

/* connect4 stuff */

void initialize(fourinarow *four_game)
{
  int count1,count2;
  four_game->nmoves=0;
  for(count2=0;count2<PW;count2++)
  {
    four_game->pieces_in_a_column[count2]=0;
    for(count1=0;count1<PH;count1++)
    {
      four_game->matrix[count1][count2]=' ';
    }
  }
}

int checking(fourinarow *four_game,int col,char color)
{
  int count, count2,row,column;
  /* ----------------------- */
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color))
  {
    count++;
    count2++;
    row--;
  }
  if(count2>=WIN) return(1); 
  /* ---------------------- */
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(column>=0))
  {
    count++;
    count2++;
    column--;
  }
  if(count2>=WIN) return(1);
  count=0;
  count2--;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(column<7))
  {
    count++;
    count2++;
    column++;
  }
  if(count2>=WIN) return(1);
  /* ------------------------------ */
  count=0;
  count2=0;
  column=col;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row<6)&&(column>=0))
  {
    count++;
    count2++;
    row++;
    column--;
  }
  if(count2>=WIN) return(1);
  count=0;
  count2--;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row>=0)&&(column<7))
  {
    count++;
    count2++;
    row--;
    column++;
  }
  if(count2>=WIN) return(1); 
  /* ----------------------------------- */
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1; 
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row>=0)&&(column>=0)) 
  {
    count++;
    count2++;
    row--;
    column--;
  }
  if(count2>=WIN) return(1);
  count=0;
  count2--;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row<6)&&(column<7))
  {
    count++;
    count2++;
    row++;
    column++;
  }
  if(count2>=WIN) return(1);
  return(0);
}




/* Module stuff */

void connect4_init() {
 char tmps2[TMPSSIZE]; 
 long xtime;
 int i;
 
 xtime=getnettime();
 setmoduledesc(MODNAM,"Xevres 4wins module");
 createfakeuser2("C","connect4","games.xchannel.org","4 in a row");
 regfakecmd(MODNAM,"c","help", c4_help, 0, 0, "help\tShows very importent infos");
 regfchanmsg(MODNAM,"c",c4_chmsg);
 longtotoken(fake2long("c"),tmps2,5);
 sendtouplink("%s J #xchannel :%ld\r\n",tmps2,xtime);
 sim_join("#xchannel",fake2long("c"));
 fflush(sockout);
 registerserverhandler(MODNAM,"K",c4_kick);
 registerserverhandler(MODNAM,"I",c4_invite);
 
 for (i=0;i<SIZEOFCL;i++){
  gamez[i]->pl1=1;
  gamez[i]->pl2=1;
 } 
}

void connect4_cleanup() {
 deregisterserverhandler2("I",MODNAM);
 deregisterserverhandler2("K",MODNAM);
 deregfakecmd("help","c");
 deregfchanmsg("c",MODNAM);
 deregfchanmsg("c",MODNAM);
 fakekill2("c","Connect4 modul unloaded");
}

