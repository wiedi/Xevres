 /* Xevres
 * Definition of global variables, datatypes etc.
 */

#ifndef GLOBALS_H

#define GLOBALS_H

#include "mysql_version.h"
#include "mysql.h"
#include "config.h"
#include "stringtools.h"

#define operservversion "0.8.3"

#define um_o 0x1	// +o on a channel (op)
#define um_v 0x2	// +v on a channel (voice)
#define cm_p 0x1	// chanmode +p (private)
#define cm_s 0x2	// chanmode +s (secret)
#define cm_m 0x4	// chanmode +m (moderated)
#define cm_n 0x8	// chanmode +n (no outside message)
#define cm_t 0x10	// chanmode +t (only ops can change topics)
#define cm_i 0x20	// chanmode +i (invite only)
#define cm_l 0x40	// chanmode +l (limit)
#define cm_k 0x80	// chanmode +k (key)
#define cm_c 0x100	// chanmode +c (no colors)
#define cm_C 0x200	// chanmode +C (no CTCPs)
#define cm_D 0x400	// chanmode +D (auditorium)
#define cm_r 0x800	// chanmode +r (only registered users)
#define cm_u 0x1000	// chanmode +u (hide PART/QUIT messages)
#define cm_O 0x2000	// chanmode +O (Oper only channel)
#define cm_F 0x4000	// chanmode +F (Forward Chan)

#define NICKLEN 15     // Maximum nick length
#define USERLEN 10     // Maximum ident length
#define HOSTLEN 63     // Maximum hostname length
#define REALLEN 50     // Maximum realname length
#define CHANNELLEN 200 // Maximum length of a channel name
#define SRVLEN 200     // Maximum length of a servername
#define TOPICLEN 250   // Maximum length of a topic
#define CHANKEYLEN 26  // Maximum length of a channelkey
#define MAXUMODES 20   // Maximum number of usermodes
#define AUTHUSERLEN 40 // Maximum Length of a Username for authing
#define AUTHPASSLEN 40 // Maximum length of a Password for authing
#define RNGLEN 150     // Realname GLINE length (for the mask)
#define RNGREAS 100    // Maximum length of reason in Realname-GLINEs
#define RNGTIM 1800    // How many seconds the realname-glines will be set
/* You need to adapt the following settings to the size of your network.
   If you make them too small, the size of the chains dangling from the
   hashtable-buckets will become too big, and X will become *S*L*O*W*
   Your average user/channelcount at peak time should be a good value for this. */
#define SIZEOFUL 100000 // How many chained lists form the Userlist
#define SIZEOFCL 100000 // The same for the channellist
#define SIZEOFNL 100000 // and the Nicklist
#define TMPSSIZE 1040  // Size of temporary strings. this is 1040 because a
                       // string from the server has at most 512 bytes.
                       // 2 * 512 + some bytes for safety makes 1040
#define SRVSHIFT 18    // Servernumeric-Multiplier. Servernumeric SHIFTLEFT
                       // SRVSHIFT + Clientnumeric = complete client numeric
#define SRVNUMMULT 262144 // The same, but for use with "%"
#define ARLIM1 100     // If new memory is needed for an array, this many
                       // records will be allocated.
#define ARLIM2 150     // If more than this number of records are free in an
                       // array, memory is freed. Those are only default
                       // values, arrays.c provides functions to change them.
#define MAXPARAMS 30   // Every commandline received from the server gets split
                       // up now. I'm pretty sure this will never exceed 30
                       // parts.
#define MODNAMELEN 50  // Maximum length of a module name (without path)
#define MODDESCLEN 100 // Length for short module description.
#define COMMANDLEN 50  // Max. length for dynamically loaded commands
#define EXCESSGLINELEN 60*60*24*180 // How long is an excessively long gline?
#define MAXINDIVIDUALGLINESYNC 10   // If more than this number of servers split
                                    // we'll resync glines to everyone to save
                                    // traffic

#define NM_GLINE  0x1				/* GLINEs set / removed */
#define NM_RNGL   0x2				/* Realname-GLINEs set */
#define NM_TRUSTS 0x4				/* Trusts added / removed */
#define NOTICEMASKDEFAULT NM_GLINE		/* Default noticemask for ircops */

#define TRUSTNAMELEN		70		/* Length of the name for a group of trusted hosts */
						/* - MUST be at least HOSTLEN! */
