/*****************************************************************************/
/*                                                                           */
/* File: stories-fs.c                                                        */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for exploring the graph knowledge representation
 // This marries/matches with Percolators/conceptualize-fs.c

 // gcc -o stories -g stories-fs.c

 // Usage example: ./stories "expectation value" 

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <getopt.h>

// cited from "../../RobIoTs/CGNgine/libpromises/graph_defs.c"
#define GR_CONTAINS  1 // for membership
#define GR_FOLLOWS   2 // i.e. influenced by
#define GR_EXPRESSES 3 // naming/represents - do not use to label membership, only exterior promises
#define GR_NEAR      4 // approx like
// END

#define BASEDIR "/home/a10004/KMdata"

#define true 1
#define false 0
#define CGN_BUFSIZE 256
#define MAX_ASSOC_ARRAY 128
#define CGN_ROOT 99

extern char *optarg;
extern int optind, opterr, optopt;

int ATYPE_OPT = CGN_ROOT;
int RECURSE_OPT = 10;

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
    "Context relevance string",
    "Association type 1-4",
    "Recursion depth",
    NULL
};

/*****************************************************************************/

struct Association
{
   char fwd[CGN_BUFSIZE];
   char bwd[CGN_BUFSIZE];
   time_t lastseen;
   double weight;
};

struct Concept
{
   char name[CGN_BUFSIZE];
   int importance_rank;
   struct Concept *next_concept;
   struct Concept *prev_concept;
   struct Association next_assoc;
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

void FollowContextualizedAssociations(char *context, char *concept, struct Concept *this, int atype, int prevtype, int level);
void FollowUnqualifiedAssociation(int prevtype,int atype,int level, char *context, char *concept, char *nextconcept, struct Concept *next);
void InitializeAssociations(struct Association *array);
char *GetBestConceptAssociations(char *best_association, char *concept,char *context,int atype,char *nextconcept);
void UpdateConceptAssociations(FILE *fin, struct Association *array,int maxentries);
void PrioritizeContextAssociations(char *array[MAX_ASSOC_ARRAY][2], char *basedir, char* concept, int atype);

struct Story *NewStory(char *name);
struct Concept *NewConcept(char *name, struct Concept *prev);
char *Indent(int level);
int PruneLoops(char *concept, struct Concept *this);
static int cmpassoc(const void *p1, const void *p2);

/*****************************************************************************/

void main(int argc, char** argv)
{
 int level = 0;
 extern char *optarg;
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
           subject = strdup(optarg);
           break;

       case 'c':
           context = strdup(optarg);
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

  if (subject)
     {
     printf("Found story subject: \"%s\"\n", subject);
     }
  else
     {
     printf("You need to provide a subject for the story (option -s before the string)\n");
     exit(1);
     }

  if (context)
     {
     printf("Found context: \"%s\"\n", context);
     }
  else
     {
     context = strdup("*");
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
    FollowContextualizedAssociations(context, subject, this, ATYPE_OPT, CGN_ROOT, level);
    }
 else
    {
    FollowContextualizedAssociations(context, subject, this, GR_FOLLOWS, CGN_ROOT, level);
    FollowContextualizedAssociations(context, subject, this, -GR_FOLLOWS, CGN_ROOT, level);

    FollowContextualizedAssociations(context, subject, this, GR_NEAR, CGN_ROOT, level);
    FollowContextualizedAssociations(context, subject, this, -GR_NEAR, CGN_ROOT, level);

    FollowContextualizedAssociations(context, subject, this, GR_CONTAINS, CGN_ROOT, level);
    FollowContextualizedAssociations(context, subject, this, -GR_CONTAINS, CGN_ROOT, level);

    // Has relevant properties...
    FollowContextualizedAssociations(context, subject, this, GR_EXPRESSES, CGN_ROOT, level);
    FollowContextualizedAssociations(context, subject, this, -GR_EXPRESSES, CGN_ROOT, level);
    }
}

/*****************************************************************************/

void FollowContextualizedAssociations(char *context, char *concept, struct Concept *this, int atype, int prevtype, int level)

{ int i, count = 0;
 char *array[MAX_ASSOC_ARRAY][2];

 PrioritizeContextAssociations(array,BASEDIR,concept,atype);

 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    if (array[i][h_context] == NULL)
       {
       continue;
       }
    
    if (PruneLoops(array[i][h_association],this)) // next assoc
       {
       continue;
       }
    
    struct Concept *next = NewConcept(array[i][h_association], this); // next assoc
    
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
    
    if (level < RECURSE_OPT+1) // Arbitrary curb on length of stories
       {
       FollowUnqualifiedAssociation(prevtype, atype,level, array[i][h_context], concept, array[i][h_association], next);
       }
    else
       {
       //printf("#trunc#");
       }


    free(array[i][0]);
    free(array[i][1]);
    }
     
