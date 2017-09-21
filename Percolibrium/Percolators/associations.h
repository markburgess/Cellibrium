/*****************************************************************************/
/*                                                                           */
/* File: associations.h                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:08 2017                                         */
/*                                                                           */
/*****************************************************************************/

#define MAX_ASSOC_ARRAY 1024
#include <openssl/evp.h>

typedef struct
{
   char *cdigest;
   char *fwd;
   char *bwd;      // unneeded?
   char *icontext;
   int relevance;
   time_t lastseen;
   double weight;
   
} LinkAssociation;

void InitializeAssociations(LinkAssociation *array);
void DeleteAssociations(LinkAssociation *array);
void GetConceptAssociations(FILE *fin, char *concept, LinkAssociation *array,int maxentries);
void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2);
void WriteConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries);

char *NameDigest(char *name, unsigned char digest[EVP_MAX_MD_SIZE + 1]);
char *GetConceptFromDigest(char *digeststr, char *name);



