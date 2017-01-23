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

// cited from "../../RobIoTs/CGNgine/libpromises/graph_defs.c"
#define GR_CONTAINS  3 // for membership
#define GR_FOLLOWS   2 // i.e. influenced by
#define GR_EXPRESSES 4 // naming/represents - do not use to label membership, only exterior promises
#define GR_NEAR      1 // approx like
// END

#define BASEDIR "/home/a10004/KMdata"

#define true 1
#define false 0
#define CGN_BUFSIZE 1024
#define MAX_ASSOC_ARRAY 128
#define CGN_ROOT 99

/*****************************************************************************/

extern char *optarg;
extern int optind, opterr, optopt;

// Global read-only policy

int ATYPE_OPT = CGN_ROOT;
int RECURSE_OPT = 10;
char *CONTEXT_OPT;

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

#include "../Percolators/associations.h"
#include "../Percolators/associations.c"

struct Concept
{
   char name[CGN_BUFSIZE];
   int importance_rank;
   struct Concept *next_concept;
   struct Concept *prev_concept;
   struct Concept *last_concept;
};

struct Story
{
   char name[CGN_BUFSIZE];
   struct Concept *next_concept;
   struct Story * next_story;
   struct Story * last_story;
}
    *STORIES = NULL;

enum hier
{
    h_context,
    h_association
};

/*****************************************************************************/

void FollowContextualizedAssociations(char *concept, struct Concept *this, int atype, int prevtype, int level);
void FollowUnqualifiedAssociation(int prevtype,int atype,int level, char *concept, char *nextconcept, struct Concept *next);
char *GetBestConceptAssociations(char *best_association, char *concept,int atype,char *nextconcept,struct Concept *next,char *ctx);
void FindContextAssociations(char *array[MAX_ASSOC_ARRAY], char *basedir, char* concept, int atype);
int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context);
struct Story *NewStory(char *name);
struct Concept *NewConcept(char *name, struct Concept *prev);
char *Indent(int level);
int PruneLoops(char *concept, struct Concept *this);
static int cmpassoc(const void *p1, const void *p2);
void SplitCompound(char *str, char *atoms[256], int len[256]);
void ShowMatchingConcepts(char *context);

/*****************************************************************************/

