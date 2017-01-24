/*****************************************************************************/
/*                                                                           */
/* File: associations.c                                                      */
/*                                                                           */
/* Created: Tue Jan 10 08:32:02 2017                                         */
/*                                                                           */
/*****************************************************************************/


/*****************************************************************************/

void InitializeAssociations(LinkAssociation *array)

{ int i;
 
 for (i = 0; i < MAX_ASSOC_ARRAY; i++)
    {
    array[i].fwd[0] = '\0';
    array[i].bwd[0] = '\0';
    array[i].context[0] = '\0';
    array[i].weight = 0;
    array[i].lastseen = 0;
    }
}
/*****************************************************************************/

void GetConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries)

{ int i;

 InitializeAssociations(array);
 
 for (i = 0; (i < maxentries) && !feof(fin); i++)
    {
    fscanf(fin, "(%[^,],%[^,],%ld,%lf,%[^)])\n",array[i].fwd,array[i].bwd,&(array[i].lastseen),&(array[i].weight),array[i].context);

    //printf("FOUND(%s <> %s at %s x %.2lf) context %s\n", array[i].fwd,array[i].bwd,ctime(&(array[i].lastseen)),array[i].weight,array[i].context);
    }
}

/*****************************************************************************/

void UpdateConceptAssociations(FILE *fin, LinkAssociation *array,int maxentries)

{ int i;

 for (i = 0; (i < maxentries) && (array[i].fwd[0] != '\0'); i++)
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
    GetConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    }

 done = false;

 for (i = 0; (i < MAX_ASSOC_ARRAY) && (array[i].fwd[0] != '\0'); i++)
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
    strcpy(array[i].fwd,fwd);
    strcpy(array[i].bwd,bwd);
    strcpy(array[i].context,context);
    array[i].weight = 0.7;
    array[i].lastseen = now;
    }

 if ((fp = fopen(filename,"w")) != NULL)
    {
    UpdateConceptAssociations(fp,array,MAX_ASSOC_ARRAY);
    fclose(fp);
    //printf("Wrote .. %s\n", filename);
    }
}

