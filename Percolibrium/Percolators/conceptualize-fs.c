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

#define BASEDIR "/home/a10004/KMdata"

typedef int Policy; // Hack to use CGNgine defs

#define true 1
#define false 0
#define CGN_BUFSIZE 256
#define MAX_ASSOC_ARRAY 128
#define GR_CONTEXT 5

// Import standard link definitions

#define GRAPH 1
#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/*****************************************************************************/

#include "../Percolators/associations.h"
#include "../Percolators/associations.c"

/*****************************************************************************/

void ReadTupleFile(char *filename);
void UpdateConcept(char *name);
void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2);
void GetConceptAssociations(FILE *fp, LinkAssociation *array,int maxentries);
void UpdateConceptAssociations(FILE *fp, LinkAssociation *array,int maxentries);
void InitializeAssociations(LinkAssociation *array);
void Canonify(char *str);

/*****************************************************************************/

void main(int argc, char** argv)
{
 if (argc < 2)
    {
    printf("Syntax: conceptualize <input file>\n");
    exit(1);
    }

 mkdir(BASEDIR,0755);
 
 // foreach file

 int i;

 for (i = 1; i < argc -1; i++)
    {
    printf("Processing %s\n",argv[i]);
    ReadTupleFile(argv[i]);
    }
}

/*****************************************************************************/

void ReadTupleFile(char *filename)
{
 FILE *fin;
 char from[CGN_BUFSIZE];
 char to[CGN_BUFSIZE];
 char afwd[CGN_BUFSIZE]; 
 char abwd[CGN_BUFSIZE];
 char context[CGN_BUFSIZE];
 char linebuff[CGN_BUFSIZE];
 int atype;
 int line = 0;

 UpdateConcept(ALL_CONTEXTS);
 
 if ((fin = fopen(filename,"r")) == NULL)
    {
    return;
    }

 // Input validation?
 
 while (!feof(fin))
    {
    from[0] = to[0] = afwd[0] = abwd[0] = context[0] = '\0';
    linebuff[0] = '\0';
    
    fgets(linebuff, CGN_BUFSIZE, fin);

    switch (linebuff[0])
       {
       case '(':
           break;
       default:
           continue;
       }

    sscanf(linebuff, "(%[^,],%d,%[^,],%[^,],%[^,],%[^)])\n",from,&atype,afwd,to,abwd,context);

    if (strlen(from) == 0)
       {
       printf("Missing field near line %d (%s) len %d\n",line,linebuff,strlen(linebuff));
       }

    if (strlen(afwd) == 0)
       {
       printf("Missing field near line %d (%s)\n",line,linebuff,strlen(linebuff));
       }

    if (strlen(abwd) == 0)
       {
       printf("Missing field near line %d (%s)\n",line,from);
       }

    if (strlen(to) == 0)
       {
       printf("Missing field near line %d (%s)\n",line,from);
       }
        
    if (strlen(context) == 0)
       {
       printf("Missing field near line %d (%s)\n",line,from);
       }

    // End validation
        
    Canonify(from);
    Canonify(to);
    
    UpdateConcept(from);
    UpdateConcept(to);

    // In all contexts, the contextualized qualified version is a member of the cluster of all (class instance)
       
    UpdateAssociation(context,from,atype,afwd,abwd,to);
    UpdateAssociation(context,to,-atype,abwd,afwd,from);

    // Now create an introspective feedback to context as a scaled concept of its own

    UpdateConcept(context);
    UpdateAssociation(ALL_CONTEXTS,from,-GR_CONTEXT,"appears in the context of","mentions the story topic",context);
    UpdateAssociation(ALL_CONTEXTS,context,GR_CONTEXT,"mentions the story topic","appears in the context of",from);
    UpdateAssociation(ALL_CONTEXTS,to,-GR_CONTEXT,"appears in the context of","mentions the story topic",context);
    UpdateAssociation(ALL_CONTEXTS,context,GR_CONTEXT,"mentions the story topic","appears in the context of",to);

    UpdateAssociation(ALL_CONTEXTS,ALL_CONTEXTS,GR_CONTEXT,"contains","is contained by",context);
    UpdateAssociation(ALL_CONTEXTS,context,-GR_CONTEXT,"is contained by","contains",ALL_CONTEXTS);
    
    line++;
    }
 
 fclose(fin);
}

/*****************************************************************************/

void UpdateConcept(char *name)
{
 char filename[CGN_BUFSIZE];

 // Represent a concept as a directory of associations (idempotent)
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s",BASEDIR,name);
 mkdir(filename,0755); 
 utime(filename, NULL);
}

/*****************************************************************************/

void Canonify(char *str)
    
{
 for (char *sp = str; *sp != '\0'; sp++)
    {
    if (*sp == '/' || *sp == '\\' || *sp == ',')
       {
       *sp = '!';
       }
    }
}
