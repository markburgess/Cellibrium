/*****************************************************************************/
/*                                                                           */
/* File: associations.h                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:08 2017                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct
{
   char fwd[CGN_BUFSIZE];
   char bwd[CGN_BUFSIZE];
   char context[CGN_BUFSIZE];
   time_t lastseen;
   double weight;
   
} LinkAssociation;
