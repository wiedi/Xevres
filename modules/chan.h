/* Xevres
 * Defines for Xevres Channel module
 */
#ifndef CHAN_H

#define CHAN_H

/* Userinfo */
char *unum2auth(long unum);
int uhaccoc (char *xuser, char *xchan, char flag);
int uhacc (char *xuser);
int uikoc (char *xuser, char *xchan);

/* Simul */
void ch_simjoin(char *xchan, long num);
void ch_simpart(char *xchan, long num);
void ch_simtopic(char *xchan, char *topic);
void ch_simmode(char *xchan, char *mode, long num);

#endif

