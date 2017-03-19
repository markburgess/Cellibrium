/*****************************************************************************/
/*                                                                           */
/* File: generate_cgn_domain_knowledge.c                                     */
/*                                                                           */
/* Created: Mon Nov 21 14:10:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o gen_dsdk -g -std=c99 generate_doctor_domain_knowledge.c

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

/******************************************************************************/

int main()
{

 // What kinds of compound contexts can we expect? State of mind...
 
 ContextCluster(stdout,"doctor service");
 ContextCluster(stdout,"patient appointment");
 ContextCluster(stdout,"patient health service");
 ContextCluster(stdout,"need to visit a doctor");
 ContextCluster(stdout,"patient doctor registration"); 
 ContextCluster(stdout,"identity authentication verification");
 
 // Compound (qualified) concepts

 RoleCluster(stdout,"general practictioner doctor","doctor", "general practitioner", "patient health service");
 RoleCluster(stdout,"surgeon doctor","doctor", "surgeon", "patient health service");
 RoleCluster(stdout,"patient appointment","appointment", "doctor patient", "patient health service");

 // Record realtime state relationships (promises) - note the role of a STATE is the state outcome, not the subject

 RoleCluster(stdout,"doctor authenticated","authenticated","doctor", "patient health service");
 RoleCluster(stdout,"patient authenticated","authenticated","patient", "patient health service");

 // Note how fragile this is to difference between available,availability ... linguistically we could fuzzy match
 
 RoleCluster(stdout,"general practictioner doctor available","doctor availability", "general,practitioner,doctor", "patient health service");
 RoleCluster(stdout,"open for business","service availability", "open,available", "need to visit a doctor");

 Gr(stdout,"have public health service access",a_depends,"patient authenticated","need to visit a doctor");
 Gr(stdout,"have public health service access",a_depends,"patient appointment","need to visit a doctor");
 Gr(stdout,"have public health service access",a_depends,"public health service available","need to visit a doctor");

 Gr(stdout,"public health service available",a_depends,"general practictioner doctor available","need to visit a doctor");
 Gr(stdout,"public health service available",a_depends,"open for business","need to visit a doctor");

 Gr(stdout,"public health service",a_depends,"public health service available","need to visit a doctor");
 Gr(stdout,"public health service",a_depends,"patient uses appointment","need to visit a doctor");
 Gr(stdout,"patient appointment",a_depends,"patient doctor register binding","need to visit a doctor");
 
 Gr(stdout,"patient doctor register binding",a_depends,"doctor patient binding service","patient doctor registration");
 Gr(stdout,"doctor",a_promises,"public health service","need to visit a doctor"); 
 Gr(stdout,"patient",a_promises,"patience","need to visit a doctor");

 Gr(stdout,"public health service",a_depends,"doctor availability","need to visit a doctor");

 Gr(stdout,"accepted doctor patient binding",a_depends,"doctor","patient doctor registration");
 Gr(stdout,"accepted doctor patient binding",a_depends,"patient","patient doctor registration");
 Gr(stdout,"accepted doctor patient binding",a_depends,"doctor authorized","patient doctor registration");
 Gr(stdout,"accepted doctor patient binding",a_depends,"doctor authenticated","patient doctor registration");
 Gr(stdout,"accepted doctor patient binding",a_depends,"patient authenticated","patient doctor registration");
  
 Gr(stdout,"doctor authenticated",a_depends,"identity credentials","identity authentication verification");
 Gr(stdout,"patient authenticated",a_depends,"identity credentials","identity authentication verification");

// Wizard info gathered as public info, REST query etc

 Gr(stdout,"identity credentials",a_origin,"https://url1/form/element1","identity authentication verification");
 Gr(stdout,"identity credentials",a_origin,"https://url1/form/element2","identity authentication verification");

//
// dynamically changing observations (cognitive inputs)
//

// User sensors - what am I doing now?

 Gr(stdout,"doctor",a_promises,"accepted doctor patient binding","patient doctor registration");
 Gr(stdout,"patient",a_promises,"accepted doctor patient binding","patient doctor registration");
 
 Gr(stdout,"entity authentication",a_promises,"doctor authenticated","patient doctor registration");
 Gr(stdout,"entity authentication",a_promises,"patient authenticated","patient doctor registration");
 Gr(stdout,"medical association",a_promises,"doctor authorized","doctor registration");

 Gr(stdout,"doctor",a_promises,"doctor availability","patient doctor registration");
 Gr(stdout,"patient uses appointment",a_hasoutcome,"promise kept",ContextCluster(stdout,"public health service patient doctor registration"));

 Gr(stdout,"doctor",a_promises,"identity credentials","patient doctor registration");
 Gr(stdout,"patient",a_promises,"identity credentials","patient doctor registration");

 return 0;
}

