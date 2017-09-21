/*****************************************************************************/
/*                                                                           */
/* File: stories-fs.c                                                        */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for exploring the graph knowledge representation
 // This marries/matches with Percolators/conceptualize-fs.c

 // gcc -o stories -g stories-fs.c

 // Usage example: ./stories "expectation value" 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>

// Import standard link definitions

#define true 1
#define false 0
#define MAX_WORD_SZ 265
#define MAX_CONTEXT 265
#define CGN_ROOT 99

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"
#include "../Percolators/associations.h"
#include "../Percolators/associations.c"

/*****************************************************************************/

extern char *optarg;
extern int optind, opterr, optopt;

// Global read-only policy

int ATYPE_OPT = CGN_ROOT;
int RECURSE_OPT = 10;
int JSON = false;
char *CONTEXT_OPT;

char MANY_WORLDS_CONTEXT[CGN_BUFSIZE];
char VBASEDIR[256];
char **PRUNELIST;

/*****************************************************************************/

static const struct option OPTIONS[8] =
{
    {"help", no_argument, 0, 'h'},
    {"subject", required_argument, 0, 's'},
    {"context", required_argument, 0, 'c'},
    {"type", required_argument, 0, 't'},
    {"recurse", required_argument, 0, 'r'},
    {"prune", required_argument, 0, 'p'},
    {"json", no_argument, 0, 'j'},
    {NULL, 0, 0, '\0'}
};

static const char *HINTS[8] =
{
    "Print the help message",
    "The subject of the story (initial condition)", 
    "Context relevance string (would be provided by a running short term history)",
    "Association type 1-4 (see paper III theory)",
    "Recursion depth",
    "prune comma separated keywords",
    "JSON-like output",
    NULL
};

/*****************************************************************************/

void NewManyWorldsContext(char *cdigest, char *context);
void DeleteManyWorldsContext(void);
void SearchForContextualizedAssociations(char *cdigest, int atype, int prevtype, int level);
int FollowNextAssociation(int prevtype,int atype,int level,char *cdigest,LinkAssociation *assoc);
int GetBestAssoc(char *best_association, char *cdigest,int atype,char *nextcdigest,char *context);
int RankAssociationsByContext(LinkAssociation array[MAX_ASSOC_ARRAY], char *basedir, char* cdigest, int atype);
int RelevantToCurrentContext(char *cdigest,char *assoc,char *nextcdigest,char *context);
int CdigestAlreadyUsed(char *cdigest,int level);
void DeleteCdigest(char *cdigest);
char *Indent(int level);
void SplitCompound(char *str, char *atoms[MAX_CONTEXT]);
void ShowMatchingConcepts(char *context);
char *Abbr(int n);
int Overlap(char *set1, char *set2);
void StartReport(int level);
void EndReport(int level);
void SectionBanner(char *s);
void EndBanner(void);
void BuildPruneList(char *list);
int Prune(char *s);
static int cmpassoc(const void *p1, const void *p2);
static int cmprel(const void *p1, const void *p2);

/*****************************************************************************/