#define TRUSTCONTACTLEN		150		/* Length of admin-email */
#define TRUSTCOMLEN		200		/* Length of the field for additional comments */
#define SIZEOFTL		1000		/* Size of trustlist (contains trusted hosts) */
#define SIZEOFIDMAP		1000		/* Size of the table mapping trustgroup-ids to names */
#define SIZEOFSHL		50		/* Size of serverhandler list hash table */
#define GLINEMAXHIT		5000		/* How many users a normal oper can hit with a GLINE */

#define CFMINUSERS		4		/* Minimum number of users needed for chanfix */
#define CFMAXSPLITSERVERS	10		/* Maximum number of split servers before we abandon chanfix */
#define CFINTERVAL		30		/* Channel scan interval */
#define CFREMEMBEROPSMAX	10*(3600*24)	/* Max time to remember ops for (10 days) */
#define CFREMEMBERPERPOINT	3600		/* How long each point lasts */
#define CFMINCANDIDATES		5		/* Minimum number of people eligible for reopping */
#define CFCANDIDATEPERCENT	25		/* The top this-percent are eligible for reopping */
#define CFMINSCORE		6		/* An "regular op" needs at least this many points to get reopped */
#define CFREMEMBERCHAN		10*(3600*24)	/* How long to remember channels for (10 days) */
#define CFFILENAME		"chandb"	/* File to save out channel database to */
#define CFSCANBUCKETS		10000		/* Number of buckets to scan each interval */
						/* You should st this to 1/10th of SIZEOFCL ! */

#define REOP_OPPED         0  /* Opped at least one user */
#define REOP_NOINFO       -1  /* No regular op info for channel */
#define REOP_NOREGOPS     -2  /* No regular ops on channel */
#define REOP_NOMATCH      -3  /* No suitable op on channel */

#define SIZEOFSNC 1024 /* Size of the subnetcount-hashtable */

typedef struct array array;
struct array {
  unsigned long cursi;
  unsigned long maxsi;
  unsigned int memberlen;
  unsigned int arlim1;
  unsigned int arlim2;
  void *content;
};

typedef struct userdata userdata;
struct userdata {  /* This struct holds information about a user on the net */
  char nick[NICKLEN+1];
  int hopsaway;
  long numeric;
  char ident[USERLEN+1];
  char *host;
  char umode[MAXUMODES+1];
  char *realname;
  char authname[NICKLEN+1];
  char md5[16];
  array chans;
  unsigned long realip;     /* The real IP of the client as a 32 bit integer */
                            /* Obviously this is only compatible with IPv4! */
  unsigned long connectat;  /* Timestamp: When this client connected */
  userdata *next;           /* Pointer to the next user (linked list) */
};

typedef struct serverdata serverdata;
struct serverdata {
  char nick[SRVLEN+1];
  int hopsaway;
  long numeric;
  long createdat;
  unsigned long connectat;
  long connectedto;
  long maxclients;
  long pingreply;
  long pingsent;
  char ports[1000];
  serverdata *next;
};

typedef struct chanuser chanuser;
struct chanuser {
  long numeric;
  int flags;
};

typedef struct regchannel regchannel;
struct regchannel {
  char *name;
  time_t lastexamined;
  array regops;
};

typedef struct channel channel;
struct channel {
  char *name;
  long createdat;
  int flags;
  char chankey[CHANKEYLEN+1]; // Channelkey - if the channel is not +k, the content
                              // of this is undefined!!
  char fwchan[CHANNELLEN+1];  // for umode +F
  int chanlim;        // Channellimit - same as with chankey
  array users;
  regchannel *reg;
  unsigned char
    canhavebans:1;    // If set to 1, there MIGHT be Bans set on the channel.
  char *topic;
  channel *next;
};

typedef struct flags flags;
struct flags {
  char c;
  int v;
};

typedef struct subnetcount subnetcount;
struct subnetcount {
  unsigned long count;
  subnetcount * succ0;
  subnetcount * succ1;
};

typedef struct subnetchelp subnetchelp;
struct subnetchelp {
  unsigned long sn;
  subnetcount * snc;
  subnetchelp * next;
};

typedef struct authdata authdata;
struct authdata {
  char username[AUTHUSERLEN+1];
  char password[AUTHPASSLEN+1]; // if authlevel is >=100, this is only a hash, not the
                     // cleartext password!
  int authlevel;     // 1 - 1000
  long lastauthed;
  int wantsnotice;
  unsigned long noticemask;
};

