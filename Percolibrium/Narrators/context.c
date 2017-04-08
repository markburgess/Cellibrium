/*****************************************************************************/
/*                                                                           */
/* File: context.c                                                           */
/*                                                                           */
/* Created: Fri Mar 24 13:09:33 2017                                         */
/*                                                                           */
/*****************************************************************************/

#define MAX_CONTEXT 2048
#define POSDIR "/tmp/CellibriumPC"
#define NEGDIR "/tmp/CellibriumNC"

static char *Trim(char *s);
static void Chop(char *s);

// Context accumulation of "state of mind"

char *POSITIVE_CONTEXT[MAX_CONTEXT] = { NULL };
char *NEGATIVE_CONTEXT[MAX_CONTEXT] = { NULL };

/*****************************************************************************/

void InitializeStateOfMind(void)
{
 char filename[CGN_BUFSIZE];

 // Using idempotent files is the simplest way to get a hash table
 
 // positive context cache
 snprintf(filename,CGN_BUFSIZE,POSDIR);
 mkdir(filename,0755);
 
 // negative context cache
 snprintf(filename,CGN_BUFSIZE,NEGDIR);
 mkdir(filename,0755); 
  
}

/*****************************************************************************/

void AppendStateOfMind(char *mix, char *pos, char *neg)

{ char mixpos[CGN_BUFSIZE],mixneg[CGN_BUFSIZE], name[CGN_BUFSIZE];
  FILE *fp;
  int i;
  DIR *dirh;
  struct dirent *dirp;

  SplitPNContext(mix,mixpos,mixneg);

  // Map to an indempotent set, source of truth
  
  snprintf(name, CGN_BUFSIZE, "%s/%s", POSDIR,pos);

  if ((fp = fopen(name,"w")) != NULL)
     {
     fclose(fp);
     }

  snprintf(name, CGN_BUFSIZE, "%s/%s", NEGDIR,neg);

  if ((fp = fopen(name,"w")) != NULL)
     {
     fclose(fp);
     }

  snprintf(name, CGN_BUFSIZE, "%s/%s", POSDIR,mixpos);

  if ((fp = fopen(name,"w")) != NULL)
     {
     fclose(fp);
     }

  snprintf(name, CGN_BUFSIZE, "%s/%s", NEGDIR,mixneg);

  if ((fp = fopen(name,"w")) != NULL)
     {
     fclose(fp);
     }

  // Flush cache

  for (i = 0; i < MAX_CONTEXT; i++)
     {
     free(POSITIVE_CONTEXT[i]);
     free(NEGATIVE_CONTEXT[i]);
     }

  // Now cache in memory for efficiency

  i = 0;
  
  if ((dirh = opendir(POSDIR)) != NULL)
     {
     for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
        {
        if (dirp->d_name[0] == '.')
           {
           continue;
           }
        
        POSITIVE_CONTEXT[i++] = strdup(dirp->d_name);
        }
     
     closedir(dirh);
     }
  
  // Check explicitly negated

  i = 0;

  if ((dirh = opendir(NEGDIR)) != NULL)
     {
     for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
        {
        if (dirp->d_name[0] == '.')
           {
           continue;
           }
        
        NEGATIVE_CONTEXT[i++] = strdup(dirp->d_name);
        }
     
     closedir(dirh);
     }
}

/**********************************************************/

void SplitPNContext(char *context_opt,char *pos,char *neg)

{ char *sp;

 if (context_opt == NULL)
    {
    neg[0] = '\0';
    strcpy(pos,"any");
    }
 else if (sp = strstr(context_opt,"but not"))
    {
    strncpy(pos,context_opt,sp-context_opt);
    pos[sp-context_opt-1] = '\0';
    strcpy(neg,sp+strlen("but not "));
    }
 else
    {
    strcpy(pos,context_opt);
    strcpy(neg,"");
    }
}

/**********************************************************/

static char *Trim(char *s)
{
 char *sp;
 for (sp = s; *sp == ' ' && *sp != '\0'; sp++)
    {
    }

 return sp;
}

/**********************************************************/

