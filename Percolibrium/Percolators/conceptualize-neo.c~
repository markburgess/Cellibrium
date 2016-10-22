/*****************************************************************************/
/*                                                                           */
/* File: conceptualize.c                                                     */
/*                                                                           */
/* Created: Wed Sep 21 11:17:46 2016                                         */
/*                                                                           */
/*****************************************************************************/

//  gcc -o conceptualize conceptualize.c
//  ./conceptualize /var/CGNgine/state/env_graph 

#include <stdio.h>

#define GRAPH 1
#define PERCOLATION 1

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/*****************************************************************************/

main(int argc, char **argv)
{
 FILE *fp;
 char line[1024];
 char from[256], to[256], fwd[256], bwd[256];
 int assoctype;

 if (argc != 2)
    {
    printf("Syntax: conceptualize <filename>\n");
    return 0;
    }
 
 if ((fp = fopen(argv[1], "r")) == NULL)
    {
    perror("open");
    return;
    }

 while (!feof(fp))
    {
    from[0] = to[0] = fwd[0] = bwd[0] = '\0';
    assoctype = 0;
    
    fgets(line, 1023, fp);
    sscanf(line, "(%[^,],%d,%[^,],%[^,],%[^)])",from,&assoctype,fwd,to,bwd);

    printf("CREATE (:concept {description:'%s'}),  (:concept {description:'%s'}),\n",from,to);
    printf("(%s)-[:'%s' {type: %d, b:'%s'}]->(%s);\n",from,fwd,assoctype,bwd,to);
    }
 
 fclose(fp);
}