typedef struct autheduser autheduser;
struct autheduser {
  long numeric;
  char authedas[AUTHUSERLEN+1];
  int authlevel;
  int wantsnotice;
  unsigned long noticemask;
  autheduser *next;
};

typedef struct rnlentry rnlentry;
struct rnlentry {
  char *r;
  int n;
};

typedef struct realnamegline realnamegline;
struct realnamegline {
  char mask[RNGLEN+1];
  char creator[AUTHUSERLEN+1];
  unsigned long expires;
  int howtogline; /* 1: *@host
                     2: user@host
                  */
  char reason[RNGREAS+1];
  unsigned long timesused;  // Only some little stats, how often this has been
                            // used since O was started
};

typedef struct afakeuser afakeuser;
struct afakeuser {
  char nick[NICKLEN+1];
  long numeric;
  char ident[USERLEN+1];
  char host[HOSTLEN+1];
  char realname[REALLEN+1];
  unsigned long connectat;
};

typedef struct splitserv splitserv;
struct splitserv {
  char name[SRVLEN+1];
  int state;
  unsigned long time;
};

typedef struct loadedmod loadedmod;
struct loadedmod {
  char name[MODNAMELEN+1];
  void *handle;
  char shortdesc[MODDESCLEN+1];
};

typedef struct dyncommands dyncommands;
struct dyncommands {
  char name[COMMANDLEN+1];
  char providedby[MODNAMELEN+1];
  char *hlptxt;
  int operonly;
  int minlev;
  void (*func)(long unum, char *tail);
};

typedef struct dynfakecmds dynfakecmds;
struct dynfakecmds {
  char name[COMMANDLEN+1];
  char providedby[MODNAMELEN+1];
  char fakenick[NICKLEN+1];
  char *hlptxt;
  int operonly;
  int minlev;
  void (*func)(long unum, char *tail);
};

typedef struct aserverhandler aserverhandler;
struct aserverhandler {
  char name[COMMANDLEN+1];
  char providedby[MODNAMELEN+1];
  void (*func)(void);
};

typedef struct aieventhandler aieventhandler;
struct aieventhandler {
  char name[COMMANDLEN+1];
  char providedby[MODNAMELEN+1];
  void (*func)(char *xparam);
};

/* channel msg for fake user */
typedef struct fchmsg fchmsg;
struct fchmsg {
  char fakenick[NICKLEN+1];
  char providedby[MODNAMELEN+1];
  void (*func)(long unum, char *chan, char *tail);
};

typedef struct trustedgroup trustedgroup;
struct trustedgroup {
  unsigned long id; // MUST NOT be 0
  char name[TRUSTNAMELEN+1];
  unsigned long trustedfor;
  unsigned long currentlyon;
  unsigned long lastused;
  unsigned long maxused;
  unsigned long maxreset;
  unsigned long expires;
  unsigned long maxperident;
  int enforceident;
  char contact[TRUSTCONTACTLEN+1];
  char comment[TRUSTCOMLEN+1];
  array identcounts;
  trustedgroup *next;
  char creator[AUTHUSERLEN+1];
};

typedef struct trustedhost trustedhost;
struct trustedhost {
  // char hostmask[] - removed, only using IPs now
  unsigned long IPv4;   // the hosts IP
  unsigned long id; // MUST NOT be 0
  unsigned long lastused; // When was the last time a user connected from
                          // this host? 0 == never!
  unsigned long maxused; // Maximum number of users ever seen on this host
  unsigned long maxreset; // timestamp: Last reset of "maxusers"
  unsigned long currentlyon; // currently online
  trustedhost *next;
};

typedef struct identcounter identcounter;
struct identcounter {
  char ident[USERLEN+1];
  unsigned long currenton;
};

typedef struct trustdeny trustdeny;
struct trustdeny {
  unsigned long v4net;   // Subnet and...
  unsigned int v4mask;  // ... Mask.
  char creator[AUTHUSERLEN+1];
  char reason[RNGREAS+1];
  unsigned long expires;  /* 0 == Does NOT expire! */
  unsigned long created;
  int type;  /* see TRUSTDENY_* Values below */
};

#define TRUSTDENY_WARN 0
#define TRUSTDENY_DENY 1

typedef struct hostcounter hostcounter;
struct hostcounter {
  char *hostname;
  int currenton;
};

typedef struct regop regop;
struct regop {
  char md5[16];
  time_t lastopped;
  unsigned int score;
};

