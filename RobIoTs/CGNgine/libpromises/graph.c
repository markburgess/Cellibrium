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

/*****************************************************************************/

const int CONTAINS  = 1;  // bitwise for OR
const int FOLLOWS   = 2;
const int EXPRESSES = 4;
const int NEAR      = 8;

const int F = 0;
const int B = 1;


char *A[a_ass_dim+1][2] =
{
    {"seen concurrent with","seen concurrent with"},
    {"contains","may be found in"},
    {"is the name for","is referred to by"},
    {"is the source of","originates from"},
    {"has attribute","is an attribute of"},
    {"has value","is the value of"},
    {"has a specific instance","is a specific instance of"},
    {"is approximately","is approximately"},
    {"maintains as proxy","is maintained by"},
    {"depends on","partly determines"},
    {"has function","is the function of"},
    {"has constraint","constrains"},
    {"also known as","also known as"},
    {"has interpretation","is interpreted from"},
    {NULL, NULL},
};

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

 Gr(consc,"system policy",EXPRESSES|CONTAINS,a_hasattr,"promise bundle");
 Gr(consc,"system policy",CONTAINS,a_contains,"namespace");

 Gr(consc,"promise bundle",CONTAINS,a_contains,"promise");
 Gr(consc,"promise",EXPRESSES,a_hasattr,"promise type");
 Gr(consc,"promise",EXPRESSES,a_hasattr,"constraint");
 Gr(consc,"promise",EXPRESSES,a_hasattr,"promisee");
 Gr(consc,"constraint",EXPRESSES,a_hasattr,"lval");
 Gr(consc,"constraint",EXPRESSES,a_hasattr,"rval");
 Gr(consc,"rval", CONTAINS, a_contains, "compound rval");
 Gr(consc,"compound rval", NEAR, a_alias, "body group");
 Gr(consc,"compound rval", NEAR, a_alias, "constraints body");
 Gr(consc,"constraints body",EXPRESSES,a_hasattr,"lval");
 Gr(consc,"constraints body",EXPRESSES,a_hasattr,"rval");

 for (size_t i = 0; i < SeqLength(policy->bundles); i++)
    {
    const Bundle *bundle = SeqAt(policy->bundles, i);
    Gr(consc,"promise bundle",CONTAINS,a_contains,bundle->name);
    Gr(consc,bundle->ns,EXPRESSES,a_hasattr,bundle->name);
    Gr(consc,bundle->ns,CONTAINS,a_contains,bundle->name);
    Gr(consc,bundle->ns,EXPRESSES,a_hasattr,"namespace");
    Gr(consc,bundle->name,EXPRESSES,a_hasattr,"promise bundle");
    Gr(consc,bundle->type,FOLLOWS,a_maintains,bundle->name);

    if (bundle->source_path)
       {
       Gr(consc,bundle->source_path,EXPRESSES,a_hasattr,bundle->name);
       Gr(consc,bundle->source_path,CONTAINS,a_contains,bundle->name);
       Gr(consc,bundle->source_path,EXPRESSES,a_interpreted,"file");
       Gr(consc,bundle->source_path,EXPRESSES,a_interpreted,"where");
       }

    Rlist *argp = NULL;

    for (argp = bundle->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,bundle->name,FOLLOWS,a_depends,RlistScalarValue(argp));
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
          Gr(consc,handle,EXPRESSES,a_hasfunction,type->name);


          Gr(consc,bundle->type,EXPRESSES,a_maintains,promise->promiser);
          Gr(consc,bundle->type,EXPRESSES,a_maintains,handle);
          Gr(consc,bundle->name,EXPRESSES,a_hasattr,handle);
          Gr(consc,bundle->name,CONTAINS,a_contains,handle);
          Gr(consc,bundle->name,CONTAINS,a_contains, promise->promiser);
          Gr(consc,bundle->name,FOLLOWS,a_depends, promise->promiser);

          Gr(consc,promise->promiser,EXPRESSES,a_interpreted,"what");

          if (promise->comment)
             {
             Gr(consc,handle,EXPRESSES,a_hasattr,promise->comment);
             }

          switch (promise->promisee.type)
             {
             case RVAL_TYPE_SCALAR:
                 Gr(consc,(char *)promise->promisee.item,FOLLOWS,a_depends, promise->promiser);
                 Gr(consc,(char *)promise->promisee.item,FOLLOWS,a_depends, handle);
                 break;

             case RVAL_TYPE_LIST:
                 for (const Rlist *rp = promise->promisee.item; rp; rp = rp->next)
                    {
                    Gr(consc,RlistScalarValue(rp),FOLLOWS,a_depends, promise->promiser);
                    Gr(consc,RlistScalarValue(rp),FOLLOWS,a_depends, handle);
                    }
                 break;

             default:
                break;
             }

          // Class activation
          Gr(consc,handle,FOLLOWS,a_depends, promise->classes);
          Gr(consc,promise->classes, EXPRESSES, a_interpreted,"context");

          for (size_t cpi = 0; cpi < SeqLength(promise->conlist); cpi++)
             {
             Constraint *constraint = SeqAt(promise->conlist, cpi);

             switch (constraint->rval.type)
                {
                case RVAL_TYPE_SCALAR:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_SCALAR,umbrella);
                    Gr(consc,handle,EXPRESSES,a_hasconstraint,umbrella);
                    Gr(consc,umbrella,EXPRESSES,a_hasattr,constraint->lval);
                    Gr(consc,umbrella,EXPRESSES,a_hasattr,constraint->rval.item);
                    Gr(consc,constraint->lval, EXPRESSES, a_interpreted, "lval");
                    Gr(consc,constraint->rval.item, EXPRESSES, a_interpreted, "rval");
                    break;

                case RVAL_TYPE_LIST:
                    MakeUniqueClusterName(constraint->lval,constraint->rval.item,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,EXPRESSES,a_hasconstraint,umbrella);
                    Gr(consc,umbrella,EXPRESSES,a_hasattr,constraint->lval);
                    Gr(consc,constraint->lval, EXPRESSES, a_interpreted, "lval");

                    for (Rlist *rp = (Rlist *)constraint->rval.item; rp != NULL; rp=rp->next)
                       {
                       Gr(consc,umbrella,EXPRESSES,a_hasattr,RlistScalarValue(rp));
                       Gr(consc,RlistScalarValue(rp), EXPRESSES, a_interpreted, "rval");
                       }
                    break;

                case RVAL_TYPE_FNCALL:
                    {
                    FnCall *fp = (FnCall *)(constraint->rval).item;
                    MakeUniqueClusterName(fp->name,fp->args,RVAL_TYPE_LIST,umbrella);
                    Gr(consc,handle,EXPRESSES,a_hasconstraint,umbrella);
                    Gr(consc,constraint->lval, EXPRESSES, a_interpreted, "lval");
                    Gr(consc,fp->name, EXPRESSES, a_interpreted, "rval");

                    Gr(consc,handle,FOLLOWS,a_depends,umbrella);
                    Gr(consc,umbrella,FOLLOWS,a_depends,fp->name);

                    Gr(consc,promise->promiser,FOLLOWS,a_depends,umbrella);
                    Gr(consc,"functions",CONTAINS,a_hasinstance,fp->name);
                    Gr(consc,fp->name,CONTAINS,a_hasinstance,umbrella);
                    Gr(consc,umbrella,EXPRESSES,a_hasattr,constraint->lval);
                       
                    for (Rlist *argp = fp->args; argp != NULL; argp = argp->next)
                       {
                       Gr(consc,umbrella,FOLLOWS,a_depends,RlistScalarValue(argp));
                       Gr(consc,RlistScalarValue(argp), EXPRESSES, a_interpreted, "argument");
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

    Gr(consc,body->name,EXPRESSES,a_hasattr,body->type);
    Gr(consc,umbrella,EXPRESSES,a_hasattr,body->type);
    Gr(consc,body->type,EXPRESSES,a_hasattr,"what");

    Gr(consc,"constraints body",CONTAINS,a_contains,body->name);
    Gr(consc,"constraints body",CONTAINS,a_contains,umbrella);

    Gr(consc,body->ns,CONTAINS|EXPRESSES,a_hasattr,body->name);
    Gr(consc,body->ns,EXPRESSES,a_hasattr,"CGNgine namespace");

    if (body->source_path)
       {
       Gr(consc,body->source_path,EXPRESSES,a_hasattr,body->name);
       Gr(consc,body->source_path,EXPRESSES,a_hasattr,umbrella);
       Gr(consc,body->source_path,EXPRESSES,a_contains,body->name);
       Gr(consc,body->source_path,EXPRESSES,a_contains,umbrella);
       Gr(consc,body->source_path,EXPRESSES,a_hasattr,"file");
       Gr(consc,body->source_path,EXPRESSES,a_hasattr,"where");
       }


    for (Rlist *argp = body->args; argp != NULL; argp = argp->next)
       {
       Gr(consc,umbrella,CONTAINS,a_contains,RlistScalarValue(argp));
       }
    }

 fclose(consc);
}

/**********************************************************************/

void Gr(FILE *consc,char *from, int type, enum associations assoc, char *to)
{
 fprintf(consc,"(%s,%d,%s,%s,%s)\n",from,type,A[assoc][F],A[assoc][B],to);
}

/**********************************************************************/

void GrQ(FILE *consc,char *from, int type, enum associations assoc, double to)
{
 fprintf(consc,"(%s,%d,%s,%s,%.2lf)\n",from,type,A[assoc][F],A[assoc][B],to);
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
