
########################################################################################################    
#
# TEST
#
########################################################################################################

import sys
import time
import os
import socket
import re
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

print "------------------------------------------------------"

#
# Calling the role interface
#

compound_name = "Professor Plum murders Miss Scarlet in the library with a breadknife because she would not marry him" 
c.RoleCluster(sys.stdout,compound_name,"murder by breadknife","in the library,Professor plum,Miss Scarlet,Miss Scarlet refuses to marry Professory plum","*")

# or

now = time.time()

who = "Miss Scarlet and Professor Plum";
what = "murder by breaknife";
why = "Miss Scarlet refuses to marry Professory plum"; 
where = "in the library";
when = now;
how = "by breadknife" #MakeAnomalyClusterName("anomaly",syndrome);
icontext = "cluedo";

c.Clue(sys.stdout,who,what,when,where,how,why,icontext);


print "------------------------------------------------------"

#
# How to call Cluedo interface for a system issue
#

now = time.time()

who = "cgn_montord";
what = "anomalous state change";
why = "unknown"; 
where = "mark's laptop";
when = now;
how = "how it happened (i.e. symptoms)" #MakeAnomalyClusterName("anomaly",syndrome);
icontext = "system monitoring";

wherex = c.WhereCluster(sys.stdout,"Oslo","marklaptop","unknown","192.168.1.183",0,0);

c.Clue(sys.stdout,who,what,when,wherex,how,why,icontext);

print "------------------------------------------------------"

# Causation 

c.Gr(sys.stdout,"performance anomaly at downstream host","a_origin","performance anomaly at upstream host","distributed system causation")
c.RoleCluster(sys.stdout,"performance anomaly at upstream host","performance anomaly","at upstream host","distributed system")
c.RoleCluster(sys.stdout,"performance anomaly at downstream host","performance anomaly","at downstream host","distributed system")


where = c.HereCluster(sys.stdout)

c.ServerCluster(sys.stdout,
               "ssh",
               "/usr/local/sshd",
               "London",
               "myserver",
               "example.com",
               "123.456.789.10/24",
               "2001:::7/64",
               22
               );

c.ClientCluster(sys.stdout,
               "ssh",
               "/usr/bin/ssh",
               "San Jose",
               "desktop",
               "example.com",
               "321.654.987.99/24",
               "2002:::8/64"
               );


c.ServerCluster(sys.stdout,
                "nodemanager",
                "/usr/local/cldstack/cloudmgrd",
                "NYC",
                "node45-abc",
                "cloudprovider.com",
                "123.456.789.55/24",
                "",
                "50345"
                )



# Parse log file - but every syslog message first needs to be understood for its intent

#syslog_what = "Something happened src=192.168.5.56, dest=192.168.5.65, type=X, state change=BLA"
#syslog_when = "Monday" # suitable grain size e.g. Monday
#syslog_where = clienthub # me
#syslog_origin = "resource manager"

# c.Influence(src,dst)
# c.Exception(event)

#c.ExceptionCluster(sys.stdout,log_when,log_where,log_what)

# c.Imposition(srcwho,srcwhere,srcwhen,dstwho,dstwhere,dstwhen,what) - would have to be discovered from source code (or open connection vs listening)
# c.ServicePromise(event)
# c.UsePromise(event)

# listener - performance can be affected
# sender - semantics can be affected




