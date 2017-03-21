/*****************************************************************************/
/*                                                                           */
/* File: associations.h                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:08 2017                                         */
/*                                                                           */
/*****************************************************************************/

#define MAX_ASSOC_ARRAY 1024
#define MAX_STORY_LEN 16

typedef struct
{
   char *concept;
   char *fwd;
   
   char *bwd;      // unneeded?

   char *context;

   int relevance;
   time_t lastseen;
   double weight;
   
} LinkAssociation;

void InitializeAssociations(LinkAssociation *array);
void DeleteAssociations(LinkAssociation *array);
void GetConceptAssociations(FILE *fin, char *concept, LinkAssociation *array,int maxentries);
void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2);

typedef struct
{
   LinkAssociation *episode[MAX_STORY_LEN];  // Can be shared

} Story;
