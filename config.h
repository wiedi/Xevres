#ifndef O_CONFIG_H

#define O_CONFIG_H

/* Do we have "mallinfo"? */
#define HAVE_MALLINFO

/* Include Doug Leas malloc? Only reocmmended for BSD because BSDs default
   malloc sucks really bad */
#undef USE_DOUGLEA_MALLOC

/* Are we on BSD? */
#undef WE_R_ON_BSD

/* Level of Debug Output - 0 through 99 */
#define DEBUGLEVEL 0

/* GLINE Resync type - define this for a 2.10.11 (asuka) network, undef it
   for lain */
#define NUMERICGLINERESYNC

#endif