void main(int argc, char** argv)
{
 int level = 0;
 int optindex = 0, i;
 char c;
 char *subject = NULL, *context = NULL;
 struct Concept *this = NULL;
 
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

  if (CONTEXT_OPT)
     {
     printf("Found context: \"%s\"\n", CONTEXT_OPT);
     }
  else
     {
     CONTEXT_OPT = strdup("*");
     }

  if (subject)
     {
     printf("Found story subject: \"%s\"\n", subject);
     }
  else
     {
     if (strcmp(CONTEXT_OPT,"*") != 0)
        {
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

  this = NewConcept(subject, NULL);

 if (ATYPE_OPT != CGN_ROOT)
    {
    FollowContextualizedAssociations(subject, this, ATYPE_OPT, CGN_ROOT, level);
    }
 else
    {
    FollowContextualizedAssociations(subject, this, GR_FOLLOWS, CGN_ROOT, level);
    FollowContextualizedAssociations(subject, this, -GR_FOLLOWS, CGN_ROOT, level);

    FollowContextualizedAssociations(subject, this, GR_NEAR, CGN_ROOT, level);
    FollowContextualizedAssociations(subject, this, -GR_NEAR, CGN_ROOT, level);

    FollowContextualizedAssociations(subject, this, GR_CONTAINS, CGN_ROOT, level);
    FollowContextualizedAssociations(subject, this, -GR_CONTAINS, CGN_ROOT, level);

    // Has relevant properties...
    FollowContextualizedAssociations(subject, this, GR_EXPRESSES, CGN_ROOT, level);
    FollowContextualizedAssociations(subject, this, -GR_EXPRESSES, CGN_ROOT, level);
    }
}

/**********************************************************/

void ShowMatchingConcepts(char *context)

{
 int level = 0;
 struct Concept *this = NewConcept("all contexts", NULL);

 RECURSE_OPT = 1;

 FollowContextualizedAssociations("all contexts", this, GR_CONTAINS, CGN_ROOT, level);
}

/*****************************************************************************/

void FollowContextualizedAssociations(char *concept, struct Concept *this, int atype, int prevtype, int level)

{ int i, count = 0;
 char *array[MAX_ASSOC_ARRAY];

 FindContextAssociations(array,BASEDIR,concept,atype);
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    if (array[i] == NULL)
       {
       continue;
       }
   
    // Don't prune NOT or context here, leave it to FollowUnqualifiedAssociation()
   
    if (PruneLoops(array[i],this))
       {
       continue;
       }
    
    struct Concept *next = NewConcept(array[i], this); // next assoc

    // Similar attributes, but don't go back the way we came
    
    if (atype == -prevtype)
       {
       if (atype == GR_CONTAINS || atype == -GR_CONTAINS || atype == GR_EXPRESSES || atype == -GR_EXPRESSES)
          {
          count += 1;
          }
       
       if (count++ > 4)
          {
          printf("     ++ more ....\n");
          break; // Truncate litany of similar things
          }
       }

    // Explore next level, if context and everything matches

    if (level < RECURSE_OPT+1) // Arbitrary curb on length of stories
       {
       FollowUnqualifiedAssociation(prevtype, atype,level, concept, array[i], next);
       }

    free(array[i]);
    }
     
 if (level == 0)
    {
    printf("-------------------------------------------\n");
    }
}

/*****************************************************************************/

void FollowUnqualifiedAssociation(int prevtype,int atype,int level, char *concept, char *nextconcept, struct Concept *next)

{ char best_association[CGN_BUFSIZE];
 char approx_context[CGN_BUFSIZE];

 if (GetBestConceptAssociations(best_association,concept,atype,nextconcept,next,approx_context))
    {
    if ((atype == -prevtype) && (abs(atype) != GR_FOLLOWS))
       {
       printf ("%s and also note \"%s\" %s \"%s\" (in the context of %s)\n", Indent(level), concept, best_association, nextconcept,approx_context);
       return; 
       }
    else
       {
       printf ("%d:(%d) %s \"%s\" %s \"%s\" (in the context of %s)\n", level,atype, Indent(level), concept, best_association, nextconcept,approx_context);
       }

    if (ATYPE_OPT != CGN_ROOT)
       {
       FollowContextualizedAssociations(nextconcept, next, ATYPE_OPT, atype, level+1);
       FollowContextualizedAssociations(nextconcept, next, -ATYPE_OPT, atype, level+1);
       }
    else
       {
       FollowContextualizedAssociations(nextconcept, next, GR_EXPRESSES, atype, level+1);
       FollowContextualizedAssociations(nextconcept, next, -GR_EXPRESSES, atype, level+1);

       FollowContextualizedAssociations(nextconcept, next, GR_FOLLOWS, atype, level+1);
       FollowContextualizedAssociations(nextconcept, next, -GR_FOLLOWS, atype, level+1);
       
       FollowContextualizedAssociations(nextconcept, next, GR_NEAR, atype, level+1);
       FollowContextualizedAssociations(nextconcept, next, -GR_NEAR, atype, level+1);
       
       // Exploring next, policy only if previous connection was also quasi-transitive
       
       
       FollowContextualizedAssociations(nextconcept, next, GR_CONTAINS, atype, level+1);
       FollowContextualizedAssociations(nextconcept, next, -GR_CONTAINS, atype, level+1);
       }
    }
}

/*****************************************************************************/

char *Indent(int level)
{
 static char indent[256];
 int i;

 for (i = 0; (i < 2*level) && (i < 256); i++)
    {
    indent[i] = ' ';
    }

 indent[i] = '\0';
 return indent;
}

/*****************************************************************************/

char *GetBestConceptAssociations(char *best_association, char *concept,int atype,char *nextconcept,struct Concept *next,char *context)

{ int i;
  FILE *fin;
  char filename[CGN_BUFSIZE];
  char comment[CGN_BUFSIZE];
  LinkAssociation array[MAX_ASSOC_ARRAY];

  *context = '\0';
  
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",BASEDIR,concept,atype,nextconcept);

 if ((fin = fopen(filename, "r")) == NULL)
    {
    return NULL;
    }

 GetConceptAssociations(fin,array,MAX_ASSOC_ARRAY);
 fclose(fin);

 for (i = 0; (i < MAX_ASSOC_ARRAY); i++)
    {
    if (array[i].fwd[0] == '\0')
       {
       break;
       }
    
    if (!RelevantToCurrentContext(concept,array[i].fwd,nextconcept,array[i].context))
       {
       //printf("(excluding concept \"%s\" in context \"%s\")\n",nextconcept,CONTEXT_OPT);
       return NULL;
       }
    }

 //qsort(array,(size_t)i, sizeof(LinkAssociation *),cmpassoc);

 // The effect of this weight sorting will be small, but look for the best result(s)

 if (i > 1)
    {
    if (strcmp(array[0].fwd,array[i].fwd) == 0)
       {
       strcpy(best_association, "both ");
       strcat(best_association, array[0].fwd);
       }
    else
       {
       strcpy(best_association,array[0].fwd);
       }

    if (strcmp(array[0].context,"all contexts") != 0)
       {
       strcat(context, array[0].context);
       }
    }
 else
    {
    strcpy(best_association, array[0].fwd);

    if (strcmp(array[0].context,"all contexts") != 0)
       {
       strcat(context, array[0].context);
       }
    }
 
 for (i = 1; (i < MAX_ASSOC_ARRAY) && (array[i].fwd[0] != '\0'); i++)
    {
    if (strcmp(array[0].fwd,array[i].fwd) == 0)
       {
       continue;
       }
    strcat(best_association, " and ");
    strcat(best_association, array[i].fwd);

    if (strcmp(array[0].context,"all contexts") != 0)
       {
       strcat(context, array[i].context);
       }
    }

 // WARN: not checking for overflow here ... very unlikely but ...

 return best_association;
}

/*****************************************************************************/

struct Story *NewStory(char *name)

{ struct Story *new;

 new = (struct Story *)malloc(sizeof(struct Story));

 strcpy(new->name,name);
 new->next_concept = NULL;
 new->next_story = NULL;
 return new;
}

/*****************************************************************************/

struct Concept *NewConcept(char *name, struct Concept *prev)

{ struct Concept *new;

 new = (struct Concept *)malloc(sizeof(struct Concept));
  
 if (name)
    {
    strncpy(new->name,name,CGN_BUFSIZE);
    }
 else
    {
    new->name[0] = '\0';
    }

 if (prev)
    {
    prev->next_concept = new;
    }

 new->importance_rank = 0;
 new->prev_concept = prev;

 // Association assoc only applies if next_concept != NULL;

 return new;
}

/*****************************************************************************/

int PruneLoops(char *concept, struct Concept *this)

{ struct Concept *prev;

 // Is this concept already a part of the story?
 
 for (prev = this->prev_concept; prev != NULL; prev = prev->prev_concept)
    {
    if (strcmp(concept, prev->name) == 0)
       {
       return true;
       }
    }

 return false;
}


/*************************************************************/

void FindContextAssociations(char *array[MAX_ASSOC_ARRAY], char *basedir, char* concept, int atype)
{
 char filename[CGN_BUFSIZE];
 DIR *dirh_context,*dirh_assocs;
 struct dirent *dirp_c,*dirp_a;
 int count;

 for (count = 0; count < MAX_ASSOC_ARRAY; count++)
    {
    array[count] = NULL;
    }

 count = 0;
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",basedir,concept,atype);

 if ((dirh_assocs = opendir(filename)) == NULL)
    {
    return; // Fail silently
    }
    
 for (dirp_a = readdir(dirh_assocs); dirp_a != NULL; dirp_a = readdir(dirh_assocs))
    {
    if (dirp_a->d_name[0] == '.')
       {
       continue;
       }
    
       // prune already if possible to eliminate pathways
       
    array[count] = strdup(dirp_a->d_name);

    if (++count > MAX_ASSOC_ARRAY)
       {
       printf("Ran out of slots...\n");
       return;
       }
    }
 
 closedir(dirh_assocs);
}

/**********************************************************/

static int cmpassoc(const void *p1, const void *p2)
{
 return (((LinkAssociation *)p1)->weight > ((LinkAssociation *)p2)->weight);
}

/**********************************************************/

int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context)

