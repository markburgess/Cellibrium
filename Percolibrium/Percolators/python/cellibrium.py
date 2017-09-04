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
        "a_providedby" :   [GR_FOLLOWS,"may be provided by","may provide"],
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
    
    def InitialCluster(self,ofile):
    
        # Basic axioms about causation (upstream/downstream principle)

        self.ContextCluster(ofile,"service relationship");
        self.ContextCluster(ofile,"system diagnostics");
        self.ContextCluster(ofile,"lifecycle state change");
        self.ContextCluster(ofile,"software exception");
        self.ContextCluster(ofile,"promise keeping");
        
        self.Gr(ofile,"client measurement anomaly","a_caused_by","client software exception","system diagnostics");
        self.Gr(ofile,"client measurement anomaly","a_caused_by","server software exception","system diagnostics");
        self.Gr(ofile,"server measurement anomaly","a_caused_by","server software exception","system diagnostics");
        
        self.Gr(ofile,"measurement anomaly","a_caused_by","software exception","system diagnostics");
        
        self.Gr(ofile,"resource contention","a_caused_by","resource limit","system diagnostics");
        self.Gr(ofile,"increasing queue length","a_caused_by","resource contention","system diagnostics");
        self.Gr(ofile,"system performance slow","a_caused_by","increasing queue length","system diagnostics");
        self.Gr(ofile,"system performance slow","a_related_to","system performance latency","system diagnostics");
        
        self.Gr(ofile,"system performance latency","a_caused_by","resource contention","system diagnostics");
        self.Gr(ofile,"system performance latency","a_caused_by","increasing queue length","system diagnostics");
        
        self.Gr(ofile,"system performance latency","a_caused_by","server unavailability","system diagnostics");
        self.Gr(ofile,"server unavailability","a_caused_by","software crash","system diagnostics");
        self.Gr(ofile,"server unavailability","a_caused_by","system performance slow","system diagnostics");

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
        self.Gr(ofile,what,"a_caused_by",why,icontext)
        
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


    # Could also use  WeekSlot (Mon-Sun,MinXX_YY),
    #                 MonthSlot (1stday, lastday, else DayN) etc

    ########################################################################################################

    def WhereCluster(self,ofile,address,uqhn,domain,ipv4,ipv6,portnumber):

        # VUQNAME, VDOMAIN, VIPADDRESS,NULL);
        # figure out my IP address, FQHN, domainname, etc...

        if len(domain) == 0:
            domain = "unknown domain";

        if len(ipv6) == 0:
            domain = "no ipv6"

        if portnumber > 0:
            where = "host %s.%s IPv4 %s ipv6 %s at %s port %d" % (uqhn,domain,ipv4,ipv6,address,portnumber)
            attr = "hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s,ip portnumber %d" % (uqhn,domain,ipv4,ipv6,address,portnumber)
        else:
            where = "host %s.%s IPv4 %s ipv6 %s at %s" % (uqhn,domain,ipv4,ipv6,address)
            attr = "hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s" % (uqhn,domain,ipv4,ipv6,address)
         
        self.RoleCluster(ofile,where,"where",attr, "location")

        host = "hostname %s" % uqhn
        self.RoleCluster(ofile,host,"hostname",uqhn,"location")
        self.Gr(ofile,where,"a_alias",host,"host identification")  # Alias for quick association
  
        domain = "domain %s" % domain
        self.RoleCluster(ofile,domain,"dns domain name",domain,"location")
 
        where4 = "ipv4 address %s" % ipv4
        self.RoleCluster(ofile,where4,"ipv4 address", ipv4,"location")
        self.Gr(ofile,where,"a_alias",where4,"host identification")  # Alias for quick association
        
        where6 = "ipv6 address %s" % ipv6
        self.RoleCluster(ofile,where6,"ipv6 address", ipv6,"location")
        self.Gr(ofile,where,"a_alias",where6,"host identification")  # Alias for quick association
        
        if portnumber > 0:
            port = "ip portnumber %d" % portnumber
            nr = "%d" % portnumber
            self.RoleCluster(ofile,port,"ip portnumber",nr,"location")
 
        desc = "decription address %s" % address
        self.RoleCluster(ofile,where,"description address",address,"location")
 
        self.Gr(ofile,domain,"a_contains",uqhn, self.ContextCluster(ofile,"location"));
        self.Gr(ofile,domain,"a_contains",ipv4, "location");
        self.Gr(ofile,domain,"a_contains",ipv6, "location");
        self.Gr(ofile,"description address","a_related_to","street address", "location");
        
        return where;

    ########################################################################################################

    def HereCluster(self,ofile):

        # VUQNAME, VDOMAIN, VIPADDRESS,NULL);
        # figure out my IP address, FQHN, domainname, etc...

        id = "host localhost domain undefined ipv4 127.0.0.1 ipv6 ::1" # how can we make this the outer ip?
        
        return id
    
    ########################################################################################################
    
    def ServiceCluster(self,ofile,servicename):

        name = "service %s" % servicename
        self.Gr(ofile,name,"a_hasrole","service","service relationship")
        self.Gr(ofile,name,"a_hasfunction",servicename,"service relationship")
        
        server = "%s server" % servicename
        self.Gr(ofile,server,"a_hasrole","server","service relationship");
        
        client = "%s client" % servicename
        self.Gr(ofile,client,"a_hasrole","client","service relationship");
        
        self.Gr(ofile,client,"a_depends",server,"service relationship");
        self.Gr(ofile,client,"a_uses",name,"service relationship");
        
        return name;

    ########################################################################################################

    def ServerCluster(self,ofile,servicename,servername,address,uqhn,domain,ipv4,ipv6,portnumber):
        where = self.WhereCluster(ofile,address,uqhn,domain,ipv4,ipv6,portnumber)
        self.ServiceCluster(ofile,servicename)
        return self.ServiceInstance(ofile,"server",servername,servicename,where);

    ########################################################################################################

    def ClientCluster(self,ofile,servicename,clientname,address,uqhn,domain,ipv4,ipv6):
        where = self.WhereCluster(ofile,address,uqhn,domain,ipv4,ipv6,0);
        self.ServiceCluster(ofile,servicename);
        return self.ServiceInstance(ofile,"client",clientname,servicename,where)  

    ########################################################################################################

    def ServiceInstance(self,ofile,role,instancename,servicename,where):

        location = "located at %s" % where
        service = "%s %s" % (servicename,role)             # e.g. ftp server
        instance = "instance %s %s" % (role,instancename)  # e.g. instance server myhost
        rolehub = "%s %s %s" % (service,instance,location) # (ftp server) (instance server myhost) (located at WHERE)
        attr = "%s,%s" % (service,location)             # (ftp server),(located at WHERE)
        
        # Top level hub
        self.RoleCluster(ofile,rolehub,instance,attr,"service relationship");
        
        # Attr hierarchy
        self.RoleCluster(ofile,location,"where",where,"service relationship instance");
        self.RoleCluster(ofile,service,role,servicename,"service relationship instance");
        
        attr = "%s,%s" % (role,instancename)               # (ftp server),(located at WHERE)
        self.RoleCluster(ofile,instance,role,instancename,"service relationship instance");
        
        # Causation
        
        if role == "client":
            self.Gr(ofile,rolehub,"a_depends",servicename,"service relationship")
        else:
            self.Gr(ofile,servicename,"a_providedby",rolehub,"service relationship")
        
        return rolehub

    ########################################################################################################

    def ExceptionCluster(self,ofile,origin,logmessage):

        # 2016-08-13T15:00:01.906160+02:00 linux-e2vo /usr/sbin/cron[23039]: pam_unix(crond:session): session opened for user root by (uid=0)
        # When                             where      who                    what                                                  (new who)
        # Why = (lifecycle state change, exception, ...)

        # ???
        self.Gr(ofile,origin,"a_related_to",logmessage,"???? TBD")

        return "something"

