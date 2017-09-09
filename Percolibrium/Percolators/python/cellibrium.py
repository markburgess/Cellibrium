import sys
import time
import re
import socket

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
    
    def InitialGr(self,ofile):
    
        # Basic axioms about causation (upstream/downstream principle)

        self.ContextGr(ofile,"service relationship");
        self.ContextGr(ofile,"system diagnostics");
        self.ContextGr(ofile,"lifecycle state change");
        self.ContextGr(ofile,"software exception");
        self.ContextGr(ofile,"promise keeping");
        self.ContextGr(ofile,"host location identification");        
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
        if from_t == to_t:
            return
        atype,fwd,bwd = list(self.A[name])
        sfrom = self.Sanitize(from_t)
        if len(context) > 0:
            fs = "(" + sfrom + "," + "%d" % atype + "," + fwd + "," + to_t + "," + bwd + "," + context + ")\n"
        else:
            fs = "(" + sfrom + "," + "%d" % atype + "," + fwd + "," + to_t + "," + bwd + "," + "*" + ")\n"
            
        ofile.write(fs)

    ########################################################################################################

    def IGr(self,ofile,from_t, name, to_t, context):
        if from_t == to_t:
            return
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

    def RoleGr(self,ofile,compound_name,role,attributes,ex_context):
    
        self.Gr(ofile,compound_name,"a_hasrole",role,ex_context)
        
        if len(attributes) > 0:
            words = attributes.split(",")
            
            for word in words:
                self.Gr(ofile,compound_name,"a_hasattr",word,self.ALL_CONTEXTS);
                
        return compound_name

    ########################################################################################################

    def ContextGr(self,ofile,compound_name):
    
        if len(compound_name) > 0:
            words = compound_name.split(" ")
            
            for word in words:
                self.Gr(ofile,compound_name,"a_contains",word,self.ALL_CONTEXTS);
                
        return compound_name

    ########################################################################################################

    def Clue(self,ofile,who,what,whentime,where,how,why,icontext):

        if (whentime > 0):
            when = self.TimeGr(ofile,whentime);
        else:
            when = "regular check";

        event = who + " saw " + what + " at " + when + " location " + where + " " + how + " cause " + why
        attr = who + "," + what + "," + when + "," + where + "," + how + "," + why

        self.RoleGr(ofile,event,"event",attr,icontext)
        self.RoleGr(ofile,who,"who","",icontext)
        self.RoleGr(ofile,what,"what",what,icontext)
        self.RoleGr(ofile,how,"how",how,icontext)
        self.RoleGr(ofile,why,"why","",icontext)
        self.Gr(ofile,what,"a_caused_by",why,icontext)
        
    ########################################################################################################

    def TimeGr(self,ofile,now):
        
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

        self.RoleGr(ofile,hub,"when",attributes,self.ContextGr(ofile,"local clock time"));
        self.RoleGr(ofile,shift,"time of day","work shift","time");
        self.RoleGr(ofile,dow,"weekday","","clock time");
        self.RoleGr(ofile,day,"day of month","","clock time");
        self.RoleGr(ofile,month,"month","","clock time");
        self.RoleGr(ofile,year,"year","","clock time");
        self.RoleGr(ofile,hour,"hour","","clock time");
        self.RoleGr(ofile,month,"minutes past the hour","minutes","clock time");       

        return hub;


    # Could also use  WeekSlot (Mon-Sun,MinXX_YY),
    #                 MonthSlot (1stday, lastday, else DayN) etc

    ########################################################################################################

    def WhereGr(self,ofile,address,uqhn,domain,ipv4,ipv6):

        # VUQNAME, VDOMAIN, VIPADDRESS,NULL);
        # figure out my IP address, FQHN, domainname, etc...

        if len(domain) == 0:
            domain = "unknown domain";

        if len(ipv6) == 0:
            domain = "no ipv6"

        where = "host %s.%s IPv4 %s ipv6 %s" % (uqhn,domain,ipv4,ipv6)

        if len(address) > 0:
            attr = "hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s" % (uqhn,domain,ipv4,ipv6,address)
        else:
            attr = "hostname %s,domain %s,IPv4 %s,IPv6 %s" % (uqhn,domain,ipv4,ipv6)
             
        self.RoleGr(ofile,where,"where",attr,"host location identification");
  
        domainx = "domain %s" % domain
        self.RoleGr(ofile,domainx,"dns domain name",domain,"host location identification")

        hostname = "hostname %s" % uqhn
        self.RoleGr(ofile,hostname,"hostname",uqhn,"host location identification")
        self.Gr(ofile,where,"a_alias",hostname,"host location identification");  # Alias for quick association
        self.Gr(ofile,domainx,"a_contains",hostname,"host location identification");
        
        identity = "host identity %s" % uqhn
        self.Gr(ofile,hostname,"a_alias",identity,"host location identification");
            
        ipv4x = "ipv4 address %s" % ipv4
        self.RoleGr(ofile,ipv4x,"ipv4 address",ipv4,"host location identification");
        self.Gr(ofile,where,"a_alias",ipv4x,"host location identification");  # Alias for quick association
        self.Gr(ofile,domainx,"a_contains",ipv4x,"host location identification");
            
        identity = "host identity %s" % ipv4
        self.Gr(ofile,ipv4x,"a_alias",identity,"host location identification");
            
        if len(ipv6) > 0:
            ipv6x = "ipv6 address %s" % ipv6
            self.RoleGr(ofile,ipv6x,"ipv6 address", ipv6,"host location identification");
            self.Gr(ofile,where,"a_alias",ipv6x,"host location identification");  # Alias for quick association
            self.Gr(ofile,domainx,"a_contains",ipv6x,"host location identification");
            identity = "host identity %s" % ipv6
            self.Gr(ofile,ipv6x,"a_alias",identity,"host location identification")
            self.Gr(ofile,hostname,"a_alias",ipv6x,"host location identification");        
            
        if len(address) > 0:
            addressx = "description address %s" % address
            self.RoleGr(ofile,addressx,"description address",address,"host location identification");
            self.Gr(ofile,domainx,"a_origin",addressx,"host location identification");
            self.Gr(ofile,"description address","a_related_to","street address","host location identification");
            
        self.Gr(ofile,hostname,"a_alias",ipv4x,"host location identification");
            
        return where;

    ########################################################################################################

    def HereGr(self,ofile,address):

        # VUQNAME, VDOMAIN, VIPADDRESS,NULL);
        # figure out my IP address, FQHN, domainname, etc...

        id = "host localhost domain undefined ipv4 127.0.0.1 ipv6 ::1" # how can we make this the outer ip?

        print socket.gethostname()
        import netifaces
        macs = []
        ipv4s = []
        ipv6s = []
        
        for i in netifaces.interfaces():
            addrs = netifaces.ifaddresses(i)
            iface_details = netifaces.ifaddresses(i)
            
            if iface_details.has_key(netifaces.AF_INET):
                ipv4 = iface_details[netifaces.AF_INET]
                if ipv4 == "127.0.0.1":
                    continue
                ipv4s.extend(map(lambda x: x['addr'], filter(lambda x: x.has_key('addr'), ipv4)))
                
            if iface_details.has_key(netifaces.AF_INET6):
                ipv6 = iface_details[netifaces.AF_INET6]
                if ipv6 == "::1":
                    continue
                ipv6s.extend(map(lambda x: x['addr'], filter(lambda x: x.has_key('addr'), ipv6)))
                    
            if iface_details.has_key(netifaces.AF_LINK):
                mac = iface_details[netifaces.AF_LINK]
                macs.extend(map(lambda x: x['addr'], filter(lambda x: x.has_key('addr'), mac)))

        fqhn = socket.getfqdn()
        
        try:
            domain = fqhn.split()[1]
        except:
            domain = "unknown"

        try:
            mainv6 = ipv6s[1]
        except:
            mainv6 = "::1"

        try:
            mainv4 = ipv4s[1]
        except:
            return "127.0.0.1"

        uqhn = socket.gethostname()

        self.WhereGr(ofile,address,uqhn,domain,mainv4,mainv6)

        #print ipv4s
        #print ipv6s
        #print macs
        
        for ip in ipv4s:
            try:
                identity = "host identity %s" % socket.gethostbyaddr(ip)[0]
                self.Gr(ofile,identity,"a_alias",mainv4,"host location identification")
            except:
                identity = "host identity %s" % ip
            identity = "host identity %s" % ip
            self.Gr(ofile,identity,"a_alias",mainv4,"host location identification")

        for ip in ipv6s:
            try:
                identity = "host identity %s" % socket.gethostbyaddr(ip)[0]
                self.Gr(ofile,identity,"a_alias",mainv4,"host location identification")
            except:
                identity = "host identity %s" % ip
            identity = "host identity %s" % ip

            if not mainv6 == "::1":
                self.Gr(ofile,identity,"a_alias",mainv6,"host location identification")
            if not mainv4 == "127.0.0.1":
                self.Gr(ofile,identity,"a_alias",mainv4,"host location identification")

        for mac in macs:
            identity = "host identity %s" % mac
            if not mainv6 == "::1":
                self.Gr(ofile,identity,"a_alias",mainv6,"host location identification")
            if not mainv4 == "127.0.0.1":
                self.Gr(ofile,identity,"a_alias",mainv4,"host location identification")

        return id
    
    ########################################################################################################
    
    def ServiceGr(self,ofile,servicename,portnr):

        name = "service %s on port %d" % (servicename,portnr)
        service = "service %s" % servicename
        port = "ip portnumber %d" % portnr
        self.RoleGr(ofile,name,service,port,"service relationship")
         
        self.Gr(ofile,service,"a_hasrole","service","service relationship");
        self.Gr(ofile,service,"a_hasfunction",servicename,"service relationship");
         
        service = "ip portnumber %d" % portnr
        port = "%d" % portnr
        self.RoleGr(ofile,service,"ip portnumber",port,"service relationship");

        # ancillary notes
                        
        server = "%s server" % servicename
        self.Gr(ofile,server,"a_hasrole","server","service relationship");
                        
        client = "%s client" % servicename
        self.Gr(ofile,client,"a_hasrole","client","service relationship");

        self.Gr(ofile,client,"a_depends",server,"service relationship");
        self.Gr(ofile,client,"a_uses",name,"service relationship");
                        
        return name;

    ########################################################################################################

    def ServerInstanceGr(self,ofile,servicename,portnumber,servername,where):
        self.ServiceGr(ofile,servicename,portnumber)
        return self.ServiceInstance(ofile,"server",servername,servicename,where);

    ########################################################################################################

    def ClientInstanceGr(self,ofile,servicename,clientname,where):
        return self.ServiceInstance(ofile,"client",clientname,servicename,where)  

    ########################################################################################################

    def ServiceInstance(self,ofile,role,instancename,servicename,where):

        location = "located at %s" % where
        service = "%s %s" % (servicename,role)             # e.g. ftp server
        instance = "instance %s %s" % (role,instancename)  # e.g. instance server myhost
        rolehub = "%s %s %s" % (service,instance,location) # (ftp server) (instance server myhost) (located at WHERE)
        attr = "%s,%s" % (service,location)             # (ftp server),(located at WHERE)
        
        # Top level hub
        self.RoleGr(ofile,rolehub,instance,attr,"service relationship");
        
        # Attr hierarchy
        self.RoleGr(ofile,location,"where",where,"service relationship instance");
        self.RoleGr(ofile,service,role,servicename,"service relationship instance");
        
        attr = "%s,%s" % (role,instancename)               # (ftp server),(located at WHERE)
        self.RoleGr(ofile,instance,role,instancename,"service relationship instance");
        
        # Causation
        
        if role == "client":
            self.Gr(ofile,rolehub,"a_depends",servicename,"service relationship")
        else:
            self.Gr(ofile,servicename,"a_providedby",rolehub,"service relationship")
        
        return rolehub

    ########################################################################################################

    def ExceptionGr(self,ofile,origin,logmessage):

        # 2016-08-13T15:00:01.906160+02:00 linux-e2vo /usr/sbin/cron[23039]: pam_unix(crond:session): session opened for user root by (uid=0)
        # When                             where      who                    what                                                  (new who)
        # Why = (lifecycle state change, exception, ...)

        # ???
        self.Gr(ofile,origin,"a_related_to",logmessage,"???? TBD")

        return "something"