void main(int argc, char** argv)
{
 int level = 0;
 int optindex = 0, i;
 char c;
 char *subject = NULL, *sdigest,*context = NULL;
 unsigned char digest[EVP_MAX_MD_SIZE + 1];
 char buffer[CGN_BUFSIZE];
 
 OpenSSL_add_all_digests();
 
  while ((c = getopt_long(argc, argv, "jht:s:c:r:p:", OPTIONS, &optindex)) != EOF)
    {
    switch ((char) c)
       {
       case 'h':

           printf("Usage: %s [OPTION]...\n");
           
           printf("\nOPTIONS:\n");
           
           for (i = 0; OPTIONS[i].name != NULL; i++)
              {
              if (OPTIONS[i].has_arg)
                 {
                 printf("  --%-12s, -%c value - %s\n", OPTIONS[i].name, (char) OPTIONS[i].val, HINTS[i]);
                 }
              else
                 {
                 printf("  --%-12s, -%-7c - %s\n", OPTIONS[i].name, (char) OPTIONS[i].val, HINTS[i]);
                 }
              }
           exit(0);
           break;

       case 's':
           sscanf(optarg,"%[^\n]",buffer);
           sdigest = NameDigest(buffer,digest);
           subject = strdup(buffer);
           break;

       case 'c':
           CONTEXT_OPT = strdup((const char *)optarg);
           break;

       case 't':
           ATYPE_OPT = atoi(optarg);
           break;

       case 'r':
           RECURSE_OPT = atoi(optarg);
           break;

       case 'j':
           JSON = true;
           break;

       case 'p':
           BuildPruneList(optarg);
           break;
           
       default:
           printf("Unknown option %c\n", c);
           break;
       }
    }

  // validate input args

  snprintf(VBASEDIR,255,"%s-%d",BASEDIR,getuid());

  if (CONTEXT_OPT)
     {
     // Should we add the subject and first order connections to the context list?

     // What kind of an agent am I?
     // Whta kinds of things do I think about?
     }
  else
     {
     CONTEXT_OPT = strdup(ALL_CONTEXTS);
     }

  if (subject)
     {
     printf("=======================================================================\n");
     printf(" STORY TOPIC SEARCH ...  \"%s\" (%s)\n\n", subject,sdigest);
     printf(" Looking for stories from this concepts\n");
     printf("=======================================================================\n");
     }
  else
     {
     if (strcmp(CONTEXT_OPT,ALL_CONTEXTS) != 0)
        {
        NewManyWorldsContext(CONTEXT_OPT,CONTEXT_OPT);
        ShowMatchingConcepts(CONTEXT_OPT);
        return;
        }
     
     printf("You need to provide a subject for the story (option -s before the string)\n");
     exit(1);
     }

  if (ATYPE_OPT != 99)
     {
     printf("Stories of type %d only\n", ATYPE_OPT);
     }

  printf("\n\n");
  
  // off we go

  NewManyWorldsContext(sdigest,CONTEXT_OPT);
  ConceptAlreadyUsed(sdigest,0);
  
  if (ATYPE_OPT != CGN_ROOT)
     {
     SearchForContextualizedAssociations(sdigest, ATYPE_OPT, CGN_ROOT, level);
     }
  else
     {
     StartReport(level);
     //SectionBanner("=========== sequential, causal reasoning =======================\n\n");
     SearchForContextualizedAssociations(sdigest, GR_FOLLOWS, CGN_ROOT, level);
     SearchForContextualizedAssociations(sdigest, -GR_FOLLOWS, CGN_ROOT, level);
     //SectionBanner("=========== proximity reasoning =======================\n\n");
     SearchForContextualizedAssociations(sdigest, GR_NEAR, CGN_ROOT, level);
     SearchForContextualizedAssociations(sdigest, -GR_NEAR, CGN_ROOT, level);
     //SectionBanner("=========== boundary or enclosure reasoning =======================\n\n");
     SearchForContextualizedAssociations(sdigest, GR_CONTAINS, CGN_ROOT, level);
     SearchForContextualizedAssociations(sdigest, -GR_CONTAINS, CGN_ROOT, level);
     //SectionBanner("=========== property or promise based reasoning =======================\n\n");
     SearchForContextualizedAssociations(sdigest, GR_EXPRESSES, CGN_ROOT, level);
     SearchForContextualizedAssociations(sdigest, -GR_EXPRESSES, CGN_ROOT, level);
     EndReport(level);
     }
  
  printf("\n");
  DeleteManyWorldsContext();
}

/**********************************************************/

void NewManyWorldsContext(char *cdigest, char *context)
{
 char name[CGN_BUFSIZE];

 umask(0); 
 snprintf(MANY_WORLDS_CONTEXT,CGN_BUFSIZE,"/tmp/story_world_%s_%s_%d",cdigest,context,getuid());
 mkdir(MANY_WORLDS_CONTEXT,((mode_t)0755));
 DIR *dirh;
 struct dirent *dirp;
 
 if ((dirh = opendir(MANY_WORLDS_CONTEXT)) == NULL)
    {
    return;
    }

 for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
    {
    if (dirp->d_name[0] == '.')
       {
       continue;
       }

    snprintf(name,CGN_BUFSIZE,"%s/%s",MANY_WORLDS_CONTEXT,dirp->d_name);
    unlink(name);
    }
 
 closedir(dirh);
}
    