extern serverdata *sls;
extern userdata * uls[SIZEOFUL]; /* Userlist */
extern channel * cls[SIZEOFCL];  /* Channellist */
extern array nls[SIZEOFNL];      /* Nick-lists */
extern array reg[SIZEOFCL];      /* Registered channel list */
extern array serverhandlerlist[SIZEOFSHL]; // List of all serverhandlers
extern array internaleventlist[SIZEOFSHL]; // List of all serverhandlers
extern array fchanmsglist[SIZEOFSHL]; // List of all channel message handlers for fake cliens 
extern autheduser *als;  /* List of authed users */
extern flags chanflags[];
extern int numcf; /* Number of Chanflags */
extern flags userflags[];
extern int numuf; /* Number of Userflags (Flags of a user on a channel) */
extern FILE *sockin, *sockout;
extern char lastline[520]; /* Last line received from the ircserver */
extern const char tokens[];
extern const unsigned int reversetok[];
extern const char crytok[];
extern const char upwdchars[];
extern char command[TMPSSIZE];
extern char conpass[100];
extern char mynick[NICKLEN+1];
extern char myname[100];
extern char conto[100];
extern unsigned int conport;
extern char sqluser[100];
extern char sqlpass[100];
extern char sqlhost[100];
extern char sqldb[100];
extern int sqlport;
extern char servernumeric[3];
extern char uplnum[3];
extern char lastburstchan[520];
extern int lastburstflags;
extern long starttime;  // Timestamp, contains starttime of the service
extern int burstcomplete;  // Is set to 1 once the netburst completed
extern char logfile[FILENAME_MAX];
extern char helppath[300];
extern long timestdif;   // Timestamp-difference
extern MYSQL sqlcon;
extern array mainrnl[256];
extern long snlstartsize;
extern array rngls;
extern long mf4warn[33];
extern const unsigned int mf4warnsteps[33];
extern long lastsettime;
extern long waitingforping;
extern long lasthourly;    // Timestamp: Last time we did our hourly duties
extern long laststatswrite; // Timestamp: Time that stats were written to the DB
extern long lastfakenum;
extern long resyncglinesat;
extern char sendglinesto[20480];
extern char *noreplytoping;
extern array myfakes; /* List of my fakeusers */
extern char *nicktomsg;
extern char params[MAXPARAMS][TMPSSIZE]; // The last line received from the
                                         // server, split at the spaces.
extern int paramcount;                   // How many slots of params[] are used
extern char sender[TMPSSIZE];            // The sender of the last line -
                                         // always as a numeric!
extern array splitservers; // List of splits, see splitdb.c
extern char modpath[FILENAME_MAX]; // Path where loadable modules reside.
extern char modend[10];        // What filesuffix modules have
extern array modulelist;      // List of all loaded modules
extern array commandlist;     // List of all loaded commands
extern array fakecmdlist;     // List of all loaded commands for fakeusers
extern array serverhandlerlist[SIZEOFSHL]; // List of all serverhandlers
extern array internaleventlist[SIZEOFSHL]; // List of all internal events
extern trustedhost * trustedhosts[SIZEOFTL];       // The trusted hosts
extern trustedgroup * trustedgroups[SIZEOFIDMAP];
extern int glineonclones;
extern int glineautosync;
extern volatile int igotsighup;
extern char *configfilename;
extern array deniedtrusts; /* List of denied trusts */
extern subnetchelp * ipv4snc[SIZEOFSNC];  /* Hashtable for subnetcounters */
extern int ipv4sncstartmask;  /* Subnetcount starts with this mask */
extern subnetchelp * impv4snt[SIZEOFSNC]; /* IMPlicit SubNet-Trusts - calculated from trustdata */
extern unsigned long ipv4usercount; /* Number of users on 0.0.0.0/0 */
extern unsigned long netmasks[33]; /* Netmasks */
extern char logtimestampformat[TMPSSIZE/2];   /* strftime format for Log timestamps */
extern int uplinkup; /* server connection ready to use? */
extern int weareinvisible; /* running in background */
extern int stayhere; /* don't launch into background */
/* End of global variable definitions */

/* Define important functions */

