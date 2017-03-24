/*****************************************************************************/
/*                                                                           */
/* File: context.c                                                           */
/*                                                                           */
/* Created: Fri Mar 24 13:09:33 2017                                         */
/*                                                                           */
/*****************************************************************************/

static char *Trim(char *s);
static void Chop(char *s);
static void sstrcat(char *s,char *n);

// Context accumulation of "state of mind"

char *POSITIVE_CONTEXT = NULL;
char *NEGATIVE_CONTEXT = NULL;


/*****************************************************************************/

void AppendStateOfMind(char *mix, char *pos, char *neg)

{ char mixpos[CGN_BUFSIZE],mixneg[CGN_BUFSIZE];
 char *oldpos, *oldneg, *sp;

  // Don't pass NULL, pass "" for empty
  // append new / possibly mixed context data into the current +/- global state

  oldpos = POSITIVE_CONTEXT;
  oldneg = NEGATIVE_CONTEXT;

  if (oldpos == NULL)
     {
     oldpos = strdup("");
     }

  if (oldneg == NULL)
     {
     oldneg = strdup("");
     }
  
  SplitPNContext(mix,mixpos,mixneg);

  POSITIVE_CONTEXT = malloc(strlen(oldpos)+strlen(mixpos)+strlen(pos)+2);
  POSITIVE_CONTEXT[0] = '\0';

  strcat(POSITIVE_CONTEXT,Trim(pos));
  Chop(POSITIVE_CONTEXT);

  sstrcat(POSITIVE_CONTEXT,Trim(oldpos));
  Chop(POSITIVE_CONTEXT);

  sstrcat(POSITIVE_CONTEXT,Trim(mixpos));
  Chop(POSITIVE_CONTEXT);
      
  NEGATIVE_CONTEXT = malloc(strlen(oldneg)+strlen(mixneg)+strlen(neg)+2);
  NEGATIVE_CONTEXT[0] = '\0';
  
  strcat(NEGATIVE_CONTEXT,Trim(neg));
  Chop(NEGATIVE_CONTEXT);
  
  sstrcat(NEGATIVE_CONTEXT,Trim(oldneg));
  Chop(NEGATIVE_CONTEXT);

  sstrcat(NEGATIVE_CONTEXT,Trim(mixneg));
  Chop(NEGATIVE_CONTEXT);
  
  free(oldpos);
  free(oldneg);
}

/**********************************************************/

void SplitPNContext(char *CONTEXT_OPT,char *pos,char *neg)

{ char *sp;

 if (sp = strstr(CONTEXT_OPT,"but not"))
    {
    strncpy(pos,CONTEXT_OPT,sp-CONTEXT_OPT);
    pos[sp-CONTEXT_OPT-1] = '\0';
    strcpy(neg,sp+strlen("but not "));
    }
 else
    {
    strcpy(pos,CONTEXT_OPT);
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

static void sstrcat(char *s,char *n)
{
 if (strlen(s) > 0 && n[0] != '\0')
    {
    strcat(s," ");
    }
 strcat(s,n);
}