static void Chop(char *s)
{
 char *sp;

 for (sp = s+strlen(s); *sp == ' ' && (sp > s); sp--)
    {
    *sp = '\0';
    }
}

/**********************************************************/

int RelevantToCurrentContext(char *concept,char *assoc,char *nextconcept,char *context)

{ int not = false;
  int relevance = 0;
  int i;
  double r, setcount = 0;
  
  for (i = 0; i < MAX_CONTEXT && POSITIVE_CONTEXT[i] != NULL; i++)
     {
     // If the next subject contains the search context 
     r += Overlap(POSITIVE_CONTEXT[i],nextconcept);
     
     // Finally, if the current context overlaps with the learnt context
     r += Overlap(POSITIVE_CONTEXT[i],context);

     setcount++;
     }

  // Check explicitly negated

  for (i = 0; i < MAX_CONTEXT && NEGATIVE_CONTEXT[i] != NULL; i++)
     {
     r -= Overlap(NEGATIVE_CONTEXT[i],context);
     setcount++;
     }

  relevance = (int) (r/setcount + 0.5);
  return relevance;
}

/**********************************************************/

int Overlap(char *intended, char *actual)

// In looking for conceptual overlap, the two contexts are not equal.
// The first is the intended one from the state of mind of the agent,
// the latter is the recorded actual context acquired during learning.
    
{ int i,j,k;
  double s = 0, t = 0, score = 0, total = 0;
  int end = false;
  double percent;
  char *atomI[MAX_CONTEXT] = {NULL};
  char *atomA[MAX_CONTEXT] = {NULL};

  const double frag_cutoff_threshold = 3; // min frag match

SplitCompound(intended,atomI); // Look at the learned relevance
SplitCompound(actual,atomA);   // Look at the current cognitive context

for (i = 0; i < MAX_CONTEXT && atomI[i]; i++)
   {
   for (j = 0; j < MAX_CONTEXT && atomA[j]; j++)
      {
      if (atomI[i] == NULL || atomA[j] == NULL)
         {
         continue;
         }

      s = t = 0;
      
      for (k = 0; k < MAX_WORD_SZ; k++)
         {
         if (atomI[i][k] == '\0') // intended is a substring of actual
            {
            //printf("match %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomA[j]+k);
            break;
            }
         
         if (atomA[j][k] == '\0') // actual is a substring of intended
            {
            //printf("match %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomI[i]+k);
            break;
            }

         if (atomI[i][k] != atomA[j][k]) // terminate on mismatch
            {
            //printf("mismatch %s %s\n", atomI[i], atomA[j]);
            t += strlen(atomA[j]+k);
            break;
            }
         else
            {
            s++;
            t++;
            }
         }

      if (s > frag_cutoff_threshold)
         {
         score += s;
         total += t;
         }
      else
         {
         // If there is no significant word overlap, then add the length of
         // the non-matching string to scale the fractional overlap
         total += strlen(atomI[i]);
         }
      }
   }

total = strlen(actual);

if (total > 0)
   {
   percent = score / total * 100.0;
   }
else
   {
   percent = 0;
   }

for (i = 0; i < MAX_CONTEXT; i++)
   {
   if (atomI[i])
      {
      free(atomI[i]);
      }
   if (atomA[i])
      {
      free(atomA[i]);
      }
   }

//printf("Overlap(%s/%s) = %d%% \n",intended,actual,(int)percent);
return (int)percent;
}

/**********************************************************/

void SplitCompound(char *str, char *atoms[MAX_CONTEXT])

// Split compound name into atoms that can permit a partial overlap match
    
{ char *sp = str;
  char word[255];
  int pos = 0;

 if (str == NULL)
    {
    return;
    }  
 
 while (*sp != '\0')
    {
    if (*sp == ' ' || *sp == ',')
       {
       sp++;
       continue;
       }
    
    word[0] = '\0';
    sscanf(sp,"%250[^ ,]",word);
    sp += strlen(word);

    if (pos < MAX_CONTEXT)
       {
       atoms[pos] = strdup(word);
       pos++;
       }
    }
}

