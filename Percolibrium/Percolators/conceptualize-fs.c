/*****************************************************************************/
/*                                                                           */
/* File: conceptualize-fs.c                                                  */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o conceptualize -g -std=c99 conceptualize-fs.c

 // Usage example: ./conceptualize ~/.CGNgine/state/env_graph 

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

typedef int Policy; // Hack to use CGNgine defs

#define true 1
#define false 0
#define CGN_BUFSIZE 4096

// Import standard link definitions

#define GRAPH 1
#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/*****************************************************************************/

#include "../Percolators/associations.h"
#include "../Percolators/associations.c"

/*****************************************************************************/

void ReadTupleFile(char *filename);
char *UpdateConcept(char *name);
void Canonify(char *str);

char VBASEDIR[256];

/*****************************************************************************/

int main(int argc, char** argv)
{
 if (argc < 2)
    {
    printf("Syntax: conceptualize <input file>\n");
    exit(1);
    }

 OpenSSL_add_all_digests();
  
 snprintf(VBASEDIR,255,"%s-%d",BASEDIR,getuid());
 mkdir(VBASEDIR,0755);
 
 // foreach file

 int i;

 for (i = 1; argv[i] != NULL; i++)
    {
    printf("Processing %s\n",argv[i]);
    ReadTupleFile(argv[i]);
    }

 return 0;
}

/*****************************************************************************/

void ReadTupleFile(char *filename)
{
 FILE *fin;
 char from[CGN_BUFSIZE];
 char to[CGN_BUFSIZE];
 char afwd[CGN_BUFSIZE]; 
 char abwd[CGN_BUFSIZE];
 char icontext[CGN_BUFSIZE];
 char linebuff[CGN_BUFSIZE];
 int atype;
 int line = 0;
 char from_digest[129];
 char to_digest[129];
 char ictx_digest[129];
 char any_digest[129];

 if ((fin = fopen(filename,"r")) == NULL)
    {
    return;
    }

 // Input validation?
 
 while (!feof(fin))
    {
    linebuff[0] = '\0';

    memset(from,0,CGN_BUFSIZE);
    memset(to,0,CGN_BUFSIZE);
    memset(icontext,0,CGN_BUFSIZE);
    memset(afwd,0,CGN_BUFSIZE);
    memset(abwd,0,CGN_BUFSIZE);
    
    fgets(linebuff, CGN_BUFSIZE, fin);

    switch (linebuff[0])
       {
       case '(':
           break;
       default:
           continue;
       }

    sscanf(linebuff, "(%1024[^,],%d,%1024[^,],%1024[^,],%1024[^,],%1024[^)])\n",from,&atype,afwd,to,abwd,icontext);

    if (strlen(from) == 0)
       {
       printf("Missing from field near line %d (%s) len %d\n",line,linebuff,strlen(linebuff));
       printf("FROM: %s\nTYPE %d\nFWD %s\nTO %s\nBWD %s\nCTX %s\n",from,atype,afwd,to,abwd,icontext);
       return;
       }

    if (strlen(afwd) == 0)
       {
       printf("Missing fwd field near line %d (%s) - length %d\n",line,linebuff,strlen(linebuff));
       printf("FROM: %s\nTYPE %d\nFWD %s\nTO %s\nBWD %s\nCTX %s\n",from,atype,afwd,to,abwd,icontext);
       return;
       }

    if (strlen(abwd) == 0)
       {
       printf("Missing bwd field near line %d (%s)\n",line,from);
       printf("FROM: %s\nTYPE %d\nFWD %s\nTO %s\nBWD %s\nCTX %s\n",from,atype,afwd,to,abwd,icontext);
       return;
       }

    if (strlen(to) == 0)
       {
       printf("Missing to field near line %d (%s)\n",line,from);
       printf("FROM: %s\nTYPE %d\nFWD %s\nTO %s\nBWD %s\nCTX %s\n",from,atype,afwd,to,abwd,icontext);
       return;
       }
        
    if (strlen(icontext) == 0)
       {
       printf("Missing ictx field near line %d (%s)\n",line,from);
       printf("FROM: %s\nTYPE %d\nFWD %s\nTO %s\nBWD %s\nCTX %s\n",from,atype,afwd,to,abwd,icontext);
       return;
       }

    // End validation
    
    strcpy(from_digest,UpdateConcept(from));
    strcpy(to_digest,UpdateConcept(to));
                                
    // In all contexts, the contextualized qualified version is a member of the cluster of all (class instance)
       
    UpdateAssociation(icontext,from_digest,atype,afwd,abwd,to_digest);
    UpdateAssociation(icontext,to_digest,-atype,abwd,afwd,from_digest);

    // Now create an introspective feedback to context as a scaled concept of its own

    strcpy(ictx_digest,UpdateConcept(icontext));
    strcpy(any_digest,UpdateConcept(ALL_CONTEXTS));
    
    UpdateAssociation(ALL_CONTEXTS,from_digest,-GR_CONTEXT,"appears in the context of","mentions the story topic",ictx_digest);
    UpdateAssociation(ALL_CONTEXTS,ictx_digest,GR_CONTEXT,"mentions the story topic","appears in the context of",from_digest);
    UpdateAssociation(ALL_CONTEXTS,to_digest,-GR_CONTEXT,"appears in the context of","mentions the story topic",ictx_digest);
    UpdateAssociation(ALL_CONTEXTS,ictx_digest,GR_CONTEXT,"mentions the story topic","appears in the context of",to_digest);

    UpdateAssociation(ALL_CONTEXTS,any_digest,GR_CONTEXT,"contains","is contained by",ictx_digest);
    UpdateAssociation(ALL_CONTEXTS,ictx_digest,-GR_CONTEXT,"is contained by","contains",any_digest);
    
    line++;
    }
 
 fclose(fin);
}

/*****************************************************************************/

char *UpdateConcept(char *name)
{
 char filename[CGN_BUFSIZE];
 unsigned char digest[EVP_MAX_MD_SIZE + 1];
 char *digeststr;
 FILE *fp;
 
 if (digeststr = NameDigest(name,digest))
    {
    // Represent a concept as a directory of associations (idempotent) using digest

    snprintf(filename,CGN_BUFSIZE,"%s/%s",VBASEDIR,digeststr);
    mkdir(filename,0755); 
    utime(filename, NULL);
    
    snprintf(filename,CGN_BUFSIZE,"%s/%s/concept",VBASEDIR,digeststr);
    
    if ((fp = fopen(filename,"w")) != NULL)
       {
       fprintf(fp,"%s",name);
       fclose(fp);
       return digeststr;
       }
    
    printf("Unable to write %s\n",filename);
    perror("fopen");
    return "failed";
    }
 
 printf("Unable to compute digest for %s\n",name);
 perror("evp");
 return "failed";
}

/*****************************************************************************/

void Canonify(char *str)
    
{ char *sp;
 
 for (sp = str; *sp != '\0'; sp++)
    {
    if (*sp == '/' || *sp == '\\' || *sp == ',')
       {
       *sp = '!';
       }
    else
       {
       *sp = tolower(*sp);
       }
    }
}
