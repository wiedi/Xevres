/*
xevres connect4 module (a clone to beware's and plugwash's fp connect4)

  based on 4inarow by Davide Corrado (also known as Mr. Dave konrad)
  
  this is just a code example for the new module interface.
  note: this module wastes about 10MB and more of mem,
  	i'll write one that uses dynamic mem allocation ;)
  	--Wiedi
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#define MODNAM "connect4"
#define MVERSION "1.3"

/* Some options */
/* Mirc Color Code Char */
#define MC '\003' 
/* Player 1 Color */
#define P1C '7'
/* Player 2 Color */
#define P2C '3'
/* Background Color */
#define BC '1'
/* Text Color */
#define TC '0'
/* Heighth */
#define PH 5
/* Width */
#define PW 7
/* Won */
#define WIN 4
/* max moves */
#define NMOVES PH*PW

struct game {
  int nmoves;
  int pieces_in_a_column[PW];
  long pl1;
  long pl2;
  int lastmove;
  char matrix[PH][PW];
};
typedef struct game fourinarow;

fourinarow gamez[SIZEOFCL];

/* here is code */

/* proto */
int getcid(char *chan);
void initialize(fourinarow *four_game);
void print_game(fourinarow *four_game, char *chan);
int insert(fourinarow *four_game,int colonna,char colore);
int checking(fourinarow *four_game,int col,char color);

/* handler */
void c4_help(long unum, char *tail) {
 msgffake(unum,"c","If you want C to be on your channel just invite me. To get rid of me, kick me.");
 msgffake(unum,"c","Type in channel: !start <nick> and !stop to begin/end a game. !1 - !7 to insert coin.");
}