/**********************************************************/

void DeleteManyWorldsContext(void)
{
 char name[CGN_BUFSIZE];
 DIR *dirh;
 struct dirent *dirp;
 
 if ((dirh = opendir(MANY_WORLDS_CONTEXT)) == NULL)
    {
    return;
    }
 
 for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
    {
    if (dirp->d_name[0] == '.')
       {
       continue;
       }

    snprintf(name,CGN_BUFSIZE,"%s/%s",MANY_WORLDS_CONTEXT,dirp->d_name);
    unlink(name);
    }
 closedir(dirh);

 rmdir(MANY_WORLDS_CONTEXT);
}
    
/**********************************************************/

void ShowMatchingConcepts(char *context)

{
 char filename[CGN_BUFSIZE];
 DIR *dirh_context;
 struct dirent *dirp_c;
 int level = 0;

 CONTEXT_OPT = strdup(context);

 printf("=======================================================================\n");
 printf(" STORY TOPIC SEARCH ..\n");
 printf(" Looking for concepts to match \"%s\"...\n",context);
 printf("=======================================================================\n");
 
 snprintf(filename,CGN_BUFSIZE,"%s",VBASEDIR);

 if ((dirh_context = opendir(filename)) == NULL)
    {
    printf("Unable to open %s\n",filename);
    return; // Fail silently
    }

 for (dirp_c = readdir(dirh_context); dirp_c != NULL; dirp_c = readdir(dirh_context))
    {
    if (dirp_c->d_name[0] == '.' || Prune(dirp_c->d_name))
       {
       continue;
       }

    // Don't go back to places we've already been... (simplistic . could use atime as deadtime lock)

    char buffer[CGN_BUFSIZE];
    char *subject = GetConceptFromDigest(dirp_c->d_name,buffer);

    if (strstr(subject,context))
       {
       ConceptAlreadyUsed(dirp_c->d_name,0);
       
       SearchForContextualizedAssociations(dirp_c->d_name, GR_FOLLOWS, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, -GR_FOLLOWS, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, GR_NEAR, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, -GR_NEAR, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, GR_CONTAINS, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, -GR_CONTAINS, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, GR_EXPRESSES, CGN_ROOT, level);
       SearchForContextualizedAssociations(dirp_c->d_name, -GR_EXPRESSES, CGN_ROOT, level);
       }
    }
}

/*****************************************************************************/
/* L1                                                                        */
/*****************************************************************************/

void SearchForContextualizedAssociations(char *cdigest, int atype, int prevtype, int level)

{ int i, count = 0;
 char dg[CGN_BUFSIZE];
  LinkAssociation array[MAX_ASSOC_ARRAY];
  const int threshold_for_relevance = 0;
  const int max_stories = 10;

 InitializeAssociations(array);

 if (!RankAssociationsByContext(array,VBASEDIR,cdigest,atype))
    {
    return;
    }

 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd != NULL); i++)
    {
    if (array[i].relevance < threshold_for_relevance)
       {
       continue;
       }
    
    // Similar attributes, but don't go back the way we came

    if (atype == -prevtype)
       {
       char trunc[CGN_BUFSIZE];
       char digest[EVP_MAX_MD_SIZE + 1];
       
       snprintf(trunc,CGN_BUFSIZE," (%d) %s `%s'",atype,(atype > 0) ? GR_TYPES[abs(atype)][0]: GR_TYPES[abs(atype)][1] ,GetConceptFromDigest(array[i].cdigest,dg));
       if ((abs(atype) == GR_FOLLOWS) && !ConceptAlreadyUsed(array[i].cdigest,0))
          {
          printf("                   (%s)\n",trunc);
          }
       continue;
       }

    // Explore next level, if context and everything matches
    
    if (level == 0)
       {
       char buffer[CGN_BUFSIZE];
       char *subject = GetConceptFromDigest(cdigest,buffer);       
       printf("\nnew story) \"%s\" %s \"%s\"\n\n",subject,(atype > 0) ? GR_TYPES[abs(atype)][0]: GR_TYPES[abs(atype)][1] ,GetConceptFromDigest(array[i].cdigest,dg));
       }
    
    if (level < RECURSE_OPT+1) // Arbitrary curb on length of stories
       {
       if (!FollowNextAssociation(prevtype,atype,level,cdigest,&(array[i])))
          {
          //printf("          %s << End of unique story\n",Indent(level));
          }

       if (count++ > max_stories)
          {
          printf("          %s  ++ more similar....\n",Indent(level));
          break;
          }
       }
    }

