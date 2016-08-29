/*****************************************************************************/
/*                                                                           */
/* File: graph.c                                                             */
/*                                                                           */
/* Created: Mon Aug 29 10:30:55 2016                                         */
/*                                                                           */
/*****************************************************************************/

#define GRAPH 1

#include <mod_files.h>
#include <graph.h>

/*****************************************************************************/

const int CONTAINS  = 1;  // bitwise for OR
const int FOLLOWS   = 2;
const int EXPRESSES = 4;
const int NEAR      = 8;

const int F = 0;
const int B = 1;


char *A[a_ass_dim+1][2] =
{
    {"seen concurrent with","seen concurrent with"},
    {"contains","is a member of"},
    {"is the name for","is referred to by"},
    {"is the source of","originates from"},
    {"has attribute","is an attribute of"},
    {"has value","is the value of"},
    {"has instance","is an instance of"},
    {"is approximately","is approximately"},
    {NULL, NULL},
};

/**********************************************************************/

void Gr(FILE *consc,char *from, int type, enum associations assoc, char *to)
{
 fprintf(consc,"(%s,%d,%s,%s,%s)\n",from,type,A[assoc][F],A[assoc][B],to);
}

/**********************************************************************/

void GrQ(FILE *consc,char *from, int type, enum associations assoc, double to)
{
 fprintf(consc,"(%s,%d,%s,%s,%.2lf)\n",from,type,A[assoc][F],A[assoc][B],to);
}


