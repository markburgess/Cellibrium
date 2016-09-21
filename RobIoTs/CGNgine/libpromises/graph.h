/*****************************************************************************/
/*                                                                           */
/* File: graph.h                                                             */
/*                                                                           */
/* Created: Mon Aug 29 10:32:25 2016                                         */
/*                                                                           */
/*****************************************************************************/

enum associations
{
    a_concurrent,
    a_contains,
    a_name,
    a_origin,
    a_hasattr,
    a_hasvalue,
    a_hasinstance,
    a_approx,
    a_maintains,
    a_depends,
    a_hasfunction,
    a_hasconstraint,
    a_alias,
    a_interpreted,
    a_ass_dim
};

void Gr(FILE *consc,char *from, int type, enum associations assoc, char *to);
void IGr(FILE *consc,char *from, int type, enum associations assoc, char *to);
void GrQ(FILE *consc,char *from, int type, enum associations assoc, double to);
void GenerateSemanticsGraph(Policy *policy);

/*****************************************************************************/

#ifndef GRAPH
extern const int CONTAINS;
extern const int FOLLOWS;
extern const int EXPRESSES;
extern const int NEAR;

extern const int F;
extern const int B1;
extern char *A[a_ass_dim+1][2];

#endif