void c4_version(long unum, char *tail) {
 msgffake(unum,"c","Xevres Connect4 Module V%s (compiled on %s)",MVERSION, __DATE__);
 msgffake(unum,"c","Coded by Wiedi based on the idea of beware's/plughwash's pascal connect4.");
 msgffake(unum,"c","Some code stolen from Davide Corrado's 4inarow.");
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
      if (((tail[1] == '1') || (tail[1] == '2') || (tail[1] == '3') || (tail[1] == '4') || (tail[1] == '5') || (tail[1] == '6') || (tail[1] == '7')) && tail[2] == '\0') {
        /* !1 - !7 */
        if ((gamez[cid].lastmove==2) ? gamez[cid].pl1==unum : gamez[cid].pl2==unum) {
          /*play*/
	  /* check if row full */
	  if(gamez[cid].pieces_in_a_column[atoi(&tail[1])-1]<PH) {
	    insert(&gamez[cid],atoi(&tail[1])-1,(gamez[cid].lastmove==2) ? 'X' : 'O');
            if (gamez[cid].lastmove==2) {gamez[cid].lastmove=1;} else {gamez[cid].lastmove=2;}
	    print_game(&gamez[cid],chan);
            if(checking(&gamez[cid],atoi(&tail[1])-1,(gamez[cid].lastmove==1) ? 'X' : 'O')==1) {
              /* win */
	      cmsgffake(chan,"c","%c%c,%c %s %c%c,%c wins! ", MC, (gamez[cid].lastmove==2) ? P2C : P1C, BC, (gamez[cid].lastmove==1) ? unum2nick(gamez[cid].pl1) : unum2nick(gamez[cid].pl2), MC, TC, BC);  
	      initialize(&gamez[cid]);
	      return;
            }
	    if(gamez[cid].nmoves==NMOVES) {
	      /* draw */
	      cmsgffake(chan,"c","Draw game!");
	      initialize(&gamez[cid]);
	      return;
	    }
	    cmsgffake(chan,"c","%c%c,%c %s %c%c,%cto move ", MC, (gamez[cid].lastmove==1) ? P2C : P1C, BC, (gamez[cid].lastmove==1) ? unum2nick(gamez[cid].pl2) : unum2nick(gamez[cid].pl1), MC, TC, BC);
          } else {
	    cmsgffake(chan,"c","Column %c is full.",tail[1]);
	  }  
        }	
      } else if (strcmp(tmps2,"!stop")==0) {
        /* check if unum = op/oper/player */
	cmsgffake(chan,"c","Game ended.");
	initialize(&gamez[cid]);
      }	
    } else if (res2 == 2) {
      /* !play <player> */
      if (strcmp(tmps2,"!play")==0) {
        if (gamez[cid].pl1!=-1) {
          cmsgffake(chan,"c","There is a game already in progress.");
	  return;
        }	
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
	  return;
	} else if (ux==unum) {
	  cmsgffake(chan,"c","You can't play against yourself!");
	  return;  
	} else {
	  /* start the game */
	  gamez[cid].pl1=unum;
	  gamez[cid].pl2=ux;
	  gamez[cid].lastmove=2;
	  print_game(&gamez[cid],chan);
	  cmsgffake(chan,"c","%c%c,%c %s %c%c,%cto move ", MC, P1C, BC, unum2nick(gamez[cid].pl1), MC, TC, BC);
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

/* this looks normaly nicer but with some irc clients it's just ugly */
void print_game_old(fourinarow *four_game, char *chan) {
  char tmps[TMPSSIZE];
  int count1,count2;
  
  cmsgffake(chan,"c","%c%c,%cGame: %s vs. %s", MC, TC, BC, unum2nick(four_game->pl1), unum2nick(four_game->pl2));
  for(count1=PH-1;count1>=0;count1--)
  {
    sprintf(tmps,"%c%c,%c |", MC, TC, BC);
    for(count2=0;count2<PW;count2++)
    {
      sprintf(tmps,"%s%c%c,%c %c |", tmps, MC, TC, BC, four_game->matrix[count1][count2]);
    }
    cmsgffake(chan,"c",tmps);
    sprintf(tmps,"%c%c,%c +", MC, TC, BC);
    for(count2=0;count2<PW;count2++) {
      sprintf(tmps, "%s---+",tmps);
    }
    cmsgffake(chan,"c",tmps);
  }
  sprintf(tmps,"%c%c,%c  ", MC, TC, BC);
  for(count2=0;count2<PW;count2++)
  {
    sprintf(tmps,"%s %d  ", tmps, count2+1);
  }
  cmsgffake(chan,"c",tmps);
}

void print_game(fourinarow *four_game, char *chan) {
  char tmps[TMPSSIZE];
  int count1,count2;
  
  cmsgffake(chan,"c","%c%c,%c%s %c%c,%cVS %c%c,%c%s ", MC, P1C, BC, unum2nick(four_game->pl1), MC, TC, BC, MC, P2C, BC, unum2nick(four_game->pl2));
  for(count1=PH-1;count1>=0;count1--) {
    sprintf(tmps,"%c%c,%c ", MC, TC, BC);
    for(count2=0;count2<PW;count2++) {
      sprintf(tmps,"%s%c%c,%c%c ", tmps, MC, (four_game->matrix[count1][count2]=='_') ? TC : ((four_game->matrix[count1][count2]=='X') ? P1C : P2C) , BC, four_game->matrix[count1][count2]);
    }
    cmsgffake(chan,"c",tmps);
  }
  sprintf(tmps,"%c%c,%c ", MC, TC, BC);
  for(count2=0;count2<PW;count2++) {
    sprintf(tmps,"%s%d ", tmps, count2+1);
  }
  cmsgffake(chan,"c",tmps);
}

void initialize(fourinarow *four_game) {
  int count1,count2;
  four_game->nmoves=0;
  four_game->pl1=-1;
  four_game->pl2=-1;
  four_game->lastmove=0;
  for(count2=0;count2<PW;count2++)
  {
    four_game->pieces_in_a_column[count2]=0;
    for(count1=0;count1<PH;count1++)
    {
      four_game->matrix[count1][count2]='_';
    }
  }
}

int insert(fourinarow *four_game,int colonna,char colore) {
  if(four_game->pieces_in_a_column[colonna]<PH) {
    four_game->nmoves++;
    four_game->matrix[four_game->pieces_in_a_column[colonna]][colonna]=colore;
    four_game->pieces_in_a_column[colonna]++;
    return(1);
  }
  else return(0);
}

int checking(fourinarow *four_game, int col, char color) {
  int count, count2,row,column;
  /* vertikal nach unten*/
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)) {
    count++;
    count2++;
    row--;
  }
  if(count2>=WIN) return(1); 
  /* horizontal nach links, dann nach rechts */
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(column>=0)) {
    count++;
    count2++;
    column--;
  }
  if(count2>=WIN) return(1);
  count=0;
  count2--;
  column=col;
  row=four_game->pieces_in_a_column[column]-1;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(column<7)) {
    count++;
    count2++;
    column++;
  }
  if(count2>=WIN) return(1);
  /* diagonal von rechts unten nach links oben*/
  count=0;
  count2=0;
  column=col;
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row<6)&&(column>=0)) {
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
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row>=0)&&(column<7)) {
    count++;
    count2++;
    row--;
    column++;
  }
  if(count2>=WIN) return(1); 
  /* diagonal von links oben nach rechts unten */
  count=0;
  count2=0;
  column=col;
  row=four_game->pieces_in_a_column[column]-1; 
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row>=0)&&(column>=0)) {
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
  while((count<WIN)&&(four_game->matrix[row][column]==color)&&(row<6)&&(column<7)) {
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
 regfakecmd(MODNAM,"c","showcommands", c4_help, 0, 0, "showcommands\tAlias for help");
 regfakecmd(MODNAM,"c","version", c4_version, 0, 0, "version\tShows version and credits");
 regfchanmsg(MODNAM,"c",c4_chmsg);
 longtotoken(fake2long("c"),tmps2,5);
 sendtouplink("%s J #xchannel :%ld\r\n",tmps2,xtime);
 sim_join("#xchannel",fake2long("c"));
 fflush(sockout);
 registerserverhandler(MODNAM,"K",c4_kick);
 registerserverhandler(MODNAM,"I",c4_invite);
 for (i=1;i<SIZEOFCL;i++){
   initialize(&gamez[i]);
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

