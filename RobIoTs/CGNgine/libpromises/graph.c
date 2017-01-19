/*****************************************************************************/
/*                                                                           */
/* File: graph.c (C) Mark Burgess                                            */
/*                                                                           */
/* Created: Mon Aug 29 10:30:55 2016                                         */
/*                                                                           */
/*****************************************************************************/

#define GRAPH 1

#include <policy.h>
#include <mod_files.h>
#include <graph.h>
#include <rlist.h>
#include <syntax.h>
#include <fncall.h>
#include <sort.h>
#include <files_names.h>
#include <files_lib.h>

static void MakeUniqueClusterName(char *lval,void *sorted,char type,char *buffer);
static char *SanitizeString(char *s);

#include "graph_defs.c"

/**********************************************************************/

void GenerateSemanticsGraph(Policy *policy)
{
 // Describe the semantics of the policy structure
 // Seq policy->bundles, policy->bodies

 // Not entirely satisfactory - still seems fragile in construction
 // due to the presence of assumed schema

 char umbrella[CF_BUFSIZE], filename[CF_BUFSIZE];
 snprintf(filename, CF_BUFSIZE, "%s/policy_graph", CFWORKDIR);
 MapName(filename);

 FILE *consc;

 if ((consc = fopen(filename, "w")) == NULL)
    {
    return;
    }

 Gr(consc,"system policy", a_contains,"promise bundle","system policy");
 Gr(consc,"system policy", a_related_to,"promise","");
 Gr(consc,"system policy", a_contains,"namespace","system policy");
 Gr(consc,"promise bundle", a_contains,"promise","system policy");
 Gr(consc,"promise", a_contains,"promise type","system policy");
 Gr(consc,"promise", a_contains,"promise handle","system policy");
 Gr(consc,"promise", a_contains,"constraint","system policy");
 Gr(consc,"promise", a_contains,"promisee","system policy");
 Gr(consc,"constraint", a_contains,"lval","system policy");
 Gr(consc,"constraint", a_contains,"rval","system policy");
 Gr(consc,"compound rval", a_alias, "body group","system policy");
 Gr(consc,"compound rval", a_alias, "constraints body","system policy");

 for (size_t i = 0; i < SeqLength(policy->bundles); i++)
    {
    const Bundle *bundle = SeqAt(policy->bundles, i);
    Gr(consc,bundle->ns,a_contains,bundle->name,"policy namespace");
    Gr(consc,bundle->ns,a_hasrole,"namespace","policy namespace");
    Gr(consc,bundle->name,a_hasrole,"promise bundle","system policy");
    Gr(consc,bundle->type,a_maintainedby,bundle->name,"system policy");

    if (bundle->source_path)
       {
       Gr(consc,bundle->source_path,a_contains,bundle->name,"system policy file");
       Gr(consc,bundle->source_path,a_hasrole,"policy file","system policy file");
       }

    Rlist *argp = NULL;

    for (argp = bundle->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,bundle->name,a_depends,RlistScalarValue(argp),"system promise bundle");
       }

    for (size_t i = 0; i < SeqLength(bundle->promise_types); i++)
       {
       PromiseType *type = SeqAt(bundle->promise_types, i);

       for (size_t ppi = 0; ppi < SeqLength(type->promises); ppi++)
          {
          Promise *promise = SeqAt(type->promises, ppi);

          char *handle = (char *)PromiseGetHandle(promise);

          if (handle == NULL)
             {
             handle = CanonifyName(promise->promiser);
             }

          // Promise type
          Gr(consc,handle,a_hasrole,type->name,"system promise handle");
          Gr(consc,handle,a_hasfunction,type->name,"system promise handle");
          Gr(consc,handle,a_contains,promise->promiser,"system promise handle");
          Gr(consc,promise->promiser,a_maintainedby,bundle->type,"system promise agent type");
          Gr(consc,promise->promiser,a_hasrole,"promiser","system promise role");

          Gr(consc,bundle->name,a_contains,handle,"system promise bundle");
          Gr(consc,handle,a_maintainedby,bundle->type,"system promise handle");
          Gr(consc,handle,a_hasrole,"promise handle","system promise handle");
          Gr(consc,handle,a_hasfunction,"promise handle","system promise handle");

          if (promise->comment)
             {
             Gr(consc,handle,a_hasattr,promise->comment,"system promise handle");
             }

          switch (promise->promisee.type)
             {
             case RVAL_TYPE_SCALAR:
                 Gr(consc,handle,a_contains,promise->promisee.item,"system promisee");
                 Gr(consc,(char *)promise->promisee.item,a_depends,promise->promiser,"system promiser");
                 Gr(consc,(char *)promise->promisee.item,a_depends,handle,"system promisee");
                 break;

             case RVAL_TYPE_LIST:
                 for (const Rlist *rp = promise->promisee.item; rp; rp = rp->next)
                    {
                    Gr(consc,handle,a_contains,RlistScalarValue(rp),"system promisee");
                    Gr(consc,RlistScalarValue(rp),a_depends, promise->promiser,"system promisee");
                    Gr(consc,RlistScalarValue(rp),a_depends, handle,"system promise handle");
                    }
                 break;

             default:
                break;
             }

          // Class activation

          Gr(consc,promise->classes,a_hasrole,"classifier or context label","system promise context precondition");

          for (size_t cpi = 0; cpi < SeqLength(promise->conlist); cpi++)
             {
             Constraint *constraint = SeqAt(promise->conlist, cpi);

             switch (constraint->rval.type)
                {
                case RVAL_TYPE_SCALAR:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_SCALAR,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella,"system promise");
                    Gr(consc,umbrella,a_hasrole,"promise body constraint","system promise constraint");
                    Gr(consc,umbrella,a_hasattr,constraint->rval.item,"system promise constraint");
                    Gr(consc,constraint->lval, a_hasrole, "lval","system promise constraint type");
                    Gr(consc,constraint->rval.item, a_hasrole, "rval","system promise constraint value");
                    Gr(consc,constraint->rval.item, a_interpreted, constraint->lval,"system promise constraint value");
                    break;

                case RVAL_TYPE_LIST:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella,"system promise");
                    Gr(consc,umbrella,a_hasrole,"promise body constraint","system promise constraint");
                    Gr(consc,umbrella,a_hasattr,constraint->lval,"system promise constraint");
                    Gr(consc,constraint->lval, a_hasrole, "lval","system promise constraint type");

                    for (Rlist *rp = (Rlist *)constraint->rval.item; rp != NULL; rp=rp->next)
                       {
                       Gr(consc,umbrella,a_hasattr,RlistScalarValue(rp),"system promise constraint");
                       Gr(consc,RlistScalarValue(rp), a_hasrole, "rval","system promise constraint value");
                       Gr(consc,RlistScalarValue(rp), a_interpreted, constraint->lval,"system promise constraint value");
                       }
                    break;

                case RVAL_TYPE_FNCALL:
                    {
                    FnCall *fp = (FnCall *)(constraint->rval).item;
                    
                    MakeUniqueClusterName(fp->name,fp->args,RVAL_TYPE_LIST,umbrella);

                    Gr(consc,handle,a_depends,umbrella,"system promise");
                    Gr(consc,umbrella,a_hasattr,fp->name,"system promise constraint");
                    Gr(consc,umbrella,a_hasattr,constraint->lval,"system promise constraint");
                    
                    Gr(consc,constraint->lval, a_hasrole, "lval","system promise constraint type");
                    Gr(consc,fp->name, a_hasrole, "rval","system promise constraint value function");
                    Gr(consc,fp->name, a_interpreted, constraint->lval,"system promise constraint value function");

                    Gr(consc,handle,a_uses,fp->name,"system promise");
                    
                    Gr(consc,promise->promiser,a_depends,umbrella,"system promise promiser");
                    Gr(consc,fp->name, a_hasrole,"function","system promise constraint value function");
                       
                    for (Rlist *argp = fp->args; argp != NULL; argp = argp->next)
                       {
                       Gr(consc,umbrella,a_depends,RlistScalarValue(argp),"system promise constraint");
                       Gr(consc,umbrella,a_hasarg,RlistScalarValue(argp),"promise promise constraint");
                       Gr(consc,RlistScalarValue(argp),a_hasrole,"argument parameter","promise constraint value function");
                       }
                    }
                    break;
                default:
                    break;
                }
             }
          }
       }
    }

 // Constraint Body parts

 for (size_t i = 0; i < SeqLength(policy->bodies); i++)
    {
    const Body *body = SeqAt(policy->bodies, i);

    MakeUniqueClusterName(body->name,body->args,RVAL_TYPE_LIST,umbrella);

    Gr(consc,body->name,a_hasrole,body->type,"promise compound constraint template");
    Gr(consc,umbrella,a_hasrole,body->type,"promise compound constraint");

    Gr(consc,umbrella,a_hasrole,"promise body constraint","promise compound constraint");
    Gr(consc,body->name,a_hasrole,"promise body constraint","promise compound constraint template");

    Gr(consc,body->ns,a_contains,body->name,"policy namespace");
    Gr(consc,body->ns,a_hasrole,"namespace","policy namespace");

    if (body->source_path)
       {
       Gr(consc,body->source_path,a_contains,body->name,"promise compound constraint template source");
       Gr(consc,body->source_path,a_contains,umbrella,"promise compound constraint template source");
       Gr(consc,body->source_path,a_hasrole,"file","promise compound constraint template source");
       }

    for (Rlist *argp = body->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,umbrella,a_depends,RlistScalarValue(argp),"promise compound constraint");
       Gr(consc,umbrella,a_hasarg,RlistScalarValue(argp),"promise compound constraint");
       Gr(consc,RlistScalarValue(argp),a_hasrole,"argument parameter","promise compound constraint argument");
       }
    }

 fclose(consc);
}

