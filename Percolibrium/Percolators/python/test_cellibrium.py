
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
c.RoleGr(sys.stdout,compound_name,"murder by breadknife","in the library,Professor plum,Miss Scarlet,Miss Scarlet refuses to marry Professory plum","*")

# or

now = time.time()

who = "Miss Scarlet and Professor Plum";
what = "murder by breaknife";
why = "Miss Scarlet refuses to marry Professory plum"; 
where = "in the library";
when = now;
how = "by breadknife" #MakeAnomalyGrName("anomaly",syndrome);
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
how = "how it happened (i.e. symptoms)" #MakeAnomalyGrName("anomaly",syndrome);
icontext = "system monitoring";

wherex = c.WhereGr(sys.stdout,"Oslo","marklaptop","unknown","192.168.1.183","");

c.Clue(sys.stdout,who,what,when,wherex,how,why,icontext);

print "------------------------------------------------------"

# Causation 

c.Gr(sys.stdout,"performance anomaly at downstream host","a_origin","performance anomaly at upstream host","distributed system causation")
c.RoleGr(sys.stdout,"performance anomaly at upstream host","performance anomaly","at upstream host","distributed system")
c.RoleGr(sys.stdout,"performance anomaly at downstream host","performance anomaly","at downstream host","distributed system")

where = c.HereGr(sys.stdout,"mark's laptop")

print "------------------------------------------------------"

# Service relationships

# Servername = sshd
# servicename = ssh (port nr 22)

# server -> role, client/server, attr -> host identity ...

where = c.WhereGr(sys.stdout,
          "London",
          "myserver",
          "example.com",
          "123.456.789.10/24",
          "2001:::7/64",
          )

c.ServerInstanceGr(sys.stdout,
           "ssh",
           22,
           "/usr/local/sshd",
           where   
           );

where = c.WhereGr(sys.stdout,
           "San Jose",
           "desktop",
           "example.com",
           "321.654.987.99/24",
           "2001:0db8:0:f101::1/64"
           );
                  
c.ClientInstanceGr(sys.stdout,
           "ssh",
           "/usr/bin/ssh",
           where
           )

hostidentity = "123.456.789.55/24"

where = c.WhereGr(sys.stdout,
          "NYC datacentre",
          "node45-abc",
          "cloudprovider.com",
          hostidentity,
          "",
          )

c.ServerInstanceGr(sys.stdout,
           "nodemanager",
           50345,
           "/usr/local/cldstack/cloudmgrd",
           c.HereGr(sys.stdout,"Florida datacentre")
           )

print "------------------------------------------------------"

