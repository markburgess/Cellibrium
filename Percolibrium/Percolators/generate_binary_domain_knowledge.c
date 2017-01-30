/*****************************************************************************/
/*                                                                           */
/* File: generate_binary_domain_knowledge.c                                     */
/*                                                                           */
/* Created: Mon Nov 21 14:10:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o gen_bdk -g generate_binary_domain_knowledge.c

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

#define true 1
#define false 0
#define CGN_BUFSIZE 256
#define MAX_ASSOC_ARRAY 128

// Import standard link definitions

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

void CrawlApplication(char *path, char *app, char *desc);
void LddDependencies(char *app,char *file, char *context);

/******************************************************************************/

void main()
{

 // What menu of contexts can we describe and measure?

 ContextCluster(stdout, "host configuration maintenance agent CGNgine");

 RoleCluster(stdout,"application cgn-agent","application", "cgn-agent,software,service","host configuration maintenance agent CGNgine");

 Gr(stdout,"application cgn-agent",a_depends,"cgn-agent","host configuration maintenance agent CGNgine");
 CrawlApplication("/var/CGNgine/bin","cgn-agent","host configuration maintenance agent CGNgine");
 
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