 if (level == 0)
    {
    printf("-------------------------------------------\n");
    }
}

/*****************************************************************************/

void FollowUnqualifiedAssociation(int prevtype,int atype,int level, char *context, char *concept, char *nextconcept, struct Concept *next)

{ char best_association[CGN_BUFSIZE];

 if (GetBestConceptAssociations(best_association,concept,context,atype,nextconcept))
    {
    if ((atype == -prevtype) && (abs(atype) != GR_FOLLOWS))
       {
       printf ("%s and also note \"%s\" %s \"%s\" in the context of \"%s\"\n", Indent(level), concept, best_association, nextconcept,context);
       return; 
       }
    else
       {
       if (strcmp(context,"*") == 0)
          {
          printf ("%d:(%d) %s Generally, \"%s\" %s \"%s\"\n", level,atype, Indent(level), concept, best_association, nextconcept);
          }
       else
          {
          printf ("%d:(%d) %s In the context of %s, \"%s\" %s \"%s\"\n", level,atype, Indent(level), context, concept, best_association, nextconcept);
          }
       }

    if (ATYPE_OPT != CGN_ROOT)
       {
       FollowContextualizedAssociations(context,nextconcept, next, ATYPE_OPT, atype, level+1);
       FollowContextualizedAssociations(context,nextconcept, next, -ATYPE_OPT, atype, level+1);
       }
    else
       {
       FollowContextualizedAssociations(context,nextconcept, next, GR_FOLLOWS, atype, level+1);
       FollowContextualizedAssociations(context,nextconcept, next, -GR_FOLLOWS, atype, level+1);
       
       FollowContextualizedAssociations(context,nextconcept, next, GR_NEAR, atype, level+1);
       FollowContextualizedAssociations(context,nextconcept, next, -GR_NEAR, atype, level+1);
       
       // Exploring next, policy only if previous connection was also quasi-transitive
       
       // Attributes of current are normally leaves adorning a story concept
       
       FollowContextualizedAssociations(context,nextconcept, next, GR_EXPRESSES, atype, level+1);
       FollowContextualizedAssociations(context,nextconcept, next, -GR_EXPRESSES, atype, level+1);
       
       FollowContextualizedAssociations(context,nextconcept, next, GR_CONTAINS, atype, level+1);
       FollowContextualizedAssociations(context,nextconcept, next, -GR_CONTAINS, atype, level+1);
       }
    }
}

/*****************************************************************************/

void InitializeAssociations(struct Association *array)
{
 int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    array[i].fwd[0] = '\0';
    array[i].bwd[0] = '\0';
    array[i].weight = 0;
    array[i].lastseen = 0;
    }
}

/*****************************************************************************/

char *Indent(int level)
{
 static char indent[256];
 int i;

 for (i = 0; (i < 3*level) && (i < 256); i++)
    {
    indent[i] = ' ';
    }

 indent[i] = '\0';
 return indent;
}

/*****************************************************************************/

