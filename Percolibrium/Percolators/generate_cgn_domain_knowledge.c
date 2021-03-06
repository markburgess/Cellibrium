/*****************************************************************************/
/*                                                                           */
/* File: generate_cgn_domain_knowledge.c                                     */
/*                                                                           */
/* Created: Mon Nov 21 14:10:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

 // Conceptual sketch for convergently updating graph knowledge representation
 // This marries/matches with Narrators/stories-fs.c

 // gcc -o gen_cgndk -g -std=c99 generate_cgn_domain_knowledge.c

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

int main()
{
 Gr(stdout,"operating system",a_generalizes,"linux",ALL_CONTEXTS);

// These tuples might be generated by a code audit / scanner

 RoleGr(stdout,"CGNgine error messages","error messages", "CGNgine", "fault diagnosis system errors faults");
 
 Gr(stdout,"CGNgine error messages",a_contains,"Cannot mix CIDR notation with xxx-yyy range notation *",ContextGr(stdout,"system errors faults"));
 Gr(stdout,ContextGr(stdout,"CGNgine error messages"),a_contains,"Not a valid address range - or not a fully qualified name *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Cannot handle ipv6 masks which are not 8 bit multiples (fix me)","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Mixture of IPv6 and IPv4 addresses","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Cannot mix CIDR notation with xx-yy range notation","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"FIPS mode is enabled, and md5 is not an approved algorithm","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"process_select body missing its a process_result return value","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"bundle_return_value promise for * in bundle * with too many constraints (ignored)","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Unsupported database type * in databases promise","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Neighbour IP address should be a host not be a network address * in BGP interface promise","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Unsupported user policy * in users promise","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Error writing marker file *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Error removing marker file *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Failed to decrypt string","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Failed to repair database *, recreating...","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Host class counting is only available in CFEngine Enterprise","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Chop was called on a string that seemed to have no terminator","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Bundle aborted on defined class *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Setting abort for * when setting *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Unable to parse class expression *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Syntax error in expression *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"While persisting class *, error reading existing value","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Chop was called on a string that seemed to have no terminator","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Bundle aborted on defined class *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Setting abort for * when setting class *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"The value of variable * contains a reference to itself, *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"An item in list variable * contains a reference to itself","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Variable *' cannot be added because ","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"You cannot use reserved hard class * as post-condition class","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"You cannot cancel a reserved hard class * in post-condition classes","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Unable to open private log *","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"Could not get host entry age","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"getusers is not implemented","system errors faults");
Gr(stdout,"CGNgine error messages",a_contains,"FIPS mode is enabled and md5 is not an approved algorithm","system errors faults");

// Derivatives

Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_related_to,"CIDR",ALL_CONTEXTS);
Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_depends,"CIDR",ALL_CONTEXTS);
Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_depends,"xxx-yyy range notation",ALL_CONTEXTS);
Gr(stdout,"xxx-yyy range notation",a_alias,"range notation",ContextGr(stdout,"system policy syntax CGNgine"));

Gr(stdout,"CIDR",a_related_to,"Classless Internet Domain Routing","*");
Gr(stdout,"CIDR",a_related_to,"Routing","*");

Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_caused_by,"network policy","system errors faults");
Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_caused_by,"FuzzySetMatch","system errors faults");

// Trying to curate the story now....:( This is about information/documentation design
// We can try to make documentation causal instead of encyclopaedic

Gr(stdout,"Cannot mix CIDR notation with xxx-yyy range notation *", a_caused_by,"incorrect use of network pattern matching","system errors faults");

Gr(stdout,"incorrect use of network pattern matching",a_related_to,ContextGr(stdout,"network pattern matching"), "system errors faults");

Gr(stdout,ContextGr(stdout,"pattern matching"), a_contains, "network pattern matching","*");
Gr(stdout,"pattern matching", a_contains, "wildcards","*");
Gr(stdout,"pattern matching", a_contains, "regular expressions","*");


Gr(stdout, "function iprange" ,a_uses,"FuzzySetMatch","software");
Gr(stdout, "function iprange" ,a_promises,"network pattern matching","software");


Gr(stdout, "function iprange" ,a_hasrole,"CGNgine class function","software");
Gr(stdout, "function iprange" ,a_hasrole,"host classifier","software");
Gr(stdout, "function iprange" ,a_hasattr,"host","software");
Gr(stdout, "function iprange" ,a_hasattr,"host classifier","software");

Gr(stdout, "label promise at line 25 of file promises.cf" ,a_uses,"function iprange","system policy declaration");

Gr(stdout, "class or context label" ,a_alias,"host classifier","system monitoring");

Gr(stdout, "system policy", a_contains, "CGNgine class function","software");
Gr(stdout, "CGNgine class function", a_contains,"CGNgine function",ALL_CONTEXTS);
Gr(stdout, "CGNgine policy language", a_contains, "CGNgine functions",ALL_CONTEXTS);
Gr(stdout,"network policy",a_contains,"interfaces configuration","computer software CGNgine");

return 0;
}
