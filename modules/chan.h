/* Xevres
 * Defines for Xevres Channel module
 */
#ifndef CHAN_H
#define CHAN_H

/* User Accessmodes */
#define UA_GIVE		0x1	/* g */
#define UA_VOICE	0x2	/* v */
#define UA_AUTO		0x4	/* a */
#define UA_OP		0x8	/* o */
#define UA_INVITE	0x10	/* i */
#define UA_LOG		0x20	/* l */
#define UA_COOWNER	0x40	/* c */
#define UA_OWNER	0x80	/* n */

/* Channel Modes */
#define CM_AUTOVOICE	0x1	/* v (auto give voice to joining clients) */
#define CM_PROTECT	0x2	/* p (protect ops from beeing deoped) */
#define CM_BITCHMODE	0x4	/* b (only allow known users to have chanflags) */
#define CM_SECUREDMODE	0x8	/* s (prevent anyone from being a chanop except x) */
#define CM_AUTOLIMIT	0x10	/* l [arg] (auto set the chanlimit+flag_limit */
#define CM_WELCOME	0x20	/* w [arg] (on join message flag_welcome */
#define CM_INVITE	0x40	/* i (force invite) */
#define CM_HIDDEN	0x80	/* h (force secret) */
#define CM_KEY		0x100	/* k [arg] (force key flag_key) */
#define CM_SUSPENDED	0x200	/* d [arg] (disabled or suspended in a level) */

/* some defines */
#define CM_DEFAULT	(CM_PROTECT+CM_AUTOLIMIT)	/* default channel modes for new channels */
#define UA_DEFAULT	(UA_AUTO+UA_OP+UA_LOG+UA_OWNER)	/* default user accessmode for chan creators */
#define DEF_LIMIT	3				/* default limit flag */
#define MAXINFOLEN	2048				/* length of info text */



/* structs */
typedef struct chanaccount {
  char name[NICKLEN+1];
  int aflags;
  struct userdata *autheduser;
  struct chanaccount *next;
} chacc;

typedef struct reggedchan {
  char name[CHANNELLEN+1];
  int cflags;
  struct channel *cptr;
  int flag_limit;
  int flag_suspendlevel;
  char flag_welcome[TOPICLEN+1];
  char flag_key[CHANKEYLEN+1];
  char flag_fwchan[CHANNELLEN+1];
  char info[MAXINFOLEN+1];
  long suspended_since;
  long suspended_until;
  char suspended_by[NICKLEN+1];
  struct chacc *canusers;
  struct reggedchan *next;
} rchan;

rchan *rchans;

/* Userinfo */
char *unum2auth(long unum);
int uhaccoc (char *xuser, char *xchan, char flag);
int uhacc (char *xuser);
int uikoc (char *xuser, char *xchan);

#endif