/* xevres.c */
int alreadyinglinesto(char *nick);
void showhelp(long unum, char *command);
void getuserdata(authdata *retad, char *username);
long countusersinul(void);
void createrandompw(char *a,int n);
int updateuserinul(authdata ad);
autheduser * getalentry(long unum);
void newalentry(long unum);
void addautheduser(long unum, authdata ad);
void delautheduser(long unum);
int opensql(char *host, char *username, char *password, char *database, int port);
int tcpopen(char *host, unsigned int port);
int numericinuse(long num);
void delchan(char *name);
void deluserfromallchans(long num);
void delallchanson(long num);
channel * getchanptr(char *name);
long getsrvnum(char *a);
int serverdoesnotreplytopings(serverdata *s);
void pingallservers(void);
long countusershit(char *pattern);
long noticeallircops(char *thetext);
void delalluserson(long num);
void delsrv(char *a);
void deluserfromchan(char *name, long numeric);
void newserver(char *nick, int hopsaway, long numeric, long createdat, long connectat, long connectedto, long maxclients);
int chanexists(char *name);
channel * newchan(char *name, long createdat);
void setchankey(char *name, char *key);
void setchanke2(channel *a, char *key);
void setfwchan(char *name, char *chan);
void setfwchan2(channel *a, char *chan);
void saveall(void);
long countusersonsrv(long num);
long countopersonsrv(long num);
unsigned long countusersonthenet(void);
long nicktonum(char *a);
void numtohostmask(long num, char *res);
void numtonick(long num, char *res);
int loadconfig(char *filename, int isrehash);
void changechanmode(char *name, long numeric, int sm, int flags);
void changechanmod2(channel *a, long numeric, int sm, int flags);
int getchanmodes(char *name, long numeric);
int getchanmode2(channel *a, long numeric);
long getmaxping(void);
int gotallpingreplys(void);
void purgeglines(void);
void remgline(char *hostmask, int sendtonet);
void addgline(char *hostmask, char *reason, char *bywhom, long howlong, int sendtonet);
void addusertochan(char *name, long numeric);
void addusertocptr(channel *b, long numeric);
void delchanfla2(channel *a, int flags);
void setchanfla2(channel *a, int flags);
void delchanflag(char *name, int flags);
void setchanflag(char *name, int flags);
void setchanli2(channel *a, int lim);
void setchanlim(char *name, int lim);
void addnicktoul(userdata *a);
int rnladd(char *realname);
void rnldel(char *realname);
long nicktonu2(char *a);
void dochanneljoins(void);
void addchantouser2(long num, channel *c);
void addchantouser(userdata *a, channel *c);
void delchanfromuser(long num, char *channam);
void addtonicklist(userdata *a);
void delfromnicklist(char *nick);
void writestatstodb(void);
int isglineset(char *mask);
void lastlinesplit(void); 
void goaway(void);
/* End of xevres.c */

/* subnetlist.c */
void sncadd(unsigned long IP, unsigned long *res);
void sncdel(unsigned long IP);
unsigned long sncget(unsigned long IP, unsigned long mask);
void recreateimpsntrusts(void);
void getimpsntrusts(unsigned long IP, unsigned long *res);
/* End of subnetlist.c */

/* fakeuser.c */
void createfakeuser(long num, char *nick, char *ident, char *host, char *realname, int sendtonet);
int clinuminuse(long senu, long clnu);
long getfreefakenum(void);
void fakeuseradd(char *nick, char *ident, char *host, char *realname, long numeric);
void fakeuserdel(long numeric);
void fakeuserkill(long numeric);
void fakeuserload(void);
void fakeusersave(void);
void dofakelist(long unum, char *tail);
void dofakekill(long unum, char *tail);
void docreatefakeuser(long unum, char *tail);
void createfakeuser2(char *nick, char *ident, char *host, char *realname);
void fakekill2(char *nick, char *qmsg);
long fake2long(char *nick);
/* End of fakeuser.c */

/* realnamegline.c */
void rnglcreate(void);
void rngladd(char *mask, char *creator, long expires, int howtoban, char *reason);
void rnglcheck(userdata *a);
void rngldel(char *mask);
void rnglexpire(void);
void rnglload(void);
void rnglsave(void);
void dorngline(long unum, char *tail);
void dornungline(long unum, char *tail);
void dornglist(long unum, char *tail);
void rnglinelog(unsigned long timestamp, char *event);
void dornglogsearch(long unum, char *tail);
void dornglogpurge(long unum, char *tail);
/* End of realnamegline.c */

/* subnettrust.c - OBSOLETED */
void loadtrustlist();
/* End of subnettrust.c */

