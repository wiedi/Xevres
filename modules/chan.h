/*  Xevres
 * Defines for Xevres Channel module
 */

#ifndef CHAN_H
#define CHAN_H

/* User Flags */
#define UF_GIVE		0x1	/* g */
#define UF_VOICE	0x2	/* v */
#define UF_AUTO		0x4	/* a */
#define UF_OP		0x8	/* o */
#define UF_INVITE	0x10	/* i */
#define UF_LOG		0x20	/* l */
#define UF_COOWNER	0x40	/* c */
#define UF_OWNER	0x80	/* n */

/* Channel Flags */
#define CF_AUTOVOICE	0x1	/* v (auto give voice to joining clients) */
#define CF_PROTECT	0x2	/* p (protect known ops from beeing deoped) */
#define CF_FORCE	0x4	/* f (only allow known users to have chanflags) */
#define CF_SECURE	0x8	/* s (prevent anyone from being a chanop except x (and owner)) */
#define CF_LIMIT	0x10	/* l [arg] (auto set the chanlimit+flag_limit */
#define CF_WELCOME	0x20	/* w [arg] (on join message flag_welcome */
#define CF_INVITE	0x40	/* i (force invite) */
#define CF_HIDDEN	0x80	/* h (force secret) */
#define CF_KEY		0x100	/* k [arg] (force key flag_key) */
#define CF_SUSPENDED	0x200	/* S [arg] (disabled or suspended in a level) */
#define CF_FORWARD	0x400	/* F [arg] (forward can only be set by opers) */

/* my beloved makros and me */
#define UHasGive(x)		((x)->aflags & UF_GIVE)
#define UHasVoice(x)		((x)->aflags & UF_VOICE)
#define UHasAuto(x)		((x)->aflags & UF_AUTO)
#define UHasOp(x)		((x)->aflags & UF_OP)
#define UHasInvite(x)		((x)->aflags & UF_INVITE)
#define UHasLog(x)		((x)->aflags & UF_LOG)
#define UHasCoowner(x)		((x)->aflags & UF_COOWNER)
#define UHasOwner(x)		((x)->aflags & UF_OWNER)

#define CIsAutovoice(x)		((x)->cflags & CF_AUTOVOICE)
#define CIsProtect(x)		((x)->cflags & CF_PROTECT)
#define CIsForce(x)		((x)->cflags & CF_FORCE)
#define CIsSecuremode(x)	((x)->cflags & CF_SECURE)
#define CIsLimit(x)		((x)->cflags & CF_LIMIT)
#define CIsWelcome(x)		((x)->cflags & CF_WELCOME)
#define CIsInvite(x)		((x)->cflags & CF_INVITE)
#define CIsHidden(x)		((x)->cflags & CF_HIDDEN)
#define CIsKey(x)		((x)->cflags & CF_KEY)
#define CIsSuspended(x)		((x)->cflags & CF_SUSPENDED)
#define CIsForward(x)		((x)->cflags & CF_FORWARD)

/* some defines */
#define CM_DEFAULT	(CF_PROTECT+CF_LIMIT)		/* default channel modes for new channels */
#define UA_DEFAULT	(UF_AUTO+UF_OP+UF_LOG+UF_OWNER)	/* default user accessmode for chan creators */
#define DEF_LIMIT	3				/* default limit flag */ 
#define CHANEXPIRE	60*3600*24			/* when will a chan be deleted */
#define SUSPGL_DUR	60*30				/* how long will clients glined for joining a */
							/* suspended channel */
/* structs */
typedef struct chanaccount {
  char name[NICKLEN+1];
  int aflags;
  struct chanaccount *next;
} cuser;

typedef struct reggedchan {
  char name[CHANNELLEN+1];
  char creator[NICKLEN+1];
  long cdate;
  int cflags;
  struct channel *cptr;
  int flag_limit;
  int flag_suspendlevel;
  char flag_welcome[TOPICLEN+1];	/* welcome message, missused as suspend or fw reason */
  char flag_key[CHANKEYLEN+1];
  char flag_fwchan[CHANNELLEN+1];
  char info[TOPICLEN+1];
  long suspended_since;
  long suspended_until;
  char suspended_by[NICKLEN+1];
  int had_s;				/* we set mode +s when the last user parts,
  					   here we remember if +s was already set (1),
					   not (0) or more than 1 user (x) is on the
					   channel (-1), to restore the modes when a user
					   joins */
  long lastused;			/* if now-this>CHANEXPIRE then del the chan */
  struct chanaccount *cusers;
  struct reggedchan *next;
} rchan;

/* borrowed from clearchan.c */
typedef struct {
  long num;
  char ident[USERLEN+1];
  char host[HOSTLEN+1];
} clearhelp;

rchan *rchans;
cuser nulluser;
extern flags uflags[];

/* database things */
void ch_readdb(void);
void ch_savedb(long unum, char *tail);

/* help functions */
rchan *ch_getchan(char *chan);
cuser *ch_getchanuser(rchan *cp, char *account);
cuser *ch_getchanuserbynick(rchan *cp, char *nick);
void ch_setlimit(char *chan, int lim);
void ch_setlimit2(channel *c, int lim);
void ch_msgtolog(rchan *cp, const char *template, ...);
void ch_chandel(char *chan, char *msg);

/* Userinfo */
char *unum2auth(long unum);
int uhacc (char *xuser);

#endif
