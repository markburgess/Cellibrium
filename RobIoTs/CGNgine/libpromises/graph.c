/*****************************************************************************/
/*                                                                           */
/* File: graph.c (C) Mark Burgess                                            */
/*                                                                           */
/* Created: Mon Aug 29 10:30:55 2016                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef GRAPH
#define GRAPH 1
#include <cf3.defs.h>
#include <graph.h>
#include <rlist.h>
#include <syntax.h>
#include <fncall.h>
#include <sort.h>
#include <files_names.h>
#include <files_lib.h>

#include "graph_defs.c"

/**********************************************************************/

void MakeUniqueClusterName(char *lval,void *sorted,char type,char *buffer)

{
 switch (type)
    {
    case RVAL_TYPE_SCALAR:
        snprintf(buffer, CF_BUFSIZE, "%s.%s", lval, (char *)sorted);
        break;
    case RVAL_TYPE_LIST:

        buffer[0] = '\0';
        strcat(buffer,lval);

        if (sorted)
           {
           strcat(buffer,".");
           }

        for (Rlist *rp = (Rlist *)sorted; rp != NULL; rp=rp->next)
           {
           if (strlen(buffer) < CF_BUFSIZE / 2)
              {
              strcat(buffer,RlistScalarValue(rp));
              if (rp->next != NULL)
                 {
                 strcat(buffer,".");
                 }
              }
           }
        break;
    }
}

#endif