// DeleteConcept(cdigest);
 DeleteAssociations(array);
}

/*****************************************************************************/

int ConceptAlreadyUsed(char *cdigest, int plevel)

{ FILE *fp;
  struct stat statbuf;
  char *sp,name[CGN_BUFSIZE];
  int hash = 0,pos = 1,level = -1;

  // filenames are only 255 while paths can be 4096, so we have to shorten long concepts
  // This is a simple approx hash

  for (sp = cdigest; *sp != '\0'; sp++, pos++)
     {
     hash += *sp * pos;
     }
  
 snprintf(name,CGN_BUFSIZE,"%s/%d",MANY_WORLDS_CONTEXT,hash);

 if ((fp = fopen(name,"r")) != NULL)
    {
    fscanf(fp, "%d", &level);
    fclose(fp);

    if ((plevel == 0) || (plevel > level))
       {
       return true;
       }
    }
 else if ((fp = fopen(name,"w")) != NULL)
    {
    fprintf(fp, "%d", plevel);
    fclose(fp);
    }
 else
    {
    printf("\nCouldn't trace steps by writing (%s),%d, len = %d\n\n",name,plevel,strlen(name));
    perror("fopen");
    }

 return false;
}

/*****************************************************************************/

void DeleteConcept(char *cdigest)

{ FILE *fp;
  struct stat statbuf;
  char name[CGN_BUFSIZE];
  int level = -1;

 snprintf(name,CGN_BUFSIZE,"%s/%s",MANY_WORLDS_CONTEXT,cdigest);
 unlink(name); 
}

/*****************************************************************************/

int FollowNextAssociation(int prevtype,int atype,int level,char *cdigest,LinkAssociation *assoc)

{ int relevance;
  const int dontwanttoseethis = 0;
  char cb1[CGN_BUFSIZE];
  char cb2[CGN_BUFSIZE];
  char cb3[CGN_BUFSIZE];

  if (ConceptAlreadyUsed(assoc->cdigest,level) || strcmp(cdigest,assoc->cdigest) == 0 || Prune(assoc->cdigest))
     {
     if (level > 0)
        {
        return false;
        }
     }

  if (assoc->relevance > dontwanttoseethis)
     {
     if ((atype == -prevtype) && (abs(atype) != GR_FOLLOWS)) // Don't double back
        {
        printf ("%s and also note \"%s\" %s \"%s\" (icontext %s)\n\n", Indent(level),GetConceptFromDigest(cdigest,cb1),assoc->fwd,GetConceptFromDigest(assoc->cdigest,cb2),assoc->icontext);
        //return; 
        }
     else
        {
        printf ("%d:%s) %s \"%s\" %s \"%s\" (icontext: %s - %d%%)\n\n", level,Abbr(atype), Indent(level),GetConceptFromDigest(cdigest,cb1),assoc->fwd,GetConceptFromDigest(assoc->cdigest,cb2),assoc->icontext,assoc->relevance);
        }
     }

  if (ATYPE_OPT != CGN_ROOT)
     {
     SearchForContextualizedAssociations(assoc->cdigest,ATYPE_OPT, atype, level+1);
     SearchForContextualizedAssociations(assoc->cdigest,-ATYPE_OPT, atype, level+1);
     }
  else
     {
     SearchForContextualizedAssociations(assoc->cdigest,GR_EXPRESSES, atype, level+1);
     SearchForContextualizedAssociations(assoc->cdigest,-GR_EXPRESSES, atype, level+1);

     SearchForContextualizedAssociations(assoc->cdigest,GR_FOLLOWS, atype, level+1);
     SearchForContextualizedAssociations(assoc->cdigest,-GR_FOLLOWS, atype, level+1);

     SearchForContextualizedAssociations(assoc->cdigest,GR_NEAR, atype, level+1);
     SearchForContextualizedAssociations(assoc->cdigest,-GR_NEAR, atype, level+1);
     
     // Exploring next, policy only if previous connection was also quasi-transitive
     SearchForContextualizedAssociations(assoc->cdigest,GR_CONTAINS, atype, level+1);
     SearchForContextualizedAssociations(assoc->cdigest,-GR_CONTAINS, atype, level+1);
     }

  if (level == 0)
     {
     printf("          %s [%d]---------------------------------------------------------------------------------\n",Indent(level),level);
     }
  return true;
}

