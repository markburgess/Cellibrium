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
 Gr(consc,"promise", a_contains,"constraint","system policy");
 Gr(consc,"promise", a_contains,"promisee","system policy");
 Gr(consc,"constraint", a_contains,"lval","system policy");
 Gr(consc,"constraint", a_contains,"rval","system policy");
 Gr(consc,"rval", a_hasrole, "compound rval","system policy");
 Gr(consc,"compound rval", a_alias, "body group","system policy");
 Gr(consc,"compound rval", a_alias, "constraints body","system policy");

 for (size_t i = 0; i < SeqLength(policy->bundles); i++)
    {
    const Bundle *bundle = SeqAt(policy->bundles, i);
    Gr(consc,bundle->ns,a_contains,bundle->name,"system policy");
    Gr(consc,bundle->ns,a_hasrole,"namespace","system policy");
    Gr(consc,bundle->name,a_hasrole,"promise bundle","system policy");
    Gr(consc,bundle->type,a_maintainedby,bundle->name,"system policy");

    if (bundle->source_path)
       {
       Gr(consc,bundle->source_path,a_contains,bundle->name,"system policy");
       Gr(consc,bundle->source_path,a_hasrole,"policy file","system policy");
       }

    Rlist *argp = NULL;

    for (argp = bundle->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,bundle->name,a_depends,RlistScalarValue(argp),"system policy");
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
          Gr(consc,handle,a_hasfunction,type->name,"system policy");
          Gr(consc,handle,a_contains,promise->promiser,"system policy");
          Gr(consc,promise->promiser,a_maintainedby,bundle->type,"system policy");
          Gr(consc,promise->promiser,a_hasrole,"promiser","system policy");

          Gr(consc,bundle->name,a_contains,handle,"system policy");
          Gr(consc,handle,a_maintainedby,bundle->type,"system policy");
          Gr(consc,handle,a_hasrole,"promise handle","system policy");
          Gr(consc,handle,a_hasfunction,"promise","system policy");

          if (promise->comment)
             {
             Gr(consc,handle,a_hasattr,promise->comment,"promise");
             }

          switch (promise->promisee.type)
             {
             case RVAL_TYPE_SCALAR:
                 Gr(consc,handle,a_contains,promise->promisee.item,"promise");
                 Gr(consc,(char *)promise->promisee.item,a_depends,promise->promiser,"promise");
                 Gr(consc,(char *)promise->promisee.item,a_depends,handle,"promise");
                 break;

             case RVAL_TYPE_LIST:
                 for (const Rlist *rp = promise->promisee.item; rp; rp = rp->next)
                    {
                    Gr(consc,handle,a_contains,RlistScalarValue(rp),"promise");
                    Gr(consc,RlistScalarValue(rp),a_depends, promise->promiser,"promise");
                    Gr(consc,RlistScalarValue(rp),a_depends, handle,"promise");
                    }
                 break;

             default:
                break;
             }

          // Class activation
          Gr(consc,handle,a_depends,promise->classes,"promise");
          Gr(consc,promise->classes,a_hasrole,"classifier or context label","promise");

          for (size_t cpi = 0; cpi < SeqLength(promise->conlist); cpi++)
             {
             Constraint *constraint = SeqAt(promise->conlist, cpi);

             switch (constraint->rval.type)
                {
                case RVAL_TYPE_SCALAR:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_SCALAR,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella,"promise");
                    Gr(consc,umbrella,a_hasrole,"promise body constraint","promise");
                    Gr(consc,umbrella,a_hasattr,constraint->lval,"promise");
                    Gr(consc,umbrella,a_hasattr,constraint->rval.item,"promise");
                    Gr(consc,constraint->lval, a_hasrole, "lval","promise");
                    Gr(consc,constraint->rval.item, a_hasrole, "rval","promise");
                    break;

                case RVAL_TYPE_LIST:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella,"promise");
                    Gr(consc,umbrella,a_hasrole,"promise body constraint","promise");
                    Gr(consc,umbrella,a_hasattr,constraint->lval,"promise");
                    Gr(consc,constraint->lval, a_hasrole, "lval","promise");

                    for (Rlist *rp = (Rlist *)constraint->rval.item; rp != NULL; rp=rp->next)
                       {
                       Gr(consc,umbrella,a_hasattr,RlistScalarValue(rp),"promise");
                       Gr(consc,RlistScalarValue(rp), a_hasrole, "rval","promise");
                       }
                    break;

                case RVAL_TYPE_FNCALL:
                    {
                    FnCall *fp = (FnCall *)(constraint->rval).item;
                    MakeUniqueClusterName(fp->name,fp->args,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella,"promise");
                    Gr(consc,constraint->lval, a_hasrole, "lval","promise");
                    Gr(consc,fp->name, a_hasrole, "rval","promise");
                    Gr(consc,handle,a_depends,umbrella,"promise");
                    Gr(consc,umbrella,a_depends,fp->name,"promise");

                    Gr(consc,promise->promiser,a_depends,umbrella,"promise");
                    Gr(consc,fp->name, a_hasrole,"function","promise");
                    Gr(consc,fp->name,a_generalizes,umbrella,"promise");
                    Gr(consc,umbrella,a_hasattr,constraint->lval,"promise");
                       
                    for (Rlist *argp = fp->args; argp != NULL; argp = argp->next)
                       {
                       Gr(consc,umbrella,a_depends,RlistScalarValue(argp),"promise");
                       Gr(consc,umbrella,a_hasarg,RlistScalarValue(argp),"promise");
                       Gr(consc,RlistScalarValue(argp),a_hasrole,"argument parameter","promise");                       
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

    Gr(consc,body->name,a_hasrole,body->type,"promise");
    Gr(consc,umbrella,a_hasrole,body->type,"promise");

    Gr(consc,umbrella,a_hasrole,"promise body constraint","promise");
    Gr(consc,body->name,a_hasrole,"promise body constraint","promise");

    Gr(consc,body->ns,a_contains,body->name,"promise");
    Gr(consc,body->ns,a_hasrole,"namespace","promise");

    if (body->source_path)
       {
       Gr(consc,body->source_path,a_contains,body->name,"promise");
       Gr(consc,body->source_path,a_contains,umbrella,"promise");
       Gr(consc,body->source_path,a_hasrole,"file","promise");
       }

    for (Rlist *argp = body->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,umbrella,a_depends,RlistScalarValue(argp),"promise");
       Gr(consc,umbrella,a_hasarg,RlistScalarValue(argp),"promise");
       Gr(consc,RlistScalarValue(argp),a_hasrole,"argument parameter","promise");
       }
    }

 fclose(consc);
}

/**********************************************************************/

void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 if (context && strlen(context) > 0)
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,context);
    }
 else
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,"*");
    }
}

/**********************************************************************/

void IGr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 fprintf(consc,"(%s,-%d,%s,%s,%s,%s)\n",from,A[assoc].type,A[assoc].bwd,to,A[assoc].fwd,context);
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
 fprintf(consc,"(%s,%d,%s,%.2lf,%s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,context);
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
