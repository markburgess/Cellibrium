/*****************************************************************************/
/*                                                                           */
/* File: generate_microservice_domain_knowledge.c                            */
/*                                                                           */
/* Created: Mon Nov 21 14:10:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o gen_msdk -g generate_microservice_domain_knowledge.c

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
#define MAX_ASSOC_ARRAY 128

// Import standard link definitions

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/******************************************************************************/

// Can we create a kind of crawler for microservices, e.g. in golang a single static binary

int main()
{

 // What menu of contexts can we describe and measure?

 ContextGr(stdout, "application service");
 ContextGr(stdout, "design write develop software");
 ContextGr(stdout, "run execute software");
 ContextGr(stdout, "software ops operations");
 ContextGr(stdout, "use software");
 ContextGr(stdout, "bug fault failure error");

 char *context = "software ops operations develop write";
 ContextGr(stdout,context);

 // Roles etc.
 
 RoleGr(stdout,"application service","application", "component,software,service",context);
 RoleGr(stdout,"microservice","application", "component,software,service",context);

 RoleGr(stdout,"service authentication","authentication", "component,software,application service,microservice,API authentication",context);
 RoleGr(stdout,"user authentication","authentication", "component,software,application service,microservice",context);

 //
 
 Gr(stdout, "ops",a_approx,"operations","software ops operations");
 Gr(stdout, "microservice",a_approx,"service",context);
 Gr(stdout, "service",a_uses,"service authentication",context);
 Gr(stdout, "service",a_uses,"user authentication",context);
 Gr(stdout, "service",a_depends,"software SOFTWARENAME",context);
 Gr(stdout, "service",a_depends,"application cgn-agent",context);

 
 RoleGr(stdout,"software SOFTWARENAME VERSION","software", "SOFTWARENAME,VERSION",context);
 RoleGr(stdout,"container CONTAINERNAME VERSION","container", "CONTAINERNAME,VERSION",context);
 RoleGr(stdout,"package PACKAGENAME1 VERSION1","package", "PACKAGENAME1,VERSION1",context);
 RoleGr(stdout,"package PACKAGENAME2 VERSION2","package", "PACKAGENAME2,VERSION2",context);
 RoleGr(stdout,"package PACKAGENAME3 VERSION3","package", "PACKAGENAME3,VERSION3",context);
 
 Gr(stdout, "software SOFTWARENAME",a_depends,"container CONTAINERNAME",context);

 Gr(stdout, "container CONTAINERNAME",a_depends,"package PACKAGENAME1 VERSION1",context);
 Gr(stdout, "container CONTAINERNAME",a_depends,"package PACKAGENAME2 VERSION2",context);
 Gr(stdout, "container CONTAINERNAME",a_depends,"package PACKAGENAME3 VERSION3",context);
 Gr(stdout, "container CONTAINERNAME",a_depends,"hosting CONTAINERNAME",context);
 Gr(stdout, "container CONTAINERNAME",a_depends,"storage",context);

 // This comes from an orchestrator K8s, openstack etc?
 
 RoleGr(stdout,"hosting CONTAINERNAME INSTANCE","hosting", "CONTAINERNAME,INSTANCE",context);
  
 // These should be scanned by internal scanner, etc
 
 Gr(stdout, "package PACKAGENAME1 VERSION1",a_depends,"library LIB1",context);
 Gr(stdout, "package PACKAGENAME2 VERSION2",a_depends,"library LIB1",context);
 Gr(stdout, "package PACKAGENAME3 VERSION3",a_depends,"library LIB1",context);

 
 //

 Gr(stdout, "service", a_depends, "hosting","software ops operations");
 Gr(stdout, "hosting", a_depends, "host HOSTNAME","execute run software ops operations");

 RoleGr(stdout,"host HOSTNAME IPADDRESS","host", "hostname HOSTNAME,ip address IPADDRESS",context);
  
 Gr(stdout, "host", a_promises, "operating system","execute run software ops operations");
 Gr(stdout, "operating system",a_promises,"kernel","execute run software ops operations");

 InitialGr(stdout);
 
 ServerInstanceGr(stdout,
               "ssh",
                  22,
               "/usr/local/sshd",
                  WhereGr(stdout,"London","myserver","example.com","123.456.789.10/24","2001:::7/64")
                  );;

 ClientInstanceGr(stdout,
               "ssh",
               "/usr/bin/ssh",
                  WhereGr(stdout,"San Jose","desktop","example.com","321.654.987.99/24","2002:::8/64")
               );
 return 0;
}

