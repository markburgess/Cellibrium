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
    array[i].context = NULL;
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
    free(array[i].context);
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
    array[i].context = strdup(context);
    array[i].concept = strdup(concept);

    //printf("FOUND(%s: %s(%s) in context %s(%s)\n", array[i].concept,fwd,array[i].fwd,context,array[i].context);
    }
}

/*****************************************************************************/

void WriteConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries)
{ int i;

 for (i = 0; (i < maxentries) && array[i].fwd; i++)
    {
    fprintf(fin, "(%s,%s,%ld,%lf,%s)\n",array[i].fwd,array[i].bwd,array[i].lastseen,array[i].weight,array[i].context);

    //printf("WROTE(%s <> %s at %s x %.2lf) context %s\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight,array[i].context);
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
 
 // (fwd,bwd,lastseen,weight)

 if (fwd[0] == '\0')
    {
    return;
    }

 InitializeAssociations(array);
 
 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d",BASEDIR,concept1,atype);
 mkdir(filename,0755);

 snprintf(filename,CGN_BUFSIZE,"%s/%s/%d/%s",BASEDIR,concept1,atype,concept2);

 if ((fp = fopen(filename,"r")) != NULL)
    {
    GetConceptAssociations(fp,filename,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }

 done = false;

 for (i = 0; (i < MAX_ASSOC_ARRAY) && array[i].fwd; i++)
    {
    if ((strcmp(fwd,array[i].fwd) == 0) && (strcmp(context,array[i].context) == 0)) // too strong/specific?
       {
       array[i].weight = (0.6+0.4*array[i].weight);
       array[i].lastseen = now;
       done = true;
       break;
       }
    }

 if (!done)
    {
    if (array[i].fwd)
       {
       free(array[i].fwd);
       }
    array[i].fwd = strdup(fwd);

    if (array[i].bwd)
       {
       free(array[i].bwd);
       }
    array[i].bwd = strdup(bwd);

    if (array[i].context)
       {
       free(array[i].context);
       }
    array[i].context = strdup(context);
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

