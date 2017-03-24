/*****************************************************************************/
/*                                                                           */
/* File: stories-fs2.c                                                       */
/*                                                                           */
/*****************************************************************************/

 // Alternative version using branching process to separate then collate stories
 // This marries/matches with Percolators/conceptualize-fs.c

 // gcc -o stories2 -g stories-fs2.c

 // Usage example: ./stories2 "expectation value" 

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
#define CGN_BUFSIZE 1024
#define MAX_WORD_SZ 256
#define MAX_CONTEXT 2048
#define MAX_STORIES 256
#define CGN_ROOT 99
#define MAX_STORY_LEN 16


#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"
#include "../Percolators/associations.h"
#include "../Percolators/associations.c"
#include "context.h"
#include "context.c"

/*****************************************************************************/

typedef struct
{
   LinkAssociation *episode[MAX_STORY_LEN];  // Can be shared
   int score;

} Story;

extern char *optarg;
extern int optind, opterr, optopt;

// Global read-only policy

int ATYPE_OPT = CGN_ROOT;
int RECURSE_OPT = 10;
char *CONTEXT_OPT;

// Global story list for collating and ranking all branches

Story ALLSTORIES[MAX_STORIES];
int STORY_COUNTER = 0;

/*****************************************************************************/

static const struct option OPTIONS[6] =
{
    {"help", no_argument, 0, 'h'},
    {"subject", required_argument, 0, 's'},
    {"context", required_argument, 0, 'c'},
    {"type", required_argument, 0, 't'},
    {"recurse", required_argument, 0, 'r'},
    {NULL, 0, 0, '\0'}
};

static const char *HINTS[6] =
{
    "Print the help message",
    "The subject of the story (initial condition)", 
    "Context relevance string (would be provided by a running short term history)",
    "Association type 1-4 (see paper III theory)",
    "Recursion depth",
    NULL
};

/*****************************************************************************/

void CopyStory(Story *to, Story *from);
void InitializeStory(Story *s);
void InitializeStories(Story all[MAX_STORIES]);
void SearchForContextualizedAssociations(char *concept, int atype, int prevtype, int level, Story this);
int FollowNextAssociation(int prevtype,int atype,int level,char *concept,LinkAssociation *assoc, Story this);
int GetBestAssoc(char *best_association, char *concept,int atype,char *nextconcept,char *context);
int RankAssociationsByContext(LinkAssociation array[MAX_ASSOC_ARRAY], char *basedir, char* concept, int atype);
int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context);
int ConceptAlreadyUsed(char *concept,Story *story);
char *Indent(int level);
void SplitCompound(char *str, char *atoms[MAX_CONTEXT]);
void ShowMatchingConcepts(char *context);
char *Abbr(int n);
int Overlap(char *set1, char *set2);
static int cmpassoc(const void *p1, const void *p2);
static int cmprel(const void *p1, const void *p2);
static int cmpst(const void *p1, const void *p2);

/*****************************************************************************/

