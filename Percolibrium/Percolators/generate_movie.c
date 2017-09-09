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
 
 ContextGr(stdout, "personalized contextualize personal search query lookup");
 ContextGr(stdout, "compose service");
 ContextGr(stdout, "award reward the users of my service");
 ContextGr(stdout, "operation management ops");

// Jin

 ContextGr(stdout, "ensure assure services applications always running up available");
 ContextGr(stdout, "ensure assure services applications are secure protected");
 ContextGr(stdout, "ensure assure users customers clients information are secure protected");
 ContextGr(stdout, "ensure assure servers applications response time perform operate within a specified limit constraint policy rule condition");

//Anup

 ContextGr(stdout, "retrieve return search find users customers clients booking reservation information data");
 ContextGr(stdout, "book movie film cinema tickets pass");
 ContextGr(stdout, "find locate film cinema movie theaters theatres");//issues:
 ContextGr(stdout, "get find film cinema movie reviews and sentiments");
 ContextGr(stdout, "get find theater theatres parking  information data");

//Mark

 ContextGr(stdout, "digital cinema film movie theater theatre");
 ContextGr(stdout, "analog cinema film movie theater theatre");
 ContextGr(stdout, "what did I do last Summer?");
 ContextGr(stdout, "learning about Norway");
 ContextGr(stdout, "looking for inspiration inspire");
 ContextGr(stdout, "in hotel room");

 
 // Added

 ContextGr(stdout, "geographical region location address");
 ContextGr(stdout, "geographical location address");
 ContextGr(stdout, "name internet location");
 ContextGr(stdout, "proper name");
 ContextGr(stdout, "qualify function role purpose");
 ContextGr(stdout, "name internet location");
 ContextGr(stdout, "films movies cinema searching");
 
// Roles etc.  invariant semantics vs more variable extensible contexts

// Usage:
// RoleGr( What's its name?, What role does it play?, What qualifying attributes?, In what context?)
 
RoleGr(stdout,"horror movie", "movie", "genre horror", "films movies cinema searching");
RoleGr(stdout,"thriller (movie)", "movie", "genre thriller", "films movies cinema searching");
RoleGr(stdout,"thriller (novel)", "novel", "genre thriller,book", "films movies cinema searching");
RoleGr(stdout,"drama movie", "movie", "genre drama", "films movies cinema searching");
RoleGr(stdout,"animation movie", "movie", "genre animation", "films movies cinema searching");
RoleGr(stdout,"documentary movie", "movie", "genre documentary", "films movies cinema searching");
RoleGr(stdout,"sci-fi movie", "movie", "genre sci-fi", "films movies cinema searching");


RoleGr(stdout,"Odeon theatre showing Jaws", "showing or presenting", "movie,film,showing,presentation,Jaws", "culture");
RoleGr(stdout,"Odeon theatre in New York City", "movie theatre", "find, movie,film,showing,presentation,", "geographical location address");
RoleGr(stdout,"New York City is 20 miles from Elizabeth NJ", "distance", "find, travel, distance", "location");
RoleGr(stdout,"20 miles from Elizabeth NJ", "distance", "Elizabeth NJ,miles,radius", "geographical region location address");

Gr(stdout, "distance measurement", a_name, "miles", "distances measurements");
Gr(stdout, "movie theatre", a_name, "Odeon", "proper name");

// RoleGr(stdout,"drama", "genre", "", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "movies", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "plays", "films movies cinema searching");
 Gr(stdout, "drama", a_contains, "novels", "fiction");
 
//THese are better modelled as clusters than these originals
//Gr(stdout, "Jaws restaurant", a_name, "Jaws", ContextClucter("proper name"));
//Gr(stdout, "sushi", a_name, "food type", "food");
//Gr(stdout, "New York City", a_name, "city", "location");
//Gr(stdout, "Elizabeth NJ", a_name, "city", "location");


// Qualifier
RoleGr(stdout,"jawssushi.com", "web address", "jawssushi restaurant", "name internet location");
RoleGr(stdout,"jawssushi restaurant", "restaurant", "Jaws,Sushi,sells food type sushi,jawssushi.com", "proper name of restaurant");
RoleGr(stdout,"Jaws movie", "movie", "Jaws,horror movie, thriller", "seems-to-be proper name of movie");
RoleGr(stdout,"sells food type sushi", "sells", "food type sushi", "function role purpose");
RoleGr(stdout,"food type sushi", "food type", "sushi", "food type");

// Names Geography
RoleGr(stdout,"New York City", "city", "New York", "proper name");
RoleGr(stdout,"Elizabeth NJ", "city", "Elizabeth", "proper name");
RoleGr(stdout,"New York State", "state", "New York", "proper name");
RoleGr(stdout,"State of New Jersey", "state", "New Jersey", "proper name");

RoleGr(stdout,"jaws sushi", "restaurant service", "food, restaurant", "restaurant");
RoleGr(stdout,"jaws sushi restaurant in Elizabeth NJ", "located", "find, food, restaurant", "location");

RoleGr(stdout,"Film festival", "festival", "movie,film", "entertainment event movies");

//Gr(stdout, "Jaws", a_name, "movie", "entertainment");
//Gr(stdout, "Jaws", a_name, "character in James Bond movie", "entertainment");


RoleGr(stdout,"jaws character in James Bond movie", "movie character", "jaws,character,James Bond movie", "searching for films movies cinema");
RoleGr(stdout,"jaws character is Steven Spielberg movie series", "jaws,character,movie character", "Jaws movie", "searching for films movies cinema");

// Service Phenomena


RoleGr(stdout,"web connections to jawssushi.com","web connections","jawssushi.com","browsing the web");
RoleGr(stdout,"jawssushi.com","sushi restaurant website","web,jaws,sushi","domain names internet search");

RoleGr(stdout,"web connections to jawssushi.com", "web connections", "jawssushi.com", "location");
Gr(stdout,"web connections for jaws", a_related_to, "web connections to jawssushi.com", "online web");
Gr(stdout,"web connections for jaws", a_related_to, "web searches for sushi", "online web");

// Causation ///////////////////////////////////////////////////////////////////////////////////////////////////

// Intentional motivation for state
RoleGr(stdout,"Interest in Film Festival","interest","Film festival,interested in","Browsing the web entertainment culture");
Gr(stdout, "web connections for jaws", a_caused_by, "Interest in Film Festival", "Browsing the web entertainment culture");


// state anomaly depends on an ingredient
RoleGr(stdout, "high web traffic","web traffic","high,traffic,web","operational state anomaly monitoring data");
Gr(stdout, "high web traffic", a_caused_by, "web connections", "online web");

RoleGr(stdout,"high web connections to jawssushi.com", "web connections to jawssushi.com", "high-anomaly,find, food, restaurant", "monitoring service performance");

// Derivative concept
RoleGr(stdout, "restaurant web traffic","web traffic","restaurant,traffic,web","operational state monitoring data");
Gr(stdout, "restaurant web traffic", a_caused_by, "web connections", "online web");

 // Specifically, monitoring tells us that ... (forming a temporary context bridge)
Gr(stdout, "high web traffic", a_caused_by, "web searches for sushi", "traffic load monitoring");
Gr(stdout, "high web traffic", a_caused_by, "web searches for jaws", "traffic load monitoring");




 return 0;
}