/*****************************************************************************/

char *Indent(int level)
{
 static char indent[MAX_WORD_SZ];
 int i;

 for (i = 0; (i < 2*level) && (i < MAX_WORD_SZ); i++)
    {
    indent[i] = ' ';
    }

 indent[i] = '\0';
 return indent;
}

/*************************************************************/

int RankAssociationsByContext(LinkAssociation array[MAX_ASSOC_ARRAY], char *basedir, char* fromcdigest, int atype)
{
 char filename[CGN_BUFSIZE];
 DIR *dirh_context,*dirh_assocs;
 struct dirent *dirp_c,*dirp_a;
 int count;
 int threshold = 0;
 count = 0;

 // Open the directory and read the possible outgoing links
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",basedir,fromcdigest,atype);

 if ((dirh_assocs = opendir(filename)) == NULL)
    {
    return false; // Fail silently
    }

 for (dirp_a = readdir(dirh_assocs); dirp_a != NULL; dirp_a = readdir(dirh_assocs))
    {
    if (dirp_a->d_name[0] == '.' || Prune(dirp_a->d_name))
       {
       continue;
       }

    if (strcmp(fromcdigest,dirp_a->d_name) == 0)
       {
       continue; // can't avoid self-reference, but no need to mention it
       }

    // Don't go back to places we've already been... (simplistic . could use atime as deadtime lock)
    
    char *nextcdigest = dirp_a->d_name;
       
    if (count >= MAX_ASSOC_ARRAY)
       {
       printf("Ran out of slots...\n");
       break;
       }

    char best_association[CGN_BUFSIZE];
    char relevance_context[CGN_BUFSIZE];

    best_association[0] = '\0';
    relevance_context[0] = '\0';
        
    array[count].relevance = GetBestAssoc(best_association,fromcdigest,atype,nextcdigest,relevance_context);

    if (array[count].relevance < threshold)
       {
       continue;
       }

    array[count].cdigest = strdup(nextcdigest);
    array[count].icontext = strdup(relevance_context);
    array[count].fwd = strdup(best_association);
    array[count].bwd = strdup("n/a"); // Don't need this here
    count++;
    }

 //qsort(array,(size_t)count, sizeof(LinkAssociation),cmprel);

 closedir(dirh_assocs);
 return true;
}

/*****************************************************************************/

int GetBestAssoc(char *best_association, char *fromcdigest,int atype,char *nextcdigest,char *relevance_context)

