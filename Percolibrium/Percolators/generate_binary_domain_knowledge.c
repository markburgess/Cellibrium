/*****************************************************************************/
/*                                                                           */
/* File: generate_binary_domain_knowledge.c                                     */
/*                                                                           */
/* Created: Mon Nov 21 14:10:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o gen_knowledge -g generate_binary_domain_knowledge.c

 // Usage example: ./gen_knowledge > ExampleTupleData/domain_knowledge_graph

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>


typedef int Policy;  // Hack to use CGNgine defs

#define true 1
#define false 0
#define CGN_BUFSIZE 256
#define MAX_ASSOC_ARRAY 128

// Import standard link definitions

#define GRAPH 1
#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

void CrawlApplication(char *path, char *app, char *desc);
void LddDependencies(char *app,char *file, char *context);

/******************************************************************************/

void main()
{

 // What menu of contexts can we describe and measure?

 ContextCluster(stdout, "application service");

 //RegisterMicroServiceP0("authenticator");
 //RegisterMicroServiceP1("cgn-serverd","CGNgine copy and remote execution service");
 //RegisterHosting(name,ipaddress,geography);

 CrawlApplication("/var/CGNgine/bin","cgn-agent","host configuration and maintenance agent for CGNgine");
 //CrawlApplication("/var/CGNgine/bin","cgn-serverd","fileserver and remote execution service for CGNgine");
 //CrawlApplication("/var/CGNgine/bin","cgn-monitord","host probe and monitoring service for CGNgine");
 
}

/******************************************************************************/

void CrawlApplication(char *path, char *app, char *desc)
{
 // Most other services depend on this one (level 0)

 char name[CGN_BUFSIZE];
 char fqname[CGN_BUFSIZE];
 char context[CGN_BUFSIZE];

 strcpy(context,ContextCluster(stdout,"host application software dependencies security"));

 snprintf(name, CGN_BUFSIZE, "application %s", app);
 RoleCluster(stdout,name,"application", "component,CGNgine",context);

 // Get the dependency graph

 snprintf(fqname, CGN_BUFSIZE, "%s/%s",path,app);
 LddDependencies(app,fqname,context);
 
 //Gr(stdout,name,a_hasfunction,"service authentication","software dependencies");

 // Host 


}

/**********************************************************************/

void LddDependencies(char *app, char *file, char *context)

{ FILE *pp;
  char line[CGN_BUFSIZE];
  char name[CGN_BUFSIZE];
  char path[CGN_BUFSIZE];
  char cmd[CGN_BUFSIZE];

  snprintf(cmd, CGN_BUFSIZE, "/usr/bin/ldd %s",file);
  
 if ((pp = popen(cmd,"r")) == NULL)
    {
    perror("popen");
    return;
    }

 while (!feof(pp))
    {
    name[0] = '\0';
    path[0] = '\0';
    line[0] = '\0';
    fgets(line,CGN_BUFSIZE,pp);
    sscanf(line,"%s %*[^/]%s",name,path);

    if (strstr(name,"lib"))
       {
       Gr(stdout,app,a_depends,name,context);
       }
    
    if (path[0] == '/')
       {
       LddDependencies(name,path,context);
       }
    }

 pclose(pp);
}

/**********************************************************************/

void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,context);
}

/**********************************************************************/

void GrNOT(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 fprintf(consc,"(%s,%d,NOT %s,%s,NOT %s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,context);
}

/**********************************************************************/

char *RoleCluster(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context)

/* Document a compound Split a comma separated list, with head
   we can use it for context or for conceptual
   RoleCluster(fp, "compound name", "hasrole unique identifier", "hasttr part1,hasttr part2", "naming unique identity")
*/
    
{ char *sp, word[255];

 Gr(consc,compound_name,a_hasrole,role,ex_context);
 
 if ((sp = attributes))
    {
    while (*sp != '\0')
       {
       if (*sp == ',')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250[^,]",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_hasattr,word,"all contexts");
       }
    }

return compound_name;
}

/**********************************************************************/

char *ContextCluster(FILE *consc,char *compound_name)

/* Document a compound Split a space separated list, with head
   we can use it for context or for conceptual - treat them as epitopes
   for fuzzy matching by set overlap. Only type 1 associations. */
    
{ char *sp, word[255];

 if ((sp = compound_name))
    {
    while (*sp != '\0')
       {
       if (*sp == ' ')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250s",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_contains,word,"all contexts");
       }
    }

return compound_name;
}

