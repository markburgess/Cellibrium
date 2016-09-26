/*****************************************************************************/
/*                                                                           */
/* File: graph_defs.c                                                        */
/*                                                                           */
/* (C) Mark Burgess                                                          */
/*                                                                           */
/*****************************************************************************/

#define GR_CONTAINS  1 // for membership
#define GR_FOLLOWS   2 // i.e. influenced by
#define GR_EXPRESSES 4 // naming/represents - do not use to label membership, only exterior promises
#define GR_NEAR      8 // approx like

/* In this semantic basis, contains and exhibits are not completely orthogonal.
 Expresses implies an exterior promise from a sub-agent or from
 within, while contains alone is an interior agent membership property */

Association A[a_ass_dim+1] =
{
    {GR_CONTAINS,"contains","belongs to/is part of"},
    {GR_CONTAINS,"generalizes","is a special case of"},
    {GR_FOLLOWS,"originates from","is the source of"},
    {GR_FOLLOWS,"is maintained by","maintains"},
    {GR_FOLLOWS,"depends on","partly determines"},
    {GR_EXPRESSES,"is called","is a name for"},
    {GR_EXPRESSES,"expresses an attribute","is an attribute expressed by"},
    {GR_EXPRESSES,"has value/state","is the state/value of"},
    {GR_EXPRESSES,"has the role of","is a role fulfilled by"},
    {GR_EXPRESSES,"has function","is the function of"},
    {GR_EXPRESSES,"has constraint","constrains"},
    {GR_EXPRESSES,"has interpretation","is interpreted from"},
    {GR_NEAR,"seen concurrently with","seen concurrently with"},
    {GR_NEAR,"also known as","also known as"},
    {GR_NEAR,"is approximately","is approximately"},
    {GR_NEAR,"is related to","is related to"},
    {0, NULL, NULL},
};

/*****************************************************************************/

