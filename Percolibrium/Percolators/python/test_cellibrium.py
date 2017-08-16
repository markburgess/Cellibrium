
########################################################################################################    
#
# TEST
#
########################################################################################################

import sys
import time
import os
import socket
from cellibrium import Cellibrium

c = Cellibrium()
s = "base\\topic"

#
# Using the primitive graph tuples
#

print "Testing...1"
c.Gr(sys.stdout,"base/topic","a_contains", "sub topic", "all the time")

print "Testing...2"
c.Gr(sys.stdout,s,"a_contains", "sub topic", "all the time")

print "Testing...3"
c.Gr(sys.stdout,"base,topic","a_contains", "sub topic", "")

print "test 4"

#
# Calling the role interface
#

compound_name = "Professor Plum murders Miss Scarlet in the library with a breadknife because she would not marry him" 
c.RoleCluster(sys.stdout,compound_name,"murder by breadknife","in the library,Professor plum,Miss Scarlet,Miss Scarlet refuses to marry Professory plum","*")

#
# How to call Cluedo interface
#

now = time.time()

who = "cgn_montord";
what = "anomalous state change";
why = "unknown"; 
where = "mark's laptop";
when = now;
how = "how it happened (i.e. symptoms)" #MakeAnomalyClusterName("anomaly",syndrome);
icontext = "system monitoring";

wherex = c.WhereCluster(sys.stdout,where);

c.Clue(sys.stdout,who,what,when,wherex,how,why,icontext);