void main(int argc, char** argv)
{
 int level = 0;
 int optindex = 0, i,j;
 char c;
 char *subject = NULL, *context = NULL;
 
  while ((c = getopt_long(argc, argv, "ht:s:c:r:", OPTIONS, &optindex)) != EOF)
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
           subject = strdup((const char *)optarg);
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

       default:
           printf("Unknown option %c\n", c);
           break;
       }
    }

  // validate input args

  printf("\nExploring stories in knowledge bank at %s\n",BASEDIR);
  
  AppendStateOfMind(CONTEXT_OPT,ALL_CONTEXTS,"");

  printf("Found starting context: \"%s\"\n", POSITIVE_CONTEXT);

  if (subject)
     {
     printf("Found story subject: \"%s\"\n\n", subject);
     }
  else
     {
     if (strcmp(CONTEXT_OPT,ALL_CONTEXTS) != 0)
        {
        printf("No subject provided, so searching/looking for possibly relevant contexts\n\n");
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

  Story thisstory;
  InitializeStory(&thisstory);
  InitializeStories(ALLSTORIES);

  // choose a search algorithm
  
  if (ATYPE_OPT != CGN_ROOT)
     {
     SearchForContextualizedAssociations(subject, ATYPE_OPT, CGN_ROOT, level, thisstory);
     }
  else
     {
     SearchForContextualizedAssociations(subject, GR_FOLLOWS, CGN_ROOT, level, thisstory);
     SearchForContextualizedAssociations(subject, -GR_FOLLOWS, CGN_ROOT, level, thisstory);
     
     SearchForContextualizedAssociations(subject, GR_NEAR, CGN_ROOT, level, thisstory);
     SearchForContextualizedAssociations(subject, -GR_NEAR, CGN_ROOT, level, thisstory);
     
     SearchForContextualizedAssociations(subject, GR_CONTAINS, CGN_ROOT, level, thisstory);
     SearchForContextualizedAssociations(subject, -GR_CONTAINS, CGN_ROOT, level, thisstory);
     
     SearchForContextualizedAssociations(subject, GR_EXPRESSES, CGN_ROOT, level, thisstory);
     SearchForContextualizedAssociations(subject, -GR_EXPRESSES, CGN_ROOT, level, thisstory);
     }
  
  printf("\n\n");

  // Collate all the stories and rank by relevance score
  
  qsort(ALLSTORIES,(size_t)STORY_COUNTER, sizeof(Story),cmpst);
  
  for (i = 0; (i < MAX_STORIES)&&(ALLSTORIES[i].episode[0] != NULL); i++)
     {
     if (ALLSTORIES[i].score > 0)
        {
        printf("\nSTORY rated %d %% relevance ---- for context: %s", ALLSTORIES[i].score, POSITIVE_CONTEXT);

        if (strlen(NEGATIVE_CONTEXT) > 2)
           {
           printf(" but not %s",NEGATIVE_CONTEXT);
           }
        
        printf("\n %s ...\n",subject);
        for (j = 0; (j < MAX_STORY_LEN)&&(ALLSTORIES[i].episode[j] != NULL); j++)
           {
           printf(" - which %s %s (in the context %s %d%%)\n",ALLSTORIES[i].episode[j]->fwd,ALLSTORIES[i].episode[j]->concept,ALLSTORIES[i].episode[j]->context, ALLSTORIES[i].episode[j]->relevance);
           }
        }
     }
}

/**********************************************************/

void ShowMatchingConcepts(char *context)

{
 int level = 0;
 Story thisstory;
 InitializeStory(&thisstory);
 RECURSE_OPT = 1;
 CONTEXT_OPT = strdup(context);
 SearchForContextualizedAssociations(ALL_CONTEXTS, GR_CONTEXT, CGN_ROOT, level,thisstory);
}

/*****************************************************************************/
/* L1                                                                        */
/*****************************************************************************/


// FindPathBranches

void SearchForContextualizedAssociations(char *concept, int atype, int prevtype, int level, Story thisstory)

{ int i, count = 0;
  LinkAssociation *array = malloc(sizeof(LinkAssociation)*MAX_ASSOC_ARRAY);
  const int threshold_for_relevance = 0;
  const int max_stories = 5;
  
 InitializeAssociations(array);
  
 if (!RankAssociationsByContext(array,BASEDIR,concept,atype))
    {
    return;
    }

 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd != NULL); i++)
    {
    if (array[i].relevance < threshold_for_relevance)
       {
       continue;
       }
    
    // Don't retrace the way we came, or similar (breadcrumbs)
    
    if (atype == -prevtype)
       {
       continue;
       }

    // Explore next level, if context and everything matches

    if ((level < RECURSE_OPT+1) && (level < MAX_STORY_LEN)) // Arbitrary curb on length of stories
       {
       if (!FollowNextAssociation(prevtype,atype,level,concept,&(array[i]),thisstory))
          {
          if (STORY_COUNTER < MAX_STORIES-1)
             {
             CopyStory(&(ALLSTORIES[STORY_COUNTER++]),&thisstory);
             }
          }

       if (count++ > max_stories)
          {
          //printf(" ++ more....\n");
          break;
          }
       }
    }
 // Don't DeleteAssociations(array), because we use the references to collate all stories;
}

/*****************************************************************************/

int ConceptAlreadyUsed(char *concept, Story *story)
{
 int i;

 for (i = 0; (i < MAX_STORY_LEN) && (story->episode[i] != NULL); i++)
    {
    if (strcmp(concept,story->episode[i]->concept) == 0)
       {
       return true;
       }
    }

 return false;
}

/*****************************************************************************/

int FollowNextAssociation(int prevtype,int atype,int level,char *concept,LinkAssociation *assoc, Story thisstory)

