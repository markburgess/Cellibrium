/*****************************************************************************/
/*                                                                           */
/* File: associations.h                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:08 2017                                         */
/*                                                                           */
/*****************************************************************************/

#define MAX_ASSOC_ARRAY 1024

typedef struct
{
   char *fwd;
   char *bwd;
   char *concept;
   char *context;
   int relevance;
   time_t lastseen;
   double weight;
   
} LinkAssociation;

void InitializeAssociations(LinkAssociation *array);
void DeleteAssociations(LinkAssociation *array);
void GetConceptAssociations(FILE *fin, char *concept, LinkAssociation *array,int maxentries);
void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2);