/**********************************************************************/

void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *sto = SanitizeString(to);
 char *scontext = SanitizeString(context);
 
 if (context && strlen(context) > 0)
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,sto,A[assoc].bwd,scontext);
    }
 else
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,sto,A[assoc].bwd,"*");
    }

 free(sfrom);
 free(sto);
 free(scontext);
}

/**********************************************************************/

void IGr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *sto = SanitizeString(to);
 char *scontext = SanitizeString(context);

 fprintf(consc,"(%s,-%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].bwd,sto,A[assoc].fwd,scontext);

 free(sfrom);
 free(sto);
 free(scontext);
}

/**********************************************************************/

void Number(FILE *consc, double q, char *context)
{
 enum associations assoc = a_hasrole;
 fprintf(consc,"(%.2lf,%d,%s,%s,%s,%s)\n",q,A[assoc].type,A[assoc].fwd,"number",A[assoc].bwd,context);
}

/**********************************************************************/

void GrQ(FILE *consc,char *from, enum associations assoc, double to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *scontext = SanitizeString(context);

 fprintf(consc,"(%s,%d,%s,%.2lf,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,scontext);

 free(sfrom);
 free(scontext); 
}

/**********************************************************************/

char *RoleCluster(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context)

/* Document a compound Split a comma separated list, with head
   we can use it for context or for conceptual
   RoleCluster(fp, "compound name", "hasrole unique identifier", "hasttr part1,hasttr part2", "naming unique identity")
*/
    
{ char *sp, word[255];

 Gr(consc,compound_name,a_hasrole,role,ex_context);
 
 if ((sp = attributes))
    {
    while (*sp != '\0')
       {
       if (*sp == ',')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250[^,]",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_hasattr,word,"all contexts");
       }
    }

return compound_name;
}

/**********************************************************************/

char *ContextCluster(FILE *consc,char *compound_name)

/* Document a compound Split a space separated list, with head
   we can use it for context or for conceptual - treat them as epitopes
   for fuzzy matching by set overlap. Only type 1 associations. */
    
{ char *sp, word[255];

 if ((sp = compound_name))
    {
    while (*sp != '\0')
       {
       if (*sp == ' ')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250s",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_contains,word,"all contexts");
       }
    }

return compound_name;
}

/**********************************************************************/

static void MakeUniqueClusterName(char *lval,void *sorted,char type,char *buffer)

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

/**********************************************************************/

static char *SanitizeString(char *s)
{
 if (s == NULL)
    {
    return NULL;
    }
 
 char *sp, *str = strdup(s);
 for (sp = str; *sp != '\0'; sp++)
    {
    switch (*sp)
       {
       case ',':
       case '/':
       case '\\':
           *sp = '_';
           break;
       default:
           break;
       }
    }
 return str;
}
