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

static const struct option OPTIONS[5] =
{
    {"help", no_argument, 0, 'h'},
    {"subject", required_argument, 0, 's'},
    {"context", required_argument, 0, 'c'},
    {"type", required_argument, 0, 't'},
    {NULL, 0, 0, '\0'}
};

static const char *HINTS[5] =
{
    "Print the help message",
    "The subject of the story (initial condition)", 
    "Context relevance string",
    "Association type 1-4",
    NULL
};

/*****************************************************************************/

typedef struct
{
   char fwd[CGN_BUFSIZE];
   char bwd[CGN_BUFSIZE];
   time_t lastseen;
   double weight;
   
} Association;

struct Concept
{
   char name[CGN_BUFSIZE];
   int importance_rank;
   struct Concept *next_concept;
   struct Concept *prev_concept;
   Association next_assoc;
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

/*****************************************************************************/

void FollowAssociations(char *context, char *concept, struct Concept *this, int atype, int prevtype, int level);
void InitializeAssociations(Association *array);
void GetConceptAssociations(FILE *fin, Association *array,int maxentries);
void UpdateConceptAssociations(FILE *fin, Association *array,int maxentries);

struct Story *NewStory(char *name);
struct Concept *NewConcept(char *name, struct Concept *prev);
char *Indent(int level);
int PruneLoops(char *concept, struct Concept *this);

/*****************************************************************************/

void main(int argc, char** argv)
{
 int level = 0;
 extern char *optarg;
 int optindex = 0, i;
 char c;
 char *subject = NULL, *context = NULL;
 int atype = 99;
 struct Concept *this = NULL;
 
  while ((c = getopt_long(argc, argv, "ht:s:c:", OPTIONS, &optindex)) != EOF)
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
           atype = atoi(optarg);
           break;

       default:
           printf("Unknown option %c\n", c);
           break;
       }
    }

  // validate input args

  if (subject)
     {
     printf("Found story subject: %s\n", subject);
     }
  else
     {
     printf("You need to provide a subject for the story (option -s before the string)\n");
     exit(1);
     }

  if (context)
     {
     printf("Found context: %s\n", context);
     }
  else
     {
     context = strdup("*");
     }

  if (atype != 99)
     {
     printf("Stories of type %d only\n", atype);
     }

  printf("\n\n");
  
  // off we go

  this = NewConcept(subject, NULL);

 if (atype != 99)
    {
    FollowAssociations(context, subject, this, atype, CGN_ROOT, level);
    }
 else
    {
    FollowAssociations(context, subject, this, GR_CONTAINS, CGN_ROOT, level);
    FollowAssociations(context, subject, this, -GR_CONTAINS, CGN_ROOT, level);

    FollowAssociations(context, subject, this, GR_FOLLOWS, CGN_ROOT, level);
    FollowAssociations(context, subject, this, -GR_FOLLOWS, CGN_ROOT, level);

    FollowAssociations(context, subject, this, GR_NEAR, CGN_ROOT, level);
    FollowAssociations(context, subject, this, -GR_NEAR, CGN_ROOT, level);

    // Has relevant properties...
    FollowAssociations(context, subject, this, GR_EXPRESSES, CGN_ROOT, level);
    FollowAssociations(context, subject, this, -GR_EXPRESSES, CGN_ROOT, level);
    }
}

/*****************************************************************************/

void FollowAssociations(char *context, char *concept, struct Concept *this, int atype, int prevtype, int level)