{ int not = false;
 char *then[256] = {NULL};
 char *now[256] = {NULL};
 int tlen[256] = {0};
 int nlen[256] = {0};

// We need a VERY good reason to actually EXCLUDE a path, because it could become relevant to lateral thinking
// unless we have no subject, in which case CONTEXT is context hub and rules are different

 if (strcmp(context,"all contexts") == 0)
    {
    SplitCompound(nextconcept,then,tlen);     // Look at the learned relevance
    SplitCompound(CONTEXT_OPT,now,nlen); // Look at the current cognitive context
    
    if (Overlap(now,then,nlen,tlen))
       {
       return true;
       }

    return false;
    }

// Get current search contexts and see whether strings seem compatible by some measure
// Want to know if there are common genes in these context strings, leading to significant overlap
 
 if (strcmp(CONTEXT_OPT,"*") == 0)
    {
    return true;
    }
 
 if (strncmp(assoc,"NOT",3) == 0)
    {
    not = true;
    }

  // -c context is in CONTEXT

 SplitCompound(context,then,tlen);     // Look at the learned relevance
 SplitCompound(CONTEXT_OPT,now,nlen); // Look at the current cognitive context

 if (Overlap(now,then,nlen,tlen))
    {
    if (not)
       {
       return false;
       }
    }
 
 return true;
}

