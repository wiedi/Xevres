
/* Little Module for O to add a command to list all idents on a trustgroup which are exceeding their limit. */
/* (C) by Kickchon */

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

#define MODNAM "exceedingtg"

void dolistexceedingusers(long unum, char *tail) {
	int res, i, yes; char tmps1[TMPSSIZE], tmps2[TMPSSIZE];
	trustedgroup *tg;	identcounter *ic;

	res = sscanf(tail, "%s %s", tmps1, tmps2);
	if (res!=2) {
		msgtouser(unum,"Syntax: listexceedingusers <name|#id>");
		return;
	}

	if (tmps2[0] == '#') {
		tg = findtrustgroupbyID(strtoul(&tmps2[1],NULL,10));
	} else {
		tg = findtrustgroupbyname(tmps2);
	}
	if (tg==NULL) { msgtouser(unum,"No such trustgroup"); return; }

	longtoduration(tmps2, tg->expires - getnettime());
	newmsgtouser(unum, "Trustgroup %s (#%lu) (expires in %s)", tg->name, tg->id, tmps2);
	newmsgtouser(unum, "   Created by: %s   Contact: %s",tg->creator,tg->contact);
	newmsgtouser(unum, "   Currently used: %d  Trusted for: %d  Max Connections per User: %d", tg->currentlyon, tg->trustedfor, tg->maxperident);
	msgtouser(unum,"----------------------------------------------------------------------------------");

	ic = (identcounter *) (tg->identcounts.content);
	yes = 0;
	for (i = 0; i<tg->identcounts.cursi;i++) {
		if ((ic[i].currenton > tg->maxperident) && (tg->maxperident != 0)) {
			newmsgtouser(unum, "%s @ trustgroup %s has excessive connections (%d / %d)", ic[i].ident, tg->name, ic[i].currenton, tg->maxperident);
			yes++;
		}
	}
	if (!yes) {
		msgtouser(unum,"No exceeding users in that trustgroup.");
	} else {
		newmsgtouser(unum, "---------------------------- End of List - %d Matches ----------------------------", yes);
	}
}

void exceedingtg_init(void) {
	setmoduledesc(MODNAM,"Adds the 'listexceedingusers' command.");
	registercommand2(MODNAM,"listexceedingusers",dolistexceedingusers, 1, 900,
		"listexceedingusers <name|#id>\tLists all idents that exceed there limit on the given trustgroup.");
}
void exceedingtg_cleanup(void) {
	deregistercommand("listexceedingusers");
}