char *GetBestConceptAssociations(char *best_association, char *concept,char *context,int atype,char *nextconcept)

{ int i;
 struct Association array[MAX_ASSOC_ARRAY];
 FILE *fin;
 char filename[CGN_BUFSIZE];

 snprintf(filename,CGN_BUFSIZE,"%s/%s/%s/%d/%s",BASEDIR,concept,context,atype,nextconcept);

 if ((fin = fopen(filename, "r")) != NULL)
    {
    InitializeAssociations(array);
    }
 else
    {
    return NULL;
    }

/* Here we only pick up the possible numerous name
 aliases/interpretations and weights of the association types, because
 the spacetime TYPE has already been used and followed, so we are
 looking for the maximum weighted interpretation of a single type of
 connection to a single concept
*/
     
 for (i = 0; (i < MAX_ASSOC_ARRAY) && !feof(fin); i++)
    {
    fscanf(fin, "(%[^,],%[^,],%ld,%lf)\n",array[i].fwd,array[i].bwd,&(array[i].lastseen),&(array[i].weight));
    }

  fclose(fin);

 // The effect of this weight sorting will be small, but look for the best result(s)

 qsort(array,(size_t)i, sizeof(struct Association *),cmpassoc);

 strcpy(best_association, array[0].fwd);
 
 for (i = 1; (i < MAX_ASSOC_ARRAY) && (array[i].fwd[0] != '\0'); i++)
    {
    strcat(best_association, " and ");
    strcat(best_association, array[0].fwd);
    }

 // WARN: not checking for overflow here ... very unlikely but ...

 return best_association;
}

/*****************************************************************************/

void UpdateConceptAssociations(FILE *fin, struct Association *array,int maxentries)

{ int i;

 for (i = 0; (i < maxentries) && (array[i].fwd[0] != '\0'); i++)
    {
    fprintf(fin, "(%s,%s,%ld,%lf)\n",array[i].fwd,array[i].bwd,array[i].lastseen,array[i].weight);
    }
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

void PrioritizeContextAssociations(char *array[MAX_ASSOC_ARRAY][2], char *basedir, char* concept, int atype)
{
 char filename[CGN_BUFSIZE];
 DIR *dirh_context,*dirh_assocs;
 struct dirent *dirp_c,*dirp_a;
 int count;

 for (count = 0; count < MAX_ASSOC_ARRAY; count++)
    {
    array[count][0] = NULL;
    array[count][1] = NULL;
    }

 count = 0;
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s",basedir,concept);

 if ((dirh_context = opendir(filename)) == NULL)
    {
    exit(0);
    }
 
 for (dirp_c = readdir(dirh_context); dirp_c != NULL; dirp_c = readdir(dirh_context))
    {
    if (dirp_c->d_name[0] == '.')
       {
       continue;
       }
    snprintf(filename,CGN_BUFSIZE,"%s/%s/%s/%d",basedir,concept,dirp_c->d_name,atype);

    if ((dirh_assocs = opendir(filename)) == NULL)
       {
       continue;
       }
    
    for (dirp_a = readdir(dirh_assocs); dirp_a != NULL; dirp_a = readdir(dirh_assocs))
       {
       if (dirp_a->d_name[0] == '.')
          {
          continue;
          }

       // prune already if possible to eliminate pathways
       
       array[count][0] = strdup(dirp_c->d_name);
       array[count][1] = strdup(dirp_a->d_name);

       //printf("Setting a[%d][0] = %s, a[%d][1] = %s\n ", count, array[count][0], count, array[count][1]);

       if (++count > MAX_ASSOC_ARRAY)
          {
          printf("Ran out of slots...\n");
          return;
          }
       }
    
    closedir(dirh_assocs);
    }
 
 closedir(dirh_context);

 //sort by NOT, context, and weight

}

/**********************************************************/

static int cmpassoc(const void *p1, const void *p2)
{
 return  (((struct Association *)p1)->weight > ((struct Association *)p2)->weight);
}