/* general.c */
time_t getnettime(void);
unsigned long parseipv4(char *ip);
int isvalidipv4(char *ip);
void splitip(unsigned long IP, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d);
char * printipv4(unsigned long IPv4);
unsigned int clhash(char *txt);
unsigned int ulhash(long num);
unsigned int nlhash(char *nick);
unsigned int shlhash(char *cmd);
void encryptpwd(char *res, char *pass);
int checkpwd(char *encrypted, char *pass);
int match2strings(char *patrn, char *strng);
int isflagset(int v, int f);
void flags2string(int flags, char *buf);
int alldigit(char *s);
unsigned long tokentolong(char* token);
void longtotoken(unsigned long what, char* result, int dig);
unsigned long iptolong(unsigned int a, unsigned int b, unsigned int c, unsigned int d);
void toLowerCase(char* a);
void toLowerCase2(char* a);
void normnum(char* a);
void delchar(char *a, char b);
void appchar(char *a, char b);
long durationtolong(char *dur);
void longtoduration(char *dur, long what);
int ischarinstr(char a, char *b);
unsigned long crc32(const unsigned char *buf);
void somethingtonumeric(char *what, char *result);
void unescapestring(char *src, char *targ);
userdata * getudptr(long numeric);
serverdata * getsdptr(long numeric);
int killuser(long num);
void mystrncpy(char *targ, char *sour, long n);
void putlog(const char *template, ...);
int getauthlevel(long unum);
void getauthedas(char *res, long unum);
int getwantsnotice(long unum);
int getwantsnotic2(long unum);
void msgtouser(long unum, char *txt);
void newmsgtouser(long unum, const char *template, ...);
void msgffake(long unum, char *nick, const char *template, ...);
void cmsgffake(char *chan, char *nick, const char *template, ...);
int checkauthlevel(long unum, int minlevel);
void sendtonoticemask(unsigned long mask, char *txt);
int isircop(long num);
unsigned long getnoticemask(long unum);
void setnoticemask(long unum, unsigned long mask);
void strreverse(char *s);
char *md5tostr(char *md5);
void deopall(channel *c);
char * alacstr(char *x);
void sendtouplink(const char *template, ...);
char *unum2nick(long unum);
char *unum2auth(long unum);
void sim_join(char *xchan, long num);
void sim_part(char *xchan, long num);
void sim_topic(char *xchan, char *topic);
void sim_mode(char *xchan, char *mode, long num);
const char *flagstostr(flags tbl[], int i);
int flagstoint(flags tbl[], char *c);
/* End of general.c */

/* serverhandlers.c */
void handlenickmsg(void);
void handlequitmsg(void);
void handleservermsg(int firsts);
void handleping(void);
void handlekillmsg(void);
void handlesquit(void);
void handlemodemsg(void);
void handleprivmsg(void);
void handleburstmsg(void);
void handlecreatemsg(void);
void handlejoinmsg(void);
void handlepartmsg(void);
void handlekickmsg(void);
void handleglinemsg(void);
void handlestatsPreply(void);
void handlestatsureply(void);
void handlestatsmsg(void);
void handleeoback(void);
void handletopic(void);
void handleremotewhois(void);
void handleeobmsg(void);
void handleaccountmsg(void);
void handleclearmodemsg();
/* End of serverhandlers.c */

/* usercommands.c */
void dowhois(long unum, char *tail);
void dostatus(long unum, char *tail);
void doopchan(long unum, char *tail);
void dokickcmd(long unum, char *tail);
void dojupe(long unum, char *tail);
void dooperhelp(long unum, char *tail);
void douserhelp(long unum, char *tail);
void dochancmd(long unum, char *tail);
void dochancmp(long unum, char *tail);
void doresync(long unum, char *tail);
void dosemiresync(long unum, char *tail);
void dodeopall(long unum, char *tail);
void doregexgline(long unum, char *tail);
void domfc(long unum, char *tail);
void dosettimecmd(long unum, char *tail);
void doserverlist(long unum, char *tail);
void dohello(long unum, char *tail);
void doauth(long unum, char *tail);
void dochangepwd(long unum, char *tail);
void dochangelev(long unum, char *tail);
void dodelusercmd(long unum, char *tail);
void donoticeset(long unum, char *tail);
void dolistauthed(long unum, char *tail);
void dosave(long unum, char *tail);
void doregexspew(long unum, char *tail);
void dornc(long unum, char *tail);
void dointtobase64(long unum, char *tail);
void dobase64toint(long unum, char *tail);
void dodie(long unum, char *tail);
void donoticemask(long unum, char *tail);
void dochanfix(long unum, char *tail);
void doshowregs(long unum, char *tail);
void dochanoplist(long unum, char *tail);
void dochanopstat(long unum, char *tail);
void doctcp(long unum, char *tail);
void dorehash(long unum, char *tail);
/* End of usercommands.c */

