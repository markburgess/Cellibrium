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
 // ContextCluster(what am I thinking? What do I see/feel/sense?)
 
 ContextCluster(stdout, "personalized contextualize personal search query lookup");
 ContextCluster(stdout, "compose service");
 ContextCluster(stdout, "award reward the users of my service");
 ContextCluster(stdout, "operation management ops");

// Jin

 ContextCluster(stdout, "ensure assure services applications always running up available");
 ContextCluster(stdout, "ensure assure services applications are secure protected");
 ContextCluster(stdout, "ensure assure users customers clients information are secure protected");
 ContextCluster(stdout, "ensure assure servers applications response time perform operate within a specified limit constraint policy rule condition");

//Anup

 ContextCluster(stdout, "retrieve return search find users customers clients booking reservation information data");
 ContextCluster(stdout, "book movie film cinema tickets pass");
 ContextCluster(stdout, "find locate film cinema movie theaters theatres");//issues:
 ContextCluster(stdout, "get find film cinema movie reviews and sentiments");
 ContextCluster(stdout, "get find theater theatres parking  information data");

//Mark

 ContextCluster(stdout, "digital cinema film movie theater theatre");
 ContextCluster(stdout, "analog cinema film movie theater theatre");
 ContextCluster(stdout, "what did I do last Summer?");
 ContextCluster(stdout, "learning about Norway");
 ContextCluster(stdout, "looking for inspiration inspire");
 ContextCluster(stdout, "in hotel room");

 
 // Added

 ContextCluster(stdout, "geographical region location address");
 ContextCluster(stdout, "geographical location address");
 ContextCluster(stdout, "name internet location");
 ContextCluster(stdout, "proper name");
 ContextCluster(stdout, "qualify function role purpose");
 ContextCluster(stdout, "name internet location");
 ContextCluster(stdout, "films movies cinema searching");
 
// Roles etc.  invariant semantics vs more variable extensible contexts

// Usage:
// RoleCluster( What's its name?, What role does it play?, What qualifying attributes?, In what context?)
 
RoleCluster(stdout,"horror movie", "movie", "genre horror", "films movies cinema searching");
RoleCluster(stdout,"thriller (movie)", "movie", "genre thriller", "films movies cinema searching");
RoleCluster(stdout,"thriller (novel)", "novel", "genre thriller,book", "films movies cinema searching");
RoleCluster(stdout,"drama movie", "movie", "genre drama", "films movies cinema searching");
RoleCluster(stdout,"animation movie", "movie", "genre animation", "films movies cinema searching");
RoleCluster(stdout,"documentary movie", "movie", "genre documentary", "films movies cinema searching");
RoleCluster(stdout,"sci-fi movie", "movie", "genre sci-fi", "films movies cinema searching");


RoleCluster(stdout,"Odeon theatre showing Jaws", "showing or presenting", "movie,film,showing,presentation,Jaws", "culture");
RoleCluster(stdout,"Odeon theatre in New York City", "movie theatre", "find, movie,film,showing,presentation,", "geographical location address");
RoleCluster(stdout,"New York City is 20 miles from Elizabeth NJ", "distance", "find, travel, distance", "location");
RoleCluster(stdout,"20 miles from Elizabeth NJ", "distance", "Elizabeth NJ,miles,radius", "geographical region location address");

Gr(stdout, "distance measurement", a_name, "miles", "distances measurements");
Gr(stdout, "movie theatre", a_name, "Odeon", "proper name");

// RoleCluster(stdout,"drama", "genre", "", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "movies", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "plays", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "novels", "fiction");
 
//THese are better modelled as clusters than these originals
//Gr(stdout, "Jaws restaurant", a_name, "Jaws", ContextClucter("proper name"));
//Gr(stdout, "sushi", a_name, "food type", "food");
//Gr(stdout, "New York City", a_name, "city", "location");
//Gr(stdout, "Elizabeth NJ", a_name, "city", "location");


// Qualifier
RoleCluster(stdout,"jawssushi.com", "web address", "jawssushi restaurant", "name internet location");
RoleCluster(stdout,"jawssushi restaurant", "restaurant", "Jaws,Sushi,sells food type sushi,jawssushi.com", "proper name of restaurant");
RoleCluster(stdout,"Jaws movie", "movie", "Jaws,horror movie, thriller", "seems-to-be proper name of movie");
RoleCluster(stdout,"sells food type sushi", "sells", "food type sushi", "function role purpose");
RoleCluster(stdout,"food type sushi", "food type", "sushi", "food type");

// Names Geography
RoleCluster(stdout,"New York City", "city", "New York", "proper name");
RoleCluster(stdout,"Elizabeth NJ", "city", "Elizabeth", "proper name");
RoleCluster(stdout,"New York State", "state", "New York", "proper name");
RoleCluster(stdout,"State of New Jersey", "state", "New Jersey", "proper name");

RoleCluster(stdout,"jaws sushi", "restaurant service", "food, restaurant", "restaurant");
RoleCluster(stdout,"jaws sushi restaurant in Elizabeth NJ", "located", "find, food, restaurant", "location");

RoleCluster(stdout,"Film festival", "festival", "movie,film", "entertainment event movies");

//Gr(stdout, "Jaws", a_name, "movie", "entertainment");
//Gr(stdout, "Jaws", a_name, "character in James Bond movie", "entertainment");


RoleCluster(stdout,"jaws character in James Bond movie", "movie character", "jaws,character,James Bond movie", "searching for films movies cinema");
RoleCluster(stdout,"jaws character is Steven Spielberg movie series", "jaws,character,movie character", "Jaws movie", "searching for films movies cinema");

// Service Phenomena


RoleCluster(stdout,"web connections to jawssushi.com","web connections","jawssushi.com","browsing the web");
RoleCluster(stdout,"jawssushi.com","sushi restaurant website","web,jaws,sushi","domain names internet search");

RoleCluster(stdout,"web connections to jawssushi.com", "web connections", "jawssushi.com", "location");
Gr(stdout,"web connections for jaws", a_related_to, "web connections to jawssushi.com", "online web");
Gr(stdout,"web connections for jaws", a_related_to, "web searches for sushi", "online web");

// Causation ///////////////////////////////////////////////////////////////////////////////////////////////////

// Intentional motivation for state
RoleCluster(stdout,"Interest in Film Festival","interest","Film festival,interested in","Browsing the web entertainment culture");
Gr(stdout, "web connections for jaws", a_caused_by, "Interest in Film Festival", "Browsing the web entertainment culture");


// state anomaly depends on an ingredient
RoleCluster(stdout, "high web traffic","web traffic","high,traffic,web","operational state anomaly monitoring data");
Gr(stdout, "high web traffic", a_caused_by, "web connections", "online web");

RoleCluster(stdout,"high web connections to jawssushi.com", "web connections to jawssushi.com", "high-anomaly,find, food, restaurant", "monitoring service performance");

// Derivative concept
RoleCluster(stdout, "restaurant web traffic","web traffic","restaurant,traffic,web","operational state monitoring data");
Gr(stdout, "restaurant web traffic", a_caused_by, "web connections", "online web");

 // Specifically, monitoring tells us that ... (forming a temporary context bridge)
Gr(stdout, "high web traffic", a_caused_by, "web searches for sushi", "traffic load monitoring");
Gr(stdout, "high web traffic", a_caused_by, "web searches for jaws", "traffic load monitoring");




 return 0;
}

