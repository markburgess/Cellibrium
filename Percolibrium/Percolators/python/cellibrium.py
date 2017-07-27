import sys
import re

GR_CONTAINS  = 3 
GR_FOLLOWS   =  2  # i.e. influenced by
GR_EXPRESSES = 4 #represents, etc
GR_NEAR      = 1 # approx like
GR_CONTEXT   = 5 # approx like
ALL_CONTEXTS = "any"

A = {
    "a_contains" :     {GR_CONTAINS,"contains","belongs to or is part of"},
    "a_generalizes" :     {GR_CONTAINS,"generalizes","is a special case of"},
    "a_origin" :     {GR_FOLLOWS,"may originate from","may be the source or origin of"},
    "a_maintainedby" :     {GR_FOLLOWS,"is maintained by","maintains"},
    "a_depends" :     {GR_FOLLOWS,"depends on","partly determines"},
    "a_caused_by" :     {GR_FOLLOWS,"may be caused by","can cause"},
    "a_uses" :     {GR_FOLLOWS,"may use","may be used by"},
    "a_name" :     {GR_EXPRESSES,"is called","is a name for"},
    "a_hasattr" :      {GR_EXPRESSES,"expresses an attribute","is an attribute of"},
    "a_promises" :      {GR_EXPRESSES,"promises","is promised by"},
    "a_hasinstance" :     {GR_EXPRESSES,"has an instance or particular case","is a particular case of"},
    "a_hasvalue" :     {GR_EXPRESSES,"has value or state","is the state or value of"},
    "a_hasarg" :     {GR_EXPRESSES,"has argument or parameter","is a parameter or argument of"},
    "a_hasrole" :      {GR_EXPRESSES,"has the role of","is a role fulfilled by"},
    "a_hasoutcome" :     {GR_EXPRESSES,"has the outcome","is the outcome of"},
    "a_hasfunction" :     {GR_EXPRESSES,"has function","is the function of"},
    "a_hasconstraint" :     {GR_EXPRESSES,"has constraint","constrains"},
    "a_interpreted" :     {GR_EXPRESSES,"has interpretation","is interpreted from"},
    "a_concurrent" :     {GR_NEAR,"seen concurrently with","seen concurrently with"},
    "a_alias" :     {GR_NEAR,"also known as","also known as"},
    "a_approx" :     {GR_NEAR,"is approximately","is approximately"},
    "a_related_to" :     {GR_NEAR,"may be related to","may be related to"},
    "a_ass_dim" :   {0, "NULL", "NULL"},
    }


########################################################################################################

def Sanitize(s):
    ss = re.sub(r"[\\/,]","_",s)
    return ss

########################################################################################################

def Gr(file,from_t, name, to_t, context):
    type,fwd,bwd = A[name]
    sfrom = Sanitize(from_t)
    if len(context) > 0:
       fs = "(" + sfrom + "," + "%d" % type + "," + fwd + "," + to_t + "," + bwd + "," + context + ")\n"
    else:
       fs = "(" + sfrom + "," + "%d" % type + "," + fwd + "," + to_t + "," + bwd + "," + "*" + ")\n"

    file.write(fs);

########################################################################################################
    
def IGr(file,from_t, name, to_t, context):
    type,fwd,bwd = A[name]
    sfrom = Sanitize(from_t)
    if len(context) > 0:
       fs = "(" + sfrom + "," + "-%d" % type + "," + bwd + "," + to_t + "," + fwd + "," + context + ")\n"
    else:
       fs = "(" + sfrom + "," + "-%d" % type + "," + bwd + "," + to_t + "," + fwd + "," + "*" + ")\n"

    file.write(fs);

########################################################################################################

def Number(file,from_t, q, context):
    type,fwd,bwd = A[a_hasrole]
    if len(context) > 0:
       fs = "(" + "%.2lf" % q + "," + "-%d" % type + "," + bwd + "," + "number" + "," + fwd + "," + context + ")\n"
    else:
       fs = "(" + "%.2lf" % q + "," + "-%d" % type + "," + bwd + "," + "number" + "," + fwd + "," + "*" + ")\n"

    file.write(fs);


########################################################################################################
    
def GrQ(file,from_t, name, q, context):
    type,fwd,bwd = A[name]

    if len(context) > 0:
       fs = "(" + sfrom + "," + "%d" % type + "," + bwd + "," + "%.2lf" % q + "," + fwd + "," + context + ")\n"
    else:
       fs = "(" + sfrom + "," + "%d" % type + "," + bwd + "," + "%.2lf" % q + "," + fwd + "," + "*" + ")\n"

    file.write(fs);

########################################################################################################

def RoleCluster(file,compound_name,role,attributes,ex_context):
    
    Gr(file,compound_name,"a_hasrole",role,ex_context)
    
    if len(attributes) > 0:
        words = attributes.split(",")
        
        for word in words:
            Gr(file,compound_name,"a_hasattr",word,ALL_CONTEXTS);

    return compound_name

########################################################################################################

def ContextCluster(file,compound_name):
    
    if len(attributes) > 0:
        words = attributes.split(" ")
        
        for word in words:
            Gr(file,compound_name,"a_contains",word,ALL_CONTEXTS);

    return compound_name