/**********************************************************/

void SplitCompound(char *str, char *atoms[256], int len[256])

// Split compound name into atoms that can permit a partial overlap match
    
{ char *sp;
 int between_words = true;
 int atom = 0;
 int new_word, end_word, in_word;
 int this_len = 0;

 for (sp = str; *sp != '\0'; sp++)
    {
    new_word = between_words && isalnum(*sp); 
    end_word = !between_words && !isalnum(*sp);
    in_word = !between_words && isalnum(*sp); 
        
    if (new_word)
       {
       atoms[atom] = sp;
       between_words = false;
       this_len = 1;
       }

    if (end_word)
       {
       len[atom] = this_len;
       between_words = true;

       //printf("WORD(%s,%d)\n", atoms[atom],len[atom]);
       
       if (++atom >= 256)
          {
          printf("Compound context overflow\n");
          return;
          }
       }
    
    if (in_word)
       {
       this_len++;
       }
    }

 len[atom] = this_len;
 //printf("WORD(%s,%d)\n", atoms[atom],len[atom]);
}

/**********************************************************/

int Overlap(char *atom1[256],char *atom2[256], int len1[256], int len2[256])

{ int i,j,score = 0;
 int min;

 for (i = 0; i < 256 && atom1[i]; i++)
    {
    for (j = 0; j < 256 && atom2[j]; j++)
       {
       min = (len1[i] < len2[j]) ? len1[i] : len2[j];

       if (strncmp(atom1[i],atom2[j],min) == 0)
          {
          score++;
          }
       }
    }

 return score;
}

