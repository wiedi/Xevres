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

#endif

