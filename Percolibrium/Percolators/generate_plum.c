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
// Maja

 // Usage::
 // ContextGr(what am I thinking? What do I see/feel/sense?)
 
 ContextGr(stdout, "found dead body in the library");
 ContextGr(stdout, "found blood in the library");

/******************************************************************************/

Gr(stdout, "dead body", a_caused_by, "murder", "crime");
Gr(stdout, "blood", a_caused_by, "murder", "crime");

// RoleGr(stdout,"drama", "genre", "other attributes", "interior context interpretation");

RoleGr(stdout,
            "Professor Plum murders Miss Scarlet in the library with a breadknife because she would not marry him",
            "murder by breadknife",
            "in the library,Professor plum,Miss Scarlet,Miss Scarlet refuses to marry Professory plum",
            "*"
            );

RoleGr(stdout,
            "murder by breadknife",
            "what action verb",
            "murder,by breadknife",
            "*"
            );

RoleGr(stdout,
            "by breadknife",
            "how",
            "breadknife",
            "*"
            );

RoleGr(stdout,
            "breadknife",
            "knife",
            "used for bread",
            "*"
            );


RoleGr(stdout,
            "in the library",
            "where",
            "the library",
            "*"
            );

RoleGr(stdout,
            "the library",
            "library",
            "Professor Plum's library",
            "*"
            );

RoleGr(stdout,
            "Miss Scarlet refuses to marry Professory plum",
            "why",
            "Miss Scarlet,marriage refusal,Professor Plum",
            "*"
            );

RoleGr(stdout,
            "marriage refusal",
            "refusal",
            "marriage",
            "*"
            );

Gr(stdout,"Professor Plum",a_hasrole,"who","murder marriage");
Gr(stdout,"Miss Scarlet",a_hasrole,"who","murder marriage");

/******************************************************************************/
/* Homomorphic example                                                        */
/******************************************************************************/

RoleGr(stdout,
            "Function MyClass:MyFn() failed in microservice MyService executing in container MyS1 with exit code 345 on host MYHOST because it received exception Out Of Storage",
            "failed with exit code 345",
            "Function MyClass:MyFn(),executing in Container MyS1 on MYHOST,received exception Out of Storage",
            "*"
            );

RoleGr(stdout,
            "failed with exit code 345",
            "how",
            "failed,exit code 345",
            "*"
            );

RoleGr(stdout,
            "Function MyClass:MyFn()",
            "function",
            "MyClass:MyFn()",
            "*"
            );

Gr(stdout,"file git/file/myfile.lang",a_contains,"MyClass:MyFn()","*");
Gr(stdout,"code class MyClass",a_contains,"MyFn()","*");

RoleGr(stdout,
            "executing in Container MyS1 on MYHOST",
            "where",
            "Container MyS1,MYHOST",
            "*"
            );

RoleGr(stdout,
            "received exception Out of Storage",
            "why",
            "exception out of storage",
            "*"
            );

Gr(stdout,"exception out of storage",a_origin,"Storage Service S3","operational state error failure");

return 0;
}