{
 char filename[CGN_BUFSIZE];
 Association array[MAX_ASSOC_ARRAY];
 int i, count, done;
 time_t now = time(NULL);
 DIR *dirh;
 FILE *fin;
 struct dirent *dirp;

 if (level > 20)
    {
    printf("!! Story truncated...at 20 levels\n");
    return;
    }
 
 InitializeAssociations(array);
 snprintf(filename,CGN_BUFSIZE,"%s/%s::%s/%d",BASEDIR,context,concept,atype);

 //printf("Looking for concept %s...\n", filename);
  
 if ((dirh = opendir(filename)) == NULL)
    {
    //printf("failed to open concept %s...\n", filename);
    return;
    }

 //printf("opened concept %s...\n", filename);

 count = 0;
 
 for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
    {
    if (dirp->d_name[0] == '.')
       {
       continue;
       }

    struct Concept *next = NewConcept(dirp->d_name, this);

    if (PruneLoops(dirp->d_name,this))
       {
       continue;
       }

    if (count++ > 4) // arbitrary limit
       {
       printf("  ++more ....\n");
       break;
       }
    
    char nextconcept[CGN_BUFSIZE];
    char nextcontext[CGN_BUFSIZE];

    nextconcept[0] = nextcontext[0] = '\0';

    sscanf(dirp->d_name, "%[^:]::%[^\n]",nextcontext,nextconcept);

    //printf("GOT related concept \"%s\" in the context of %s\n", nextconcept,nextcontext);
    
    Association array[MAX_ASSOC_ARRAY];

    // Try the direct approach first
    
    snprintf(filename,CGN_BUFSIZE,"%s/%s::%s/%d/%s",BASEDIR,context,concept,atype,dirp->d_name);

    if ((fin = fopen(filename, "r")) != NULL)
       {
       InitializeAssociations(array);
       GetConceptAssociations(fin,array,MAX_ASSOC_ARRAY);
       fclose(fin);
       }
    else
       {
       printf("Missing concept - %s\n", filename);
       continue;
       }
    
    for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd[0] != '\0'); i++)
       {
       // If we explore alternatives like where we came from, then limited value

       if (atype == -prevtype) // ALSO EXCLUDE THE NODE WE JUST CAME FROM
          {
          if (strcmp(concept, nextconcept) != 0)
             {
             printf ("%s and also note \"%s\" %s \"%s\"\n", Indent(level), concept, array[i].fwd, nextconcept);
             }
          continue; 
          }
       else
          {
          if (strcmp(concept, nextconcept) != 0)
             {
             printf ("(%d) %s In the context of %s, \"%s\" %s \"%s\" (in the context %s)\n", atype, Indent(level), context, concept, array[i].fwd, nextconcept, nextcontext);
             }
          }
       
       // Attributes of current are normally leaves adorning a story concept

       FollowAssociations(nextcontext,nextconcept, next, GR_EXPRESSES, atype, level+1);
       FollowAssociations(nextcontext,nextconcept, next, -GR_EXPRESSES, atype, level+1);
       
       FollowAssociations(nextcontext,nextconcept, next, GR_NEAR, atype, level+1);
       FollowAssociations(nextcontext,nextconcept, next, -GR_NEAR, atype, level+1);
       
       // Exploring next, policy only if previous connection was also quasi-transitive
       
       FollowAssociations(nextcontext,nextconcept, next, GR_CONTAINS, atype, level+1);
       FollowAssociations(nextcontext,nextconcept, next, GR_FOLLOWS, atype, level+1);

       FollowAssociations(nextcontext,nextconcept, next, -GR_CONTAINS, atype, level+1);
       FollowAssociations(nextcontext,nextconcept, next, -GR_FOLLOWS, atype, level+1);


       printf("\n");

       // Attributes mark highlight where we have reached (final destination) and don't propagate
       // So no need to go through all. This could come at the end, so it doesn't prune other avenues
       
       if (atype == GR_EXPRESSES) 
          {
          continue;
          }
       }
    }
 closedir(dirh);

 if (level == 0)
    {
    printf("-------------------------------------------\n");
    }
}

/*****************************************************************************/

void InitializeAssociations(Association *array)
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

void GetConceptAssociations(FILE *fin, Association *array,int maxentries)

{ int i;

 for (i = 0; (i < maxentries) && !feof(fin); i++)
    {
    array[i].fwd[0] = array[i].bwd[0] = '\0';
    array[i].weight = 0;
    array[i].lastseen = 0;
    
    fscanf(fin, "(%[^,],%[^,],%ld,%lf)\n",array[i].fwd,array[i].bwd,&(array[i].lastseen),&(array[i].weight));
    }
}

/*****************************************************************************/

void UpdateConceptAssociations(FILE *fin, Association *array,int maxentries)

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
       //printf("SKIPPING %s...%s\n", concept, prev->name);
       return true;
       }
    }

 return false;
}
