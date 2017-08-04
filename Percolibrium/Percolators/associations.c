/*****************************************************************************/
/*                                                                           */
/* File: associations.c                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:02 2017                                         */
/*                                                                           */
/*****************************************************************************/

void WriteConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries);

/*****************************************************************************/

void InitializeAssociations(LinkAssociation *array)

{ int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    array[i].fwd = NULL;
    array[i].bwd = NULL;
    array[i].icontext = NULL;
    array[i].concept = NULL;
    array[i].weight = 0;
    array[i].lastseen = 0;
    array[i].relevance = 0;
    }
}

/*****************************************************************************/

void DeleteAssociations(LinkAssociation *array)

{ int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    free(array[i].fwd);
    free(array[i].bwd);
    free(array[i].icontext);
    free(array[i].concept);
    }
}

/*****************************************************************************/

void GetConceptAssociations(FILE *fin, char *concept, LinkAssociation *array,int maxentries)

{ int i;
 char fwd[CGN_BUFSIZE];
 char bwd[CGN_BUFSIZE];
 char context[CGN_BUFSIZE];

 InitializeAssociations(array);
 
 for (i = 0; !feof(fin); i++)
    {
    if (i >= maxentries)
       {
       return;
       }

    fwd[0] = '\0';
    bwd[0] = '\0';
    context[0] = '\0';
            
    fscanf(fin, "(%[^,],%[^,],%ld,%lf,%[^)])\n",fwd,bwd,&(array[i].lastseen),&(array[i].weight),context);

    array[i].fwd = strdup(fwd);
    array[i].bwd = strdup(bwd);
    array[i].icontext = strdup(context);
    array[i].concept = strdup(concept);

    //printf("FOUND(%s: %s(%s) in context %s(%s)\n", array[i].concept,fwd,array[i].fwd,context,array[i].icontext);
    }
}

/*****************************************************************************/

void WriteConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries)
{ int i;

 for (i = 0; (i < maxentries) && array[i].fwd; i++)
    {
    fprintf(fin, "(%s,%s,%ld,%lf,%s)\n",array[i].fwd,array[i].bwd,array[i].lastseen,array[i].weight,array[i].icontext);

    //printf("WROTE(%s <> %s at %s x %.2lf) context %s\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight,array[i].icontext);
    }
}

/*****************************************************************************/

void UpdateAssociation(char *context, char *concept1, int atype, char *fwd, char *bwd, char *concept2)
{
 char filename[CGN_BUFSIZE];
 LinkAssociation array[MAX_ASSOC_ARRAY];
 FILE *fp;
 int i, done;
 time_t now = time(NULL);
 extern char VBASEDIR[256];
 
 // (fwd,bwd,lastseen,weight)

 if (fwd[0] == '\0')
    {
    return;
    }

 InitializeAssociations(array);
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",VBASEDIR,concept1,atype);
 mkdir(filename,0755);

 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",VBASEDIR,concept1,atype,concept2);

 if ((fp = fopen(filename,"r")) != NULL)
    {
    GetConceptAssociations(fp,filename,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }

 done = false;

 // Update all the aliases for this association
 
 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd != NULL); i++)
    {
    if ((strcmp(fwd,array[i].fwd) == 0))
       {
       array[i].weight = (0.6+0.4*array[i].weight);
       array[i].lastseen = now;
       done = true;
       break;
       }
    }

 // If Not already registered, add a new line
 
 if (!done && (i < MAX_ASSOC_ARRAY))
    {
    array[i].fwd = strdup(fwd);
    array[i].bwd = strdup(bwd);
    array[i].icontext = strdup(context);
    array[i].weight = 0.7;
    array[i].lastseen = now;
    array[i].relevance = 99;
    }

 if ((fp = fopen(filename,"w")) != NULL)
    {
    WriteConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }
}