/* splitdb.c */
void addsplit(char *name);
void changesplitstatus(char *name, int newstatus);
int getsplitstatus(char *name);
void delsplit(char *name);
void clearsplits(void);
void splitpurge(void);
void dosplitlist(long unum, char *tail);
void dosplitdel(long unum, char *tail);
/* End of splitdb.c */

/* arrays.c */
void array_init(array *a, unsigned int memberlen);
void array_setlim1(array *a, unsigned int lim);
void array_setlim2(array *a, unsigned int lim);
unsigned long array_getfreeslot(array *a);
void array_delslot(array *a, unsigned long slotn);
void array_free(array *a);
/* End of arrays.c */

/* dynamic.c */
int loadandinitmodule(char *name);
int isloaded(char *name);
int removemodule(char *name);
void setmoduledesc(char *name, char *txt);
void registercommand(char *mod, char *name, void *func, int operonly, char *hlptxt);
void registercommand2(char *mod, char *name, void *func, int operonly, int minlev, char *hlptxt);
void registerserverhandler(char *mod, char *name, void *func);
void deregistercommand(char *name);
void deregisterserverhandler(char *name);
void deregisterserverhandler2(char *name, char *mod);
	/* ievents */
void registerinternalevent(char *mod, char *name, void *func);
void deregisterinternalevent(char *name, char *mod);
int dointernalevents(char *cmd, char *template, ...);
	/* fakeuser commands */
void regfakecmd(char *mod, char *nick, char *name, void *func, int operonly, int minlev, char *hlptxt);
void deregfakecmd(char *name, char *nick);
int dodynfakecmds(char *cmd, char *nick, long unum, char *tail, int oper, int authlev);
	/* fakeuser channel handlers */
void regfchanmsg(char *mod, char *nick, void *func);
void deregfchanmsg(char *nick, char *mod);
int dofchanmsg(char *nick, long unum, char *chan, char *template, ...);

int dodyncmds(char *cmd, long unum, char *tail, int oper, int authlev);
int doserverhandlers(char *cmd);
void dyncmdhelp(long unum, int oper, int authlev);
void doloadmod(long unum, char *tail);
void dounloadmod(long unum, char *tail);
void doreloadmod(long unum, char *tail);
void dolsmod(long unum, char *tail);
void printhelp(long unum, char *hlptxt);
/* End of dynamic.c */

/* trusts.c */
unsigned int tlhash(unsigned long IPv4);
unsigned int strthash(char *s);
unsigned int tgshash(unsigned long ID);
unsigned long istrusted(unsigned long IPv4);
void trustnewclient(char *ident, unsigned long IPv4);
void trustdelclient(char *ident, unsigned long IPv4);
trustedgroup * findtrustgroupbyID(unsigned long ID);
trustedgroup * findtrustgroupbyname(char *name);
unsigned long getfreetgid();
int destroytrustgroup(unsigned long id);
void trustgroupexpire(void);
unsigned long loadtrusts(void);
void savetrusts(void);
unsigned long loadtrustdeny(void);
void savetrustdeny(void);
void trustdenyexpire(void);
trustdeny * gettrustdeny(unsigned long IP);
void addtotrustedhost(char *ident, unsigned long id);
void updatetrustedhost(unsigned long IPv4, int modifier);
void delfromtrustedhost(char *ident, unsigned long id);
/* End of trusts.c */

/* md5.c */
void setmd5(userdata *up);
void changemd5(userdata *up);
/* End of md5.c */

/* chandb.c */
regchannel *createregchannel(char *channel);
regchannel *findregchannelifexists(char *channel);
void saveallchans();
void loadallchans();
/* End of chandb.c */

/* chancheck.c */
void checkallchans();
void checkchan(channel *c);
void showchan(long unum, channel *c, int all);
int doreop(channel *c);
void showregs(long unum, channel *c);
void showchansummary(long unum, channel *c);
/* End of chancheck.c */



/* End of important functions defining */

#endif
