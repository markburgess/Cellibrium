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

 Gr(consc,"system policy", a_contains,"promise bundle");
 Gr(consc,"system policy", a_contains,"namespace");
 Gr(consc,"promise bundle", a_contains,"promise");
 Gr(consc,"promise", a_contains,"promise type");
 Gr(consc,"promise", a_contains,"constraint");
 Gr(consc,"promise", a_contains,"promisee");
 Gr(consc,"constraint", a_contains,"lval");
 Gr(consc,"constraint", a_contains,"rval");
 Gr(consc,"rval", a_hasrole, "compound rval");
 Gr(consc,"compound rval", a_alias, "body group");
 Gr(consc,"compound rval", a_alias, "constraints body");

 for (size_t i = 0; i < SeqLength(policy->bundles); i++)
    {
    const Bundle *bundle = SeqAt(policy->bundles, i);
    Gr(consc,bundle->ns,a_contains,bundle->name);
    Gr(consc,bundle->ns,a_hasrole,"namespace");
    Gr(consc,bundle->name,a_hasrole,"promise bundle");
    Gr(consc,bundle->type,a_maintainedby,bundle->name);

    if (bundle->source_path)
       {
       Gr(consc,bundle->source_path,a_contains,bundle->name);
       Gr(consc,bundle->source_path,a_hasrole,"file");
       }

    Rlist *argp = NULL;

    for (argp = bundle->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,bundle->name,a_depends,RlistScalarValue(argp));
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
          Gr(consc,handle,a_hasfunction,type->name);
          Gr(consc,handle,a_contains,promise->promiser);
          Gr(consc,promise->promiser,a_maintainedby,bundle->type);
          Gr(consc,promise->promiser,a_hasrole,"promiser");

          Gr(consc,bundle->name,a_contains,handle);          
          Gr(consc,handle,a_maintainedby,bundle->type);
          Gr(consc,handle,a_hasrole,"promise handle");
          Gr(consc,handle,a_hasfunction,"promise");

          Gr(consc,promise->promiser,a_hasrole,"what");

          if (promise->comment)
             {
             Gr(consc,handle,a_hasattr,promise->comment);
             }

          switch (promise->promisee.type)
             {
             case RVAL_TYPE_SCALAR:
                 Gr(consc,handle,a_contains,promise->promisee.item);
                 Gr(consc,(char *)promise->promisee.item,a_depends,promise->promiser);
                 Gr(consc,(char *)promise->promisee.item,a_depends,handle);
                 break;

             case RVAL_TYPE_LIST:
                 for (const Rlist *rp = promise->promisee.item; rp; rp = rp->next)
                    {
                    Gr(consc,handle,a_contains,RlistScalarValue(rp));
                    Gr(consc,RlistScalarValue(rp),a_depends, promise->promiser);
                    Gr(consc,RlistScalarValue(rp),a_depends, handle);
                    }
                 break;

             default:
                break;
             }

          // Class activation
          Gr(consc,handle,a_depends,promise->classes);
          Gr(consc,promise->classes,a_hasrole,"class/context label");

          for (size_t cpi = 0; cpi < SeqLength(promise->conlist); cpi++)
             {
             Constraint *constraint = SeqAt(promise->conlist, cpi);

             switch (constraint->rval.type)
                {
                case RVAL_TYPE_SCALAR:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_SCALAR,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella);
                    Gr(consc,umbrella,a_hasrole,"promise body constraint");
                    Gr(consc,umbrella,a_hasattr,constraint->lval);
                    Gr(consc,umbrella,a_hasattr,constraint->rval.item);
                    Gr(consc,constraint->lval, a_hasrole, "lval");
                    Gr(consc,constraint->rval.item, a_hasrole, "rval");
                    break;

                case RVAL_TYPE_LIST:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella);
                    Gr(consc,umbrella,a_hasrole,"promise body constraint");
                    Gr(consc,umbrella,a_hasattr,constraint->lval);
                    Gr(consc,constraint->lval, a_hasrole, "lval");

                    for (Rlist *rp = (Rlist *)constraint->rval.item; rp != NULL; rp=rp->next)
                       {
                       Gr(consc,umbrella,a_hasattr,RlistScalarValue(rp));
                       Gr(consc,RlistScalarValue(rp), a_hasrole, "rval");
                       }
                    break;

                case RVAL_TYPE_FNCALL:
                    {
                    FnCall *fp = (FnCall *)(constraint->rval).item;
                    MakeUniqueClusterName(fp->name,fp->args,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,a_hasconstraint,umbrella);
                    Gr(consc,constraint->lval, a_hasrole, "lval");
                    Gr(consc,fp->name, a_hasrole, "rval");
                    Gr(consc,handle,a_depends,umbrella);
                    Gr(consc,umbrella,a_depends,fp->name);

                    Gr(consc,promise->promiser,a_depends,umbrella);
                    Gr(consc,fp->name, a_hasrole,"function");
                    Gr(consc,fp->name,a_generalizes,umbrella);
                    Gr(consc,umbrella,a_hasattr,constraint->lval);
                       
                    for (Rlist *argp = fp->args; argp != NULL; argp = argp->next)
                       {
                       Gr(consc,umbrella,a_depends,RlistScalarValue(argp));
                       Gr(consc,RlistScalarValue(argp),a_hasrole,"parameter/argument");
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

    Gr(consc,body->name,a_hasrole,body->type);
    Gr(consc,umbrella,a_hasrole,body->type);
    Gr(consc,body->type,a_hasrole,"what");

    Gr(consc,umbrella,a_hasrole,"promise body constraint");
    Gr(consc,body->name,a_hasrole,"promise body constraint");

    Gr(consc,body->ns,a_contains,body->name);
    Gr(consc,body->ns,a_hasrole,"namespace");

    if (body->source_path)
       {
       Gr(consc,body->source_path,a_contains,body->name);
       Gr(consc,body->source_path,a_contains,umbrella);
       Gr(consc,body->source_path,a_hasrole,"file");
       Gr(consc,body->source_path,a_hasrole,"where");
       }

    for (Rlist *argp = body->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,umbrella,a_depends,RlistScalarValue(argp));
       }
    }

 fclose(consc);
}

/**********************************************************************/

void Gr(FILE *consc,char *from, enum associations assoc, char *to)
{
 fprintf(consc,"(%s,%d,%s,%s,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd);
}

/**********************************************************************/

void IGr(FILE *consc,char *from, enum associations assoc, char *to)
{
 fprintf(consc,"(%s,-%d,%s,%s,%s)\n",from,A[assoc].type,A[assoc].bwd,to,A[assoc].fwd);
}

/**********************************************************************/

void Number(FILE *consc, double q)
{
 enum associations assoc = a_hasrole;
 fprintf(consc,"(%.2lf,%d,%s,%s,%s)\n",q,A[assoc].type,A[assoc].fwd,"number",A[assoc].bwd);
}

/**********************************************************************/

void GrQ(FILE *consc,char *from, enum associations assoc, double to)
{
 fprintf(consc,"(%s,%d,%s,%.2lf,%s)\n",from,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd);
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
