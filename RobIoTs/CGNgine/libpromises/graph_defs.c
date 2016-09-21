/*****************************************************************************/
/*                                                                           */
/* File: graph_defs.c                                                        */
/*                                                                           */
/* (C) Mark Burgess                                                          */
/*                                                                           */
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
    {"contains","belongs to"},
    {"is a name for","is called"},
    {"is the source of","originates from"},
    {"has attribute","is an attribute of"},
    {"has value","is the value of"},
    {"has a specific instance","is a specific instance of"},
    {"is approximately","is approximately"},
    {"maintains as proxy","is maintained by"},
    {"depends on","partly determines"},
    {"has function","is the function of"},
    {"has constraint","constrains"},
    {"also known as","also known as"},
    {"has interpretation","is interpreted from"},
    {NULL, NULL},
};

/*****************************************************************************/

