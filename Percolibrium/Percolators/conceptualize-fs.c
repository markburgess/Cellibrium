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
void UpdateConcept(char *name);
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

 snprintf(VBASEDIR,255,"%s-%d",BASEDIR,getuid());
 mkdir(VBASEDIR,0755);
 
 // foreach file

 int i;

 for (i = 1; i < argc -1; i++)
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

 UpdateConcept(ALL_CONTEXTS);
 
 if ((fin = fopen(filename,"r")) == NULL)
    {
    return;
    }

 // Input validation?
 
 while (!feof(fin))
    {
    from[0] = to[0] = afwd[0] = abwd[0] = icontext[0] = '\0';
    linebuff[0] = '\0';
    
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
        
    Canonify(from);
    Canonify(to);
    
    UpdateConcept(from);
    UpdateConcept(to);

    // In all contexts, the contextualized qualified version is a member of the cluster of all (class instance)
       
    UpdateAssociation(icontext,from,atype,afwd,abwd,to);
    UpdateAssociation(icontext,to,-atype,abwd,afwd,from);

    // Now create an introspective feedback to context as a scaled concept of its own

    UpdateConcept(icontext);
    UpdateAssociation(ALL_CONTEXTS,from,-GR_CONTEXT,"appears in the context of","mentions the story topic",icontext);
    UpdateAssociation(ALL_CONTEXTS,icontext,GR_CONTEXT,"mentions the story topic","appears in the context of",from);
    UpdateAssociation(ALL_CONTEXTS,to,-GR_CONTEXT,"appears in the context of","mentions the story topic",icontext);
    UpdateAssociation(ALL_CONTEXTS,icontext,GR_CONTEXT,"mentions the story topic","appears in the context of",to);

    UpdateAssociation(ALL_CONTEXTS,ALL_CONTEXTS,GR_CONTEXT,"contains","is contained by",icontext);
    UpdateAssociation(ALL_CONTEXTS,icontext,-GR_CONTEXT,"is contained by","contains",ALL_CONTEXTS);
    
    line++;
    }
 
 fclose(fin);
}

/*****************************************************************************/

void UpdateConcept(char *name)
{
 char filename[CGN_BUFSIZE];

 // Represent a concept as a directory of associations (idempotent)
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s",VBASEDIR,name);
 mkdir(filename,0755); 
 utime(filename, NULL);
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
