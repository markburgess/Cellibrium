/*****************************************************************************/
/*                                                                           */
/* File: graph.h                                                             */
/*                                                                           */
/* Created: Mon Aug 29 10:32:25 2016                                         */
/*                                                                           */
/*****************************************************************************/

enum associations
{
    a_contains,
    a_generalizes,
    a_origin,
    a_maintainedby,
    a_depends,
    a_caused_by,
    a_uses,
    a_name,
    a_hasattr,  // avoid this, it says nothing unambiguously
    a_promises, 
    a_hasinstance,
    a_hasvalue,
    a_hasarg,
    a_hasrole,     // what is its function/object type? e.g. file,process..
    a_hasoutcome,
    a_hasfunction,
    a_hasconstraint,
    a_interpreted,
    a_concurrent,
    a_alias,
    a_approx,
    a_related_to,
    a_ass_dim
};

typedef struct
{
   int type;
   char *fwd;
   char *bwd;
}
Association;

#ifndef PERCOLATION
void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context);
void IGr(FILE *consc,char *from, enum associations assoc, char *to, char *context);
void GrQ(FILE *consc,char *from, enum associations assoc, double to, char *context);
void Number(FILE *consc, double q, char *context);
char *RoleCluster(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context);
char *ContextCluster(FILE *consc,char *compound_name);
#endif

/*****************************************************************************/

#ifndef GRAPH
extern Association A[a_ass_dim+1];
#endif

#define GR_CONTAINS  3 
#define GR_FOLLOWS   2 // i.e. influenced by
#define GR_EXPRESSES 4 // represents, etc
#define GR_NEAR      1 // approx like
#define GR_CONTEXT   5 // approx like
#define ALL_CONTEXTS "searching looking"

