/* Stringtools - originally written for Operservice 2, but can of course be
   used for other purposes too. Released under the GNU General Public Licence.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "stringtools.h"

struct strtstr {
  int timesused;
  struct strtstr *next;
};

struct smallstr {
  int timesused[SMALLARRSIZE];
  char cont[SMALLARRSIZE][SMALLSTRSIZE];
  int elementsinuse;
  struct smallstr *next;
};

struct strtstr * strtstrs[STRTHASHSIZE];
struct smallstr * smallstrs[STRTHASHSIZE];
char onecharstrings[256][2];

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
void stringtoolstats(long *res) {
  struct strtstr *a;
  struct smallstr *b;
  int i, j; unsigned long long k, l;
  res[0]=STRTHASHSIZE;
  res[3]=SMALLARRSIZE;
  res[7]=SMALLSTRSIZE;
  res[1]=0; res[2]=0;
  res[5]=0; res[6]=0;
  k=0; l=0;
  for (i=0; i<STRTHASHSIZE; i++) {
    if (strtstrs[i]!=NULL) {
      res[2]++;
      a=strtstrs[i]; j=0;
      while (a!=NULL) { j++; a=a->next; }
      if (j>res[6]) { res[6]=j; }
    }
    if (smallstrs[i]!=NULL) {
      res[1]++;
      b=smallstrs[i]; j=0;
      while (b!=NULL) {
        j++;
        k+=SMALLARRSIZE;
        l+=b->elementsinuse;
        b=b->next;
      }
      if (j>res[5]) { res[5]=j; }
    }
  }
  if (k>0) {
    res[4]=(long)(l*1000/k);
  } else { res[4]=0; }
}

void stringtoolsinit(void) {
  int i;
  for (i=0;i<STRTHASHSIZE;i++) {
    strtstrs[i]=NULL;
    smallstrs[i]=NULL;
  }
  for (i=0;i<256;i++) {
    onecharstrings[i][0]=i;
    onecharstrings[i][1]=0;
  }
}

char * getastring(char *s) {
  int l; unsigned char uc; unsigned int h;
  l=strlen(s);
  if (l==0) { return &onecharstrings[0][0]; }
  if (l==1) {
    uc=(unsigned char)s[0];
    return &onecharstrings[uc][0];
  }
  h=strthash(s); // Everything that follows needs the hash
  if (l<SMALLSTRSIZE) {
    struct smallstr *myss; int i; struct smallstr *hasonefree;
    struct smallstr *lastmyss;
    hasonefree=NULL; lastmyss=NULL;
    myss=smallstrs[h];
    while (myss!=NULL) {
      for (i=0;i<SMALLARRSIZE;i++) {
        if (myss->timesused[i]>0) {
          if (strcmp(myss->cont[i],s)==0) {
            myss->timesused[i]++;
            return &(myss->cont[i][0]);
          }
        } else {
          if (hasonefree==NULL) { hasonefree=myss; }
        }
      }
      lastmyss=myss;
      myss=myss->next;
    }
    if (hasonefree==NULL) { // No free space, allocate new array
      hasonefree=(struct smallstr *)malloc(sizeof(struct smallstr));
      if (hasonefree==NULL) {
        printf("\nOut of memory in %s Line %d - Terminating\n",
          __FILE__ , __LINE__);
        exit(666);
      }
      hasonefree->next=NULL;
      hasonefree->elementsinuse=1;
      for (i=1;i<SMALLARRSIZE;i++) {
        hasonefree->timesused[i]=0;
      }
      hasonefree->timesused[0]=1;
      strcpy(hasonefree->cont[0],s);
      if (lastmyss==NULL) {
        smallstrs[h]=hasonefree;
      } else {
        lastmyss->next=hasonefree;
      }
      return &hasonefree->cont[0][0];
    } else {
      for (i=0;i<SMALLARRSIZE;i++) {
        if (hasonefree->timesused[i]==0) {
          strcpy(hasonefree->cont[i],s);
          hasonefree->timesused[i]++;
          hasonefree->elementsinuse++;
          return &hasonefree->cont[i][0];
        }
      }
      // This point MUST NOT be reached, else we have a serious memory corruption!
      printf("\nFATAL in %s Line %d: Memory Corruption, terminating\n",
        __FILE__ , __LINE__);
      exit(666);
      return NULL;
    }
  } else {
    struct strtstr * mys; struct strtstr * mys2;
    char *hp;
    mys=strtstrs[h]; mys2=NULL;
    while (mys!=NULL) {
      hp=(char *)((void *)mys+sizeof(struct strtstr));
      if (strcmp(hp,s)==0) {
        mys->timesused++;
        return hp;
      }
      mys2=mys;
      mys=mys->next;
    }
    mys=(struct strtstr *)malloc(sizeof(struct strtstr)+l+1);
    if (mys==NULL) {
      printf("\nOut of memory in %s Line %d - Terminating\n",
        __FILE__ , __LINE__);
      exit(666);
    }
    mys->timesused=1;
    mys->next=NULL;
    hp=(char *)((void *)mys+sizeof(struct strtstr));
    strcpy(hp,s);
    if (mys2==NULL) {
      strtstrs[h]=mys;
    } else {
      mys2->next=mys;
    }
    return hp;
  }
}

void freeastring(char *s) {
  int l; unsigned int h;
  l=strlen(s);
  if (l<=1) { return; }
  h=strthash(s);
  if (l<SMALLSTRSIZE) {
    int i; struct smallstr *myss; struct smallstr *myss2;
    myss=smallstrs[h]; myss2=NULL;
    while (myss!=NULL) {
      for (i=0;i<SMALLARRSIZE;i++) {
        if (myss->timesused[i]>0) {
          if (strcmp(myss->cont[i],s)==0) { /* Free the string, it's no longer needed */
            myss->timesused[i]--;
            if (myss->timesused[i]==0) {
              myss->elementsinuse--;
              myss->cont[i][0]='\0';
            }
            if (myss->elementsinuse==0) { /* Free the struct. More free mem, hooray! */
              if (myss2==NULL) {
                smallstrs[h]=myss->next;
              } else {
                myss2->next=myss->next;
              }
              free(myss);
            }
            return;
          }
        }
      }
      myss2=myss;
      myss=myss->next;
    }
    printf("\nFreeing nonexistant string in %s Line %d\n", __FILE__ , __LINE__);
  } else {
    struct strtstr * mys; struct strtstr * mys2;
    char *hp;
    mys=strtstrs[h]; mys2=NULL;
    while (mys!=NULL) {
      hp=(char *)((void *)mys+sizeof(struct strtstr));
      if (strcmp(hp,s)==0) {
        mys->timesused--;
        if (mys->timesused==0) { /* Free the string, it's no longer needed */
          if (mys2==NULL) {
            strtstrs[h]=mys->next;
          } else {
            mys2->next=mys->next;
          }
          free(mys);
        }
        return;
      }
      mys2=mys;
      mys=mys->next;
    }
    printf("\nFreeing nonexistant string in %s Line %d\n", __FILE__ , __LINE__);
  }
}
