/*
Xevres (based on Operservice 2)
(C) Michael Meier 2000-2001 - released under GPL
-----------------------------------------------------------------------------
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
------------------------------------------------------------------------------
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
#include "mysql_version.h"
#include "mysql.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"

/* subnetlist-trust functions */
/* OBSOLETED - not needed anymore. The only function remaining is loadtrustlist,
   which gives a warning when there are still old subnettrusts in the database.
*/

void loadtrustlist() {
  int res;
  MYSQL_RES *myres; MYSQL_ROW myrow;
  res=mysql_query(&sqlcon,"SELECT COUNT(*) FROM sntrustlist");
  if (res!=0) {
    return;
  }
  myres=mysql_store_result(&sqlcon);
  if (mysql_num_fields(myres)!=1) {
    mysql_free_result(myres);
    return;
  }
  while ((myrow=mysql_fetch_row(myres))) {
    if (strtol(myrow[0],NULL,10)>0) {
      putlog("WARNING: Old subnettrusts detected in the database.");
      putlog("Those can NOT be automatically imported.");
      putlog("drop table sntrustlist from the database to get rid of this warning.");
    }
  }
  mysql_free_result(myres);
}

/* End of subnetlist-trust-functions */