{ int i, relevance = 0;
  FILE *fin;
  char filename[CGN_BUFSIZE];
  char comment[CGN_BUFSIZE];
  LinkAssociation array[MAX_ASSOC_ARRAY];

 *relevance_context = '\0';
  
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",VBASEDIR,fromcdigest,atype,nextcdigest);

 if ((fin = fopen(filename, "r")) == NULL)
    {
    return 0;
    }

 GetConceptAssociations(fin,fromcdigest,array,MAX_ASSOC_ARRAY);
 fclose(fin);

 if (array[0].fwd == NULL)
    {
    return 0;
    }

 for (i = 0; (i < MAX_ASSOC_ARRAY); i++)
    {
    if (array[i].fwd == NULL)
       {
       break;
       }
    }

 //qsort(array,(size_t)i, sizeof(LinkAssociation *),cmprel);

 // The effect of this weight sorting will be small, but look for the best result(s)

 if (--i > 0)
    {
    if (strcmp(array[0].fwd,array[i].fwd) != 0)
       {
       strcpy(best_association, "both ");
       strcat(best_association, array[0].fwd);
       }
    else
       {
       strcpy(best_association,array[0].fwd);
       }

    strcat(relevance_context, array[0].icontext);
    }
 else
    {
    strcpy(best_association, array[0].fwd);
    strcat(relevance_context, array[0].icontext);
    }
 
 for (i = 1; (i < MAX_ASSOC_ARRAY) && array[i].fwd; i++)
    {
    if (strcmp(array[0].fwd,array[i].fwd) == 0)
       {
       continue;
       }

    strcat(best_association, " and ");
    strcat(best_association, array[i].fwd);

    strcat(relevance_context," ");
    strcat(relevance_context, array[i].icontext);
    }

 // WARN: not checking for overflow here ... very unlikely but ...

 char dg1[CGN_BUFSIZE],dg2[CGN_BUFSIZE];
 
 relevance = RelevantToCurrentContext(GetConceptFromDigest(fromcdigest,dg1),best_association,GetConceptFromDigest(nextcdigest,dg2),relevance_context);
 DeleteAssociations(array);
 return relevance;
}

/**********************************************************/

static int cmpassoc(const void *p1, const void *p2)
{
 LinkAssociation *c1,*c2;

 c1 = ((LinkAssociation *)p1);
 c2 = ((LinkAssociation *)p2);
 
 return (c1->weight  > c2->weight);
}

/**********************************************************/

static int cmprel(const void *p1, const void *p2)
{
 LinkAssociation *c1,*c2;

 c1 = ((LinkAssociation *)p1);
 c2 = ((LinkAssociation *)p2);

 return (c1->relevance  < c2->relevance);
}

/**********************************************************/

int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context)

{ int not = false;
  int relevance = 0;

// We need a VERY good reason to actually EXCLUDE a path, because it could become relevant to lateral thinking
// unless we have no subject, in which case CONTEXT is context hub and rules are different

if (strcmp(CONTEXT_OPT,ALL_CONTEXTS) == 0) // if we are looking for unknown subject
   {
   relevance += 80;
   }

if (strncmp(assoc,"NOT",3) == 0)
   {
   not = true;
   }

// If the next concept is related to the search concept (must exist exactly), recursively
relevance += Overlap(concept,nextconcept);

 // If the next subject contains the search context 
relevance += Overlap(CONTEXT_OPT,nextconcept);

// Finally, if the current context overlaps with the learnt context
relevance += Overlap(CONTEXT_OPT,context);

if (relevance > 0)
   {
   // Check this, handle NOT
   if (not)
      {
      return 0;
      }
   
   return relevance;
   }

return 0;
}

/**********************************************************/

int Overlap(char *intended, char *actual)

// In looking for conceptual overlap, the two contexts are not equal.
// The first is the intended one from the state of mind of the agent,
// the latter is the recorded actual context acquired during learning.
    
