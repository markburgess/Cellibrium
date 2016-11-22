/*****************************************************************************/
/*                                                                           */
/* File: stories-fs.c                                                        */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for exploring the graph knowledge representation
 // This marries/matches with Percolators/conceptualize-fs.c

 // gcc -o stories -g -std=c99 stories-fs.c

 // Usage example: ./stories "expectation value" 

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

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

void FollowAssociations(char *concept, struct Concept *this, int atype, int prevtype, int level);
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

 struct Concept *this = NewConcept(argv[1], NULL);

 if (argc == 3)
    {
    FollowAssociations(argv[1], this, atoi(argv[2]), CGN_ROOT, level);
    }
 else
    {
    // Causation and membership ...
    FollowAssociations(argv[1], this, -GR_CONTAINS, CGN_ROOT, level);
    FollowAssociations(argv[1], this, -GR_FOLLOWS, CGN_ROOT, level);

    // Inverse causation/membership
    FollowAssociations(argv[1], this, GR_CONTAINS, CGN_ROOT, level);
    FollowAssociations(argv[1], this, GR_FOLLOWS, CGN_ROOT, level);

    // A bit like ...
    FollowAssociations(argv[1], this, GR_NEAR, CGN_ROOT, level);
    FollowAssociations(argv[1], this, -GR_NEAR, CGN_ROOT, level);

    // Has relevant properties...
    FollowAssociations(argv[1], this, GR_EXPRESSES, CGN_ROOT, level);
    FollowAssociations(argv[1], this, -GR_EXPRESSES, CGN_ROOT, level);
    }
}

/*****************************************************************************/

void FollowAssociations(char *concept, struct Concept *this, int atype, int prevtype, int level)

{
 char filename[CGN_BUFSIZE];
 Association array[MAX_ASSOC_ARRAY];
 int i, done;
 time_t now = time(NULL);
 DIR *dirh;
 FILE *fin;
 struct dirent *dirp;

 InitializeAssociations(array);
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",BASEDIR,concept,atype);

 // printf("Looking for concept %s...\n", filename);
  
 if ((dirh = opendir(filename)) == NULL)
    {
    return;
    }

 //printf("opened concept %s...\n", filename);
 
 for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
    {
    if (dirp->d_name[0] == '.')
       {
       continue;
       }

    if (PruneLoops(dirp->d_name,this))
       {
       continue;
       }

    struct Concept *next = NewConcept(dirp->d_name, this);
    
    //printf("GOT related concept \"%s\"\n", dirp->d_name);

    Association array[MAX_ASSOC_ARRAY];

    snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",BASEDIR,concept,atype,dirp->d_name);

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
       if (atype == -prevtype)
          {
          printf ("(%d) %s and also note \"%s\" %s \"%s\"\n", atype, Indent(level), concept, array[i].fwd, dirp->d_name);
          }
       else
          {
          printf ("(%d) %s \"%s\" %s \"%s\"\n", atype, Indent(level), concept, array[i].fwd, dirp->d_name);
          }
       
       // Attributes of current are normally leaves adorning a story concept

       FollowAssociations(dirp->d_name, next, GR_EXPRESSES, atype, level+1);
       FollowAssociations(dirp->d_name, next, -GR_EXPRESSES, atype, level+1);

       if (atype == GR_EXPRESSES)
          {
          continue;
          }
       
       FollowAssociations(dirp->d_name, next, GR_NEAR, atype, level+1);
       FollowAssociations(dirp->d_name, next, -GR_NEAR, atype, level+1);
       
       // Exploring next, policy only if previous connection was also quasi-transitive
       
       FollowAssociations(dirp->d_name, next, GR_CONTAINS, atype, level+1);
       FollowAssociations(dirp->d_name, next, GR_FOLLOWS, atype, level+1);
       FollowAssociations(dirp->d_name, next, -GR_CONTAINS, atype, level+1);
       FollowAssociations(dirp->d_name, next, -GR_FOLLOWS, atype, level+1);
       
       printf("\n");
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
 for (int i = 0; i < MAX_ASSOC_ARRAY; i++)
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
       return true;
       }
    }

 return false;
}
