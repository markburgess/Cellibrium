/*****************************************************************************/
/*                                                                           */
/* File: associations.c                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:02 2017                                         */
/*                                                                           */
/*****************************************************************************/

#define CF_MD5_LEN 16
extern char VBASEDIR[256];

/*****************************************************************************/

char *NameDigest(char *name, unsigned char digest[EVP_MAX_MD_SIZE + 1])
{
const EVP_MD *md = EVP_get_digestbyname("md5");
static char printable[2*CF_MD5_LEN+1] = {0};

if (!md)
   {
   printf("Unable to initialize digest, digests added?\n");
   return NULL;
   }

EVP_MD_CTX context;
unsigned int md_len = 0;
unsigned int i;

EVP_DigestInit(&context,md);
EVP_DigestUpdate(&context,name,strlen(name));
EVP_DigestFinal(&context, digest, &md_len);

for (i = 0; i < CF_MD5_LEN; i++)
   {
   snprintf(printable + 2 * i, sizeof(printable) - (2 * i), "%02x", digest[i]);
   }

return printable;
}

/*****************************************************************************/

char *GetConceptFromDigest(char *digeststr,char *name)
{
 FILE *fp;
 char filename[CGN_BUFSIZE];

 snprintf(filename,CGN_BUFSIZE,"%s/%s/concept",VBASEDIR,digeststr);
 
 if ((fp = fopen(filename,"r")) != NULL)
    {
    fscanf(fp,"%4095[^\n]",name);
    fclose(fp);
    }
 else
    {
    printf("Unable to get the concept name for digest `%s'\n",digeststr);
    }
 
 return name;
}

/*****************************************************************************/

void InitializeAssociations(LinkAssociation *array)

{ int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    array[i].fwd = NULL;
    array[i].bwd = NULL;
    array[i].icontext = NULL;
    array[i].cdigest = NULL;
    array[i].weight = 0;
    array[i].lastseen = 0;
    array[i].relevance = 0;
    }
}

/*****************************************************************************/

void DeleteAssociations(LinkAssociation *array)

{ int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    free(array[i].fwd);
    free(array[i].bwd);
    free(array[i].icontext);
    free(array[i].cdigest);
    }
}

/*****************************************************************************/

void GetConceptAssociations(FILE *fin, char *concept, LinkAssociation *array,int maxentries)

{ int i;
 char fwd[CGN_BUFSIZE];
 char bwd[CGN_BUFSIZE];
 char context[CGN_BUFSIZE];

 InitializeAssociations(array);
 
 for (i = 0; !feof(fin); i++)
    {
    if (i >= maxentries)
       {
       return;
       }

    fwd[0] = '\0';
    bwd[0] = '\0';
    context[0] = '\0';
            
    fscanf(fin, "(%[^,],%[^,],%ld,%lf,%[^)])\n",fwd,bwd,&(array[i].lastseen),&(array[i].weight),context);

    array[i].fwd = strdup(fwd);
    array[i].bwd = strdup(bwd);
    array[i].icontext = strdup(context);
    array[i].cdigest = strdup(concept);

    //printf("FOUND(%s: %s(%s) in context %s(%s)\n", array[i].cdigest,fwd,array[i].fwd,context,array[i].icontext);
    }
}

/*****************************************************************************/

void WriteConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries)
{ int i;

 for (i = 0; (i < maxentries) && array[i].fwd; i++)
    {
    fprintf(fin, "(%s,%s,%ld,%lf,%s)\n",array[i].fwd,array[i].bwd,array[i].lastseen,array[i].weight,array[i].icontext);

    //printf("WROTE(%s <> %s at %s x %.2lf) context %s\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight,array[i].icontext);
    }
}

/*****************************************************************************/

void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2)
{
 char filename[CGN_BUFSIZE];
 LinkAssociation array[MAX_ASSOC_ARRAY];
 FILE *fp;
 int i, done;
 time_t now = time(NULL);
 
 // (fwd,bwd,lastseen,weight)

 if (fwd[0] == '\0')
    {
    return;
    }

 InitializeAssociations(array);
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",VBASEDIR,concept1,atype);
 mkdir(filename,0755);

 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",VBASEDIR,concept1,atype,concept2);

 if ((fp = fopen(filename,"r")) != NULL)
    {
    GetConceptAssociations(fp,filename,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }

 done = false;

 // Update all the aliases for this association
 
 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd != NULL); i++)
    {
    if ((strcmp(fwd,array[i].fwd) == 0))
       {
       array[i].weight = (0.6+0.4*array[i].weight);
       array[i].lastseen = now;
       done = true;
       break;
       }
    }

 // If Not already registered, add a new line
 
 if (!done && (i < MAX_ASSOC_ARRAY))
    {
    array[i].fwd = strdup(fwd);
    array[i].bwd = strdup(bwd);
    array[i].icontext = strdup(context);
    array[i].weight = 0.7;
    array[i].lastseen = now;
    array[i].relevance = 99;
    }

 if ((fp = fopen(filename,"w")) != NULL)
    {
    WriteConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }
 else
    {
    printf("Can't write file (%s)\n",filename);
    printf(" - from concept %d (%s)\n",strlen(concept1),concept1);
    printf(" - to concept %d (%s)\n",strlen(concept2),concept2);
    printf(" - total %d\n",strlen(filename));
    perror("fopen");
    }
}

