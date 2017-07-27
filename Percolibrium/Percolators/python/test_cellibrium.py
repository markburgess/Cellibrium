
########################################################################################################    
#
# TEST
#
########################################################################################################

import sys
from cellibrium import Gr

s = "base\\topic"

print "Testing..."
Gr(sys.stdout,"base/topic","a_contains", "sub topic", "all the time")
Gr(sys.stdout,s,"a_contains", "sub topic", "all the time")
Gr(sys.stdout,"base,topic","a_contains", "sub topic", "")

