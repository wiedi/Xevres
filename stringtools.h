/* Stringtools - originally written for Operservice 2, but can of course be
   used for other purposes too. Released under the GNU General Public Licence.
*/

#ifndef STRINGTOOLS_H
#define STRINGTOOLS_H
// Some Settings

// How many Bytes must a string have to be considered "small" ?
// Strings smaller than this are put into an array, greater or equal than that
// are allocated "standalone"
#define SMALLSTRSIZE 30

// How many strings does an ARRAY of small strings contain?
#define SMALLARRSIZE 5

// Hashtablesize?
#define STRTHASHSIZE 60000

// End of settings

// Functions
void stringtoolsinit(void);
char * getastring(char *s);
void freeastring(char *s);
/* Stats function. Fills in the integer-Array supplied:
   [0] Hashtablesize
   [1] used entries in small strings
   [2] used entried in big strings
   [3] size of a smallstring array
   [4] average usage of smallstring arrays - percent*10 !
   [5] longest chain in small strings
   [6] Longest chain in big strings
   [7] How long a "small" string is
*/
void stringtoolstats(long *res);
// This function is NOT supplied here, it has to be supplied by whatever uses
// this tools!
unsigned int strthash(char *s);

#endif
