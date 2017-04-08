/*****************************************************************************/
/*                                                                           */
/* File: context.h                                                           */
/*                                                                           */
/* Created: Fri Mar 24 13:09:52 2017                                         */
/*                                                                           */
/*****************************************************************************/

void AppendStateOfMind(char *old, char *pos, char *neg);
void SplitPNContext(char *context_opt,char *pos,char *neg);
int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context);
void SplitCompound(char *str, char *atoms[MAX_CONTEXT]);
int Overlap(char *set1, char *set2);
