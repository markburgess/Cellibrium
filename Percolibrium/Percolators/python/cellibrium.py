import sys
import time
import re

class Cellibrium:

    GR_CONTAINS  = 3 
    GR_FOLLOWS   = 2  # i.e. influenced by
    GR_EXPRESSES = 4 #represents, etc
    GR_NEAR      = 1 # approx like
    GR_CONTEXT   = 5 # approx like
    ALL_CONTEXTS = "any"
    
    A = {
        "a_contains" :     [GR_CONTAINS,"contains","belongs to or is part of"],
        "a_generalizes" :  [GR_CONTAINS,"generalizes","is a special case of"],
        "a_origin" :       [GR_FOLLOWS,"may originate from","may be the source or origin of"],
        "a_maintainedby" : [GR_FOLLOWS,"is maintained by","maintains"],
        "a_depends" :      [GR_FOLLOWS,"depends on","partly determines"],
        "a_caused_by" :    [GR_FOLLOWS,"may be caused by","can cause"],
        "a_uses" :         [GR_FOLLOWS,"may use","may be used by"],
        "a_name" :         [GR_EXPRESSES,"is called","is a name for"],
        "a_hasattr" :      [GR_EXPRESSES,"expresses an attribute","is an attribute of"],
        "a_promises" :     [GR_EXPRESSES,"promises","is promised by"],
        "a_hasinstance" :  [GR_EXPRESSES,"has an instance or particular case","is a particular case of"],
        "a_hasvalue" :     [GR_EXPRESSES,"has value or state","is the state or value of"],
        "a_hasarg" :       [GR_EXPRESSES,"has argument or parameter","is a parameter or argument of"],
        "a_hasrole" :      [GR_EXPRESSES,"has the role of","is a role fulfilled by"],
        "a_hasoutcome" :   [GR_EXPRESSES,"has the outcome","is the outcome of"],
        "a_hasfunction" :  [GR_EXPRESSES,"has function","is the function of"],
        "a_hasconstraint" :[GR_EXPRESSES,"has constraint","constrains"],
        "a_interpreted" :  [GR_EXPRESSES,"has interpretation","is interpreted from"],
        "a_concurrent" :   [GR_NEAR,"seen concurrently with","seen concurrently with"],
        "a_alias" :        [GR_NEAR,"also known as","also known as"],
        "a_approx" :       [GR_NEAR,"is approximately","is approximately"],
        "a_related_to" :   [GR_NEAR,"may be related to","may be related to"],
        "a_ass_dim" :      [0, "NULL", "NULL"],
        }

    GR_DAY_TEXT = [
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
        "Sunday"
        ]
        
    GR_MONTH_TEXT = [
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
        ]
        
    GR_SHIFT_TEXT = [
        "Night",
        "Morning",
        "Afternoon",
        "Evening"
        ]

    ########################################################################################################
    
    def Sanitize(self,s):
        ss = re.sub(r"[\\/,]","_",s)
        return ss
    
    ########################################################################################################

    def Gr(self,ofile,from_t, name, to_t, context):
        atype,fwd,bwd = list(self.A[name])
        sfrom = self.Sanitize(from_t)
        if len(context) > 0:
            fs = "(" + sfrom + "," + "%d" % atype + "," + fwd + "," + to_t + "," + bwd + "," + context + ")\n"
        else:
            fs = "(" + sfrom + "," + "%d" % atype + "," + fwd + "," + to_t + "," + bwd + "," + "*" + ")\n"
            
        ofile.write(fs)

    ########################################################################################################

    def IGr(self,ofile,from_t, name, to_t, context):
        type,fwd,bwd = self.A[name]
        sfrom = self.Sanitize(from_t)
        if len(context) > 0:
            fs = "(" + sfrom + "," + "-%d" % type + "," + bwd + "," + to_t + "," + fwd + "," + context + ")\n"
        else:
            fs = "(" + sfrom + "," + "-%d" % type + "," + bwd + "," + to_t + "," + fwd + "," + "*" + ")\n"
            
        ofile.write(fs)

    ########################################################################################################

    def Number(self,ofile,from_t, q, context):
        type,fwd,bwd = self.A[a_hasrole]
        if len(context) > 0:
            fs = "(" + "%.2lf" % q + "," + "-%d" % type + "," + bwd + "," + "number" + "," + fwd + "," + context + ")\n"
        else:
            fs = "(" + "%.2lf" % q + "," + "-%d" % type + "," + bwd + "," + "number" + "," + fwd + "," + "*" + ")\n"         
        ofile.write(fs)

    ########################################################################################################
    
    def GrQ(self,ofile,from_t, name, q, context):
        type,fwd,bwd = self.A[name]
        
        if len(context) > 0:
            fs = "(" + sfrom + "," + "%d" % type + "," + bwd + "," + "%.2lf" % q + "," + fwd + "," + context + ")\n"
        else:
            fs = "(" + sfrom + "," + "%d" % type + "," + bwd + "," + "%.2lf" % q + "," + fwd + "," + "*" + ")\n"
            
        ofile.write(fs)

    ########################################################################################################

    def RoleCluster(self,ofile,compound_name,role,attributes,ex_context):
    
        self.Gr(ofile,compound_name,"a_hasrole",role,ex_context)
        
        if len(attributes) > 0:
            words = attributes.split(",")
            
            for word in words:
                self.Gr(ofile,compound_name,"a_hasattr",word,self.ALL_CONTEXTS);
                
        return compound_name

    ########################################################################################################

    def ContextCluster(self,ofile,compound_name):
    
        if len(compound_name) > 0:
            words = compound_name.split(" ")
            
            for word in words:
                self.Gr(ofile,compound_name,"a_contains",word,self.ALL_CONTEXTS);
                
        return compound_name

    ########################################################################################################

    def Clue(self,ofile,who,what,whentime,where,how,why,icontext):

        if (whentime > 0):
            when = self.TimeCluster(ofile,whentime);
        else:
            when = "regular check";

        event = who + " saw " + what + " at " + when + " location " + where + " " + how + " cause " + why
        attr = who + "," + what + "," + when + "," + where + "," + how + "," + why

        self.RoleCluster(ofile,event,"event",attr,icontext)
        self.RoleCluster(ofile,who,"who","",icontext)
        self.RoleCluster(ofile,what,"what",what,icontext)
        self.RoleCluster(ofile,how,"how",how,icontext)
        self.RoleCluster(ofile,why,"why","",icontext)
        self.Gr(ofile,what,"a_caused_by",why,icontext);

        
    ########################################################################################################

    def TimeCluster(self,ofile,now):
        
        # To do: Exend to add GMT too...

        lt = time.localtime(now)

        # Time semantics
        
        lifecycle = "Lcycle_%d" % (lt[0] % 3)
        year = "Yr%d" % lt[0]
        month = self.GR_MONTH_TEXT[lt[1]-1]
        day = "Day%02d" % lt[2]
        dow = "%s" % self.GR_DAY_TEXT[lt[6]]
        hour = "Hr%02d" % lt[3]
        shift = "%s" % self.GR_SHIFT_TEXT[lt[3] / 6];
        quarter = "Q%d" % ((lt[4] / 15) + 1)
        min = "Min%02d" % lt[4]
        interval_start = (lt[4] / 5) * 5
        interval_end = (interval_start + 5) % 60
        mins = "Min%02d_%02d" % (interval_start,interval_end)

        hub = "on %s %s %s %s %s at %s %s %s" % (shift,dow,day,month,year,hour,min,quarter)
        attributes = "%s,%s,%s,%s,%s,%s,%s,%s" % (shift,dow,day,month,year,hour,min,quarter)

        self.RoleCluster(ofile,hub,"when",attributes,self.ContextCluster(ofile,"local clock time"));
        self.RoleCluster(ofile,shift,"time of day","work shift","time");
        self.RoleCluster(ofile,dow,"weekday","","clock time");
        self.RoleCluster(ofile,day,"day of month","","clock time");
        self.RoleCluster(ofile,month,"month","","clock time");
        self.RoleCluster(ofile,year,"year","","clock time");
        self.RoleCluster(ofile,hour,"hour","","clock time");
        self.RoleCluster(ofile,month,"minutes past the hour","minutes","clock time");       

        return hub;

    ########################################################################################################

    def WhereCluster(self,ofile,address):

        # VUQNAME, VDOMAIN, VIPADDRESS,NULL);
        # figure out my IP address, FQHN, domainname, etc...

        # The endless problem of host identification has no generic solution...
        
        return "no idea"
    