{ int i,j,k;
  double s = 0, t = 0, score = 0, total = 0;
  int end = false;
  double percent;
  char *atomI[MAX_CONTEXT] = {NULL};
  char *atomA[MAX_CONTEXT] = {NULL};

  const double frag_cutoff_threshold = 3; // min frag match

SplitCompound(intended,atomI); // Look at the learned relevance
SplitCompound(actual,atomA);   // Look at the current cognitive context

for (i = 0; i < MAX_CONTEXT && atomI[i]; i++)
   {
   for (j = 0; j < MAX_CONTEXT && atomA[j]; j++)
      {
      if (atomI[i] == NULL || atomA[j] == NULL)
         {
         continue;
         }

      s = t = 0;
      
      for (k = 0; k < MAX_WORD_SZ; k++)
         {
         if (atomI[i][k] == '\0') // intended is a substring of actual
            {
            //printf("match %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomA[j]+k);
            break;
            }
         
         if (atomA[j][k] == '\0') // actual is a substring of intended
            {
            //printf("match %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomI[i]+k);
            break;
            }

         if (atomI[i][k] != atomA[j][k]) // terminate on mismatch
            {
            //printf("mismatch %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomA[j]+k);
            break;
            }
         else
            {
            s++;
            t++;
            }
         }

      if (s > frag_cutoff_threshold)
         {
         score += s;
         total += t;
         }
      else
         {
         // If there is no significant word overlap, then add the length of
         // the non-matching string to scale the fractional overlap
         total += strlen(atomI[i]);
         }
      }
   }

if (total > 0)
   {
   percent = score / total * 100.0;
   }
else
   {
   percent = 0;
   }

//printf("  - SCORE %f (%s,%s)\n",percent,intended,actual);

for (i = 0; i < MAX_CONTEXT; i++)
   {
   if (atomI[i])
      {
      free(atomI[i]);
      }
   if (atomA[i])
      {
      free(atomA[i]);
      }
   }

return (int)percent;
}

/**********************************************************/

void SplitCompound(char *str, char *atoms[MAX_CONTEXT])

// Split compound name into atoms that can permit a partial overlap match
    
{ char *sp = str;
  char word[255];
  int pos = 0;

 if (str == NULL)
    {
    return;
    }  
 
 while (*sp != '\0')
    {
    if (*sp == ' ' || *sp == ',')
       {
       sp++;
       continue;
       }
    
    word[0] = '\0';
    sscanf(sp,"%250[^ ,]",word);
    sp += strlen(word);

    if (pos < MAX_CONTEXT)
       {
       atoms[pos] = strdup(word);
       pos++;
       }
    }
}

/**********************************************************/

char *Abbr(int d)

{
 switch (d)
    {
    case GR_CONTAINS:
        return "cntains";
    case -GR_CONTAINS:
        return "cntaind";
    case GR_FOLLOWS:
        return "follows";
    case -GR_FOLLOWS:
        return "preceds";
    case GR_EXPRESSES:
        return "hasprop";
    case -GR_EXPRESSES:
        return "expr-by";
    case GR_NEAR:
    case -GR_NEAR:
        return "apprxnr";
    case GR_CONTEXT:
    case -GR_CONTEXT:
        return "context";

    default:
        return "???";
    }
}

/**********************************************************/

void StartReport(int level)
{
 if (JSON)
    {
    printf("%s{",Indent(level));
    }
}

/**********************************************************/

void EndReport(int level)
{
 if (JSON)
    {
    printf("%s}",Indent(level));
    }
}

/**********************************************************/

void SectionBanner(char *s)
{
 if (JSON)
    {
    }
 else
    {
    printf("%s",s);
    }
}

/**********************************************************/

void EndBanner(void)

{
}

/**********************************************************/

void BuildPruneList(char *s)
{
 char *sp, item[CGN_BUFSIZE];
 int i = 0, n = 2;

 for (sp = s; *sp != '\0'; sp++)
    {
    if (*sp == ',')
       {
       n++;
       }
    }

 PRUNELIST = (char **)malloc((sizeof(char *)*n));
 
 for (sp = s, i = 0; (sp < s+strlen(s)) && (i < n); sp += strlen(item)+1, i++)
    {
    item[0] = '\0';
    sscanf(sp,"%[^,\n]",item);

    if (*item == '\0')
       {
       PRUNELIST[i] = NULL;
       break;
       }
    else
       {
       PRUNELIST[i] = strdup(item);
       }
    }
 
for (i = 0; PRUNELIST[i] != NULL; i++)
   {
   printf(" - Prune search on strings containing `%s'\n",PRUNELIST[i]);
   }
}

/**********************************************************/

int Prune(char *s)
{
 int i;
 char buffer[CGN_BUFSIZE];

 if (!PRUNELIST)
    {
    return false;
    }
 
 for (i = 0; PRUNELIST[i] != NULL; i++)
    {
    if (strstr(GetConceptFromDigest(s,buffer),PRUNELIST[i]))
       {
       return true;
       }
    }
 return false;
}
