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

#define true 1
#define false 0
#define CGN_BUFSIZE 256
#define MAX_ASSOC_ARRAY 128

/*****************************************************************************/

typedef struct
{
   char fwd[CGN_BUFSIZE];
   char bwd[CGN_BUFSIZE];
   time_t lastseen;
   double weight;
   
} Association;

/*****************************************************************************/

void ReadTupleFile(char *filename);
void UpdateConcept(char *name);
void UpdateAssociation(char *concept1, int atype, char *fwd, char *bwd, char *concept2);
void GetConceptAssociations(FILE *fp, Association *array,int maxentries);
void UpdateConceptAssociations(FILE *fp, Association *array,int maxentries);
void InitializeAssociations(Association *array);
void Canonify(char *str);

/*****************************************************************************/

void main(int argc, char** argv)
{
 if (argc =! 2)
    {
    printf("Syntax: conceptualize <input file>\n");
    exit(1);
    }

 mkdir(BASEDIR,0755);
 
 // foreach file
 
 ReadTupleFile(argv[1]); 
}

/*****************************************************************************/

void ReadTupleFile(char *filename)
{
 FILE *fin;
 char from[CGN_BUFSIZE];
 char to[CGN_BUFSIZE];
 char afwd[CGN_BUFSIZE];
 char abwd[CGN_BUFSIZE];
 int atype;

 if ((fin = fopen(filename,"r")) == NULL)
    {
    return;
    }

 // Input validation?
 
 while (!feof(fin))
    {
    from[0] = to[0] = afwd[0] = abwd[0] = '\0';
    
    fscanf(fin, "(%[^,],%d,%[^,],%[^,],%[^)])\n",from,&atype,afwd,to,abwd);

    Canonify(from);
    Canonify(to);
    
    //printf("FROM(%s) -(%s)- TO(%s)\n  (%s,%d)\n",from,afwd,to,abwd,atype);
    UpdateConcept(from);
    UpdateConcept(to);
    UpdateAssociation(from,atype,afwd,abwd,to);
    UpdateAssociation(to,-atype,abwd,afwd,from);
    }
 
 fclose(fin);
}

/*****************************************************************************/

void UpdateConcept(char *name)
{
 char filename[CGN_BUFSIZE];

 // Represent a concept as a directory of associations
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s",BASEDIR,name);

 // Ignore return code to make idempotent

 mkdir(filename,0755); 
 utime(filename, NULL);
}

/*****************************************************************************/

void UpdateAssociation(char *concept1, int atype, char *fwd, char *bwd, char *concept2)
{
 char filename[CGN_BUFSIZE];
 Association array[MAX_ASSOC_ARRAY];
 FILE *fp;
 int i, done;
 time_t now = time(NULL);
 
 // (fwd,bwd,lastseen,weight)

 if (fwd[0] == '\0')
    {
    return;
    }

 InitializeAssociations(array);
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",BASEDIR,concept1,atype);
 mkdir(filename,0755);
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",BASEDIR,concept1,atype,concept2);

 if ((fp = fopen(filename,"r")) != NULL)
    {
    GetConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }

 done = false;

 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd[0] != '\0'); i++)
    {
    if (strcmp(fwd,array[i].fwd) == 0)
       {
       array[i].weight = (0.6+0.4*array[i].weight);
       array[i].lastseen = now;
       done = true;
       break;
       }
    }

 if (!done)
    {
    strcpy(array[i].fwd,fwd);
    strcpy(array[i].bwd,bwd);
    array[i].weight = 0.7;
    array[i].lastseen = now;
    }

 if ((fp = fopen(filename,"w")) != NULL)
    {
    UpdateConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
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

void GetConceptAssociations(FILE *fin, Association *array,int maxentries)

{ int i;

 for (i = 0; (i < maxentries) && !feof(fin); i++)
    {
    array[i].fwd[0] = array[i].bwd[0] = '\0';
    array[i].weight = 0;
    array[i].lastseen = 0;
    
    fscanf(fin, "(%[^,],%[^,],%ld,%lf)\n",array[i].fwd,array[i].bwd,&(array[i].lastseen),&(array[i].weight));

    //printf("FOUND(%s <> %s at %s x %.2lf)\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight);
    }
}

/*****************************************************************************/

void UpdateConceptAssociations(FILE *fin, Association *array,int maxentries)

{ int i;

 for (i = 0; (i < maxentries) && (array[i].fwd[0] != '\0'); i++)
    {
    fprintf(fin, "(%s,%s,%ld,%lf)\n",array[i].fwd,array[i].bwd,array[i].lastseen,array[i].weight);

    //printf("WROTE(%s <> %s at %s x %.2lf)\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight);
    }
}

/*****************************************************************************/

void Canonify(char *str)
    
{
 for (char *sp = str; *sp != '\0'; sp++)
    {
    if (*sp == '/' || *sp == '\\')
       {
       *sp = ':';
       }
    }
}