{ int relevance;
  const int dontwanttoseethis = 0;

  if (ConceptAlreadyUsed(assoc->concept,&thisstory))
     {
     return false;
     }

  // Append this path step in this private branch
  thisstory.episode[level] = assoc;

  AppendStateOfMind(assoc->context,"",""); // Do we want to think about NOT here?
  
  if (ATYPE_OPT != CGN_ROOT)
     {
     SearchForContextualizedAssociations(assoc->concept,ATYPE_OPT, atype, level+1, thisstory);
     SearchForContextualizedAssociations(assoc->concept,-ATYPE_OPT, atype, level+1, thisstory);
     }
  else
     {
     SearchForContextualizedAssociations(assoc->concept,GR_EXPRESSES, atype, level+1, thisstory);
     SearchForContextualizedAssociations(assoc->concept,-GR_EXPRESSES, atype, level+1, thisstory);

     SearchForContextualizedAssociations(assoc->concept,GR_FOLLOWS, atype, level+1, thisstory);
     SearchForContextualizedAssociations(assoc->concept,-GR_FOLLOWS, atype, level+1, thisstory);

     SearchForContextualizedAssociations(assoc->concept,GR_NEAR, atype, level+1, thisstory);
     SearchForContextualizedAssociations(assoc->concept,-GR_NEAR, atype, level+1, thisstory);
     
     SearchForContextualizedAssociations(assoc->concept,GR_CONTAINS, atype, level+1, thisstory);
     SearchForContextualizedAssociations(assoc->concept,-GR_CONTAINS, atype, level+1, thisstory);
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

int RankAssociationsByContext(LinkAssociation array[MAX_ASSOC_ARRAY], char *basedir, char* fromconcept, int atype)
{
 char filename[CGN_BUFSIZE];
 DIR *dirh_context,*dirh_assocs;
 struct dirent *dirp_c,*dirp_a;
 int count;
 int threshold = 0;
 count = 0;

 // Open the directory and read the possible outgoing links
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",basedir,fromconcept,atype);

 if ((dirh_assocs = opendir(filename)) == NULL)
    {
    return false; // Fail silently
    }
    
 for (dirp_a = readdir(dirh_assocs); dirp_a != NULL; dirp_a = readdir(dirh_assocs))
    {
    if (dirp_a->d_name[0] == '.')
       {
       continue;
       }

    // Don't go back to places we've already been... (simplistic . could use atime as deadtime lock)
    
    char *nextconcept = dirp_a->d_name;
       
    if (count >= MAX_ASSOC_ARRAY)
       {
       printf("Ran out of slots...\n");
       break;
       }

    char best_association[CGN_BUFSIZE];
    char relevance_context[CGN_BUFSIZE];

    best_association[0] = '\0';
    relevance_context[0] = '\0';
        
    array[count].relevance = GetBestAssoc(best_association,fromconcept,atype,nextconcept,relevance_context);

    if (array[count].relevance < threshold)
       {
       continue;
       }
    
    array[count].concept = strdup(nextconcept);
    array[count].context = strdup(relevance_context);
    array[count].fwd = strdup(best_association);
    count++;
    }

 qsort(array,(size_t)count, sizeof(LinkAssociation),cmprel);

 closedir(dirh_assocs);
 return true;
}

/*****************************************************************************/

int GetBestAssoc(char *best_association, char *fromconcept,int atype,char *nextconcept,char *relevance_context)

{ int i, relevance = 0;
  FILE *fin;
  char filename[CGN_BUFSIZE];
  char comment[CGN_BUFSIZE];
  LinkAssociation array[MAX_ASSOC_ARRAY];

 *relevance_context = '\0';
  
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",BASEDIR,fromconcept,atype,nextconcept);

 if ((fin = fopen(filename, "r")) == NULL)
    {
    return 0;
    }

 GetConceptAssociations(fin,fromconcept,array,MAX_ASSOC_ARRAY);
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

    strcat(relevance_context, array[0].context);
    }
 else
    {
    strcpy(best_association, array[0].fwd);
    strcat(relevance_context, array[0].context);
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
    strcat(relevance_context, array[i].context);
    }

 // WARN: not checking for overflow here ... very unlikely but ...

 relevance = RelevantToCurrentContext(fromconcept,best_association,nextconcept,relevance_context);
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

static int cmpst(const void *p1, const void *p2)
{
 Story *c1,*c2;

 c1 = ((Story *)p1);
 c2 = ((Story *)p2);

 return (c1->score  < c2->score);
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

void CopyStory(Story *to, Story *from)

{ int i;
 double sum = 0, num = 0;
 
 for (i = 0; (i < MAX_STORY_LEN) && (from->episode[i] != NULL); i++)
    {
    to->episode[i] = from->episode[i];
    sum += (double) from->episode[i]->relevance;
    num ++;
    }

 to->score = (int)(sum/num+0.5);
}

/**********************************************************/

void InitializeStory(Story *s)
{
 int j;
 
 s->score = 0;
 
 for (j = 0; j < MAX_STORY_LEN; j++)
    {
    s->episode[j] = NULL;
    }
}

/**********************************************************/

void InitializeStories(Story all[MAX_STORIES])
{
 int i,j;

 for (i = 0; i < MAX_STORIES; i++)
    {
    InitializeStory(&(all[i]));
    }
}
