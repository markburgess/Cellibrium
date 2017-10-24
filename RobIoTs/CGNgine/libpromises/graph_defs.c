/*****************************************************************************/
/*                                                                           */
/* File: graph_defs.c                                                        */
/*                                                                           */
/* (C) Mark Burgess                                                          */
/*                                                                           */
/*****************************************************************************/

#ifndef GRAPHDEF
#define GRAPHDEF 1

/* In this semantic basis, contains and exhibits are not completely
 orthogonal.  Expresses implies an exterior promise from a sub-agent
 or from within, while contains alone is an interior agent membership
 property

 The approach here requires a set of simple conventions allied with
 a method of hub decomposition. It is quite painstaking work to make
 something practical, but the general principles are simple. */

Association A[a_ass_dim+1] =
{
    {GR_CONTAINS,"contains","belongs to or is part of"},
    {GR_CONTAINS,"generalizes","is a special case of"},
    {GR_FOLLOWS,"may originate from","may be the source or origin of"},
    {GR_FOLLOWS,"may be provided by","may provide"},
    {GR_FOLLOWS,"is maintained by","maintains"},
    {GR_FOLLOWS,"depends on","partly determines"},
    {GR_FOLLOWS,"may be caused by","can cause"},
    {GR_FOLLOWS,"may use","may be used by"},
    {GR_EXPRESSES,"is called","is a name for"},
    {GR_EXPRESSES,"expresses an attribute","is an attribute of"},
    {GR_EXPRESSES,"promises","is promised by"},
    {GR_EXPRESSES,"has an instance or particular case","is a particular case of"},
    {GR_EXPRESSES,"has value or state","is the state or value of"},
    {GR_EXPRESSES,"has argument or parameter","is a parameter or argument of"},
    {GR_EXPRESSES,"has the role of","is a role fulfilled by"},
    {GR_EXPRESSES,"has the outcome","is the outcome of"},
    {GR_EXPRESSES,"has function","is the function of"},
    {GR_EXPRESSES,"has constraint","constrains"},
    {GR_EXPRESSES,"has interpretation","is interpreted from"},
    {GR_NEAR,"seen concurrently with","seen concurrently with"},
    {GR_NEAR,"also known as","also known as"},
    {GR_NEAR,"is approximately","is approximately"},
    {GR_NEAR,"may be related to","may be related to"},
    {0, NULL, NULL},
};


const char *GR_TYPES[6][2] =
{
    {"no type",                  "nothing"},
    {"similar to",               "similar to"},
    {"may follow from/caused by","may cause / precede"},
    {"contains",                 "may be part of"},
    {"may express",              "may be expressed by"},
    {"may be found in context",  "may be context for"}
};

/********************************************************************************/

const char *GR_DAY_TEXT[] =
{
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
    NULL
};

const char *GR_MONTH_TEXT[] =
{
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
    "December",
    NULL
};

const char *GR_SHIFT_TEXT[] =
{
    "Night",
    "Morning",
    "Afternoon",
    "Evening",
    NULL
};

/******************************************************************************/

void InitialGr(FILE *fp)
{
 // Basic axioms about causation (upstream/downstream principle)

 ContextGr(fp,"service relationship");
 ContextGr(fp,"system diagnostics");
 ContextGr(fp,"lifecycle state change");
 ContextGr(fp,"software exception");
 ContextGr(fp,"promise keeping");
 ContextGr(fp,"host location identification");
 
 Gr(fp,"client measurement anomaly",a_caused_by,"client software exception","system diagnostics");
 Gr(fp,"client measurement anomaly",a_caused_by,"server software exception","system diagnostics");
 Gr(fp,"server measurement anomaly",a_caused_by,"server software exception","system diagnostics");

 Gr(fp,"measurement anomaly",a_caused_by,"software exception","system diagnostics");

 Gr(fp,"resource contention",a_caused_by,"resource limit","system diagnostics");
 Gr(fp,"increasing queue length",a_caused_by,"resource contention","system diagnostics");
 Gr(fp,"system performance slow",a_caused_by,"increasing queue length","system diagnostics");
 Gr(fp,"system performance slow",a_related_to,"system performance latency","system diagnostics");
 
 Gr(fp,"system performance latency",a_caused_by,"resource contention","system diagnostics");
 Gr(fp,"system performance latency",a_caused_by,"increasing queue length","system diagnostics");

 Gr(fp,"system performance latency",a_caused_by,"server unavailability","system diagnostics");
 Gr(fp,"server unavailability",a_caused_by,"software crash","system diagnostics");
 Gr(fp,"server unavailability",a_caused_by,"system performance slow","system diagnostics");
}

/*****************************************************************************/

void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *sto = SanitizeString(to);
 char *scontext = SanitizeString(context);

 if (strcmp(sfrom,sto) == 0)
    {
    return;
    }
 
 if (context && strlen(context) > 1)
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,sto,A[assoc].bwd,scontext);
    }
 else
    {
    fprintf(consc,"(%s,%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,sto,A[assoc].bwd,ALL_CONTEXTS);
    }

 free(sfrom);
 free(sto);
 free(scontext);
}

/**********************************************************************/

void IGr(FILE *consc,char *from, enum associations assoc, char *to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *sto = SanitizeString(to);
 char *scontext = SanitizeString(context);

 if (strcmp(sfrom,sto) == 0)
    {
    return;
    } 

 if (context && strlen(context) > 1)
    {
    fprintf(consc,"(%s,-%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].bwd,sto,A[assoc].fwd,scontext);
    }
 else
    {
    fprintf(consc,"(%s,-%d,%s,%s,%s,%s)\n",sfrom,A[assoc].type,A[assoc].bwd,sto,A[assoc].fwd,ALL_CONTEXTS);
    }
     
 free(sfrom);
 free(sto);
 free(scontext);
}

/**********************************************************************/

void Number(FILE *consc, double q, char *context)
{
 enum associations assoc = a_hasrole;

 if (context && strlen(context) > 1)
    {
    fprintf(consc,"(%.2lf,%d,%s,%s,%s,%s)\n",q,A[assoc].type,A[assoc].fwd,"number",A[assoc].bwd,context);
    }
 else
    {
    fprintf(consc,"(%.2lf,%d,%s,%s,%s,%s)\n",q,A[assoc].type,A[assoc].fwd,"number",A[assoc].bwd,ALL_CONTEXTS);
    }
}

/**********************************************************************/

void GrQ(FILE *consc,char *from, enum associations assoc, double to, char *context)
{
 char *sfrom = SanitizeString(from);
 char *scontext = SanitizeString(context);

 if (context && strlen(context) > 1)
    {
    fprintf(consc,"(%s,%d,%s,%.2lf,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,scontext);
    }
 else
    {
    fprintf(consc,"(%s,%d,%s,%.2lf,%s,%s)\n",sfrom,A[assoc].type,A[assoc].fwd,to,A[assoc].bwd,ALL_CONTEXTS);
    }
 
 free(sfrom);
 free(scontext); 
}

/**********************************************************************/

char *RoleGr(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context)

/* Document a compound Split a comma separated list, with head
   we can use it for context or for conceptual
   RoleGr(fp, "compound name", "hasrole unique identifier", "hasttr part1,hasttr part2", "naming unique identity")
*/
    
{ char *sp, word[255];

 Gr(consc,compound_name,a_hasrole,role,ex_context);
 
 if ((sp = attributes))
    {
    while (*sp != '\0')
       {
       if (*sp == ',')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250[^,]",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_hasattr,word,ALL_CONTEXTS);
       }
    }

return compound_name;
}

/**********************************************************************/

char *ContextGr(FILE *consc,char *compound_name)

/* Document a compound Split a space separated list, with head
   we can use it for context or for conceptual - treat them as epitopes
   for fuzzy matching by set overlap. Only type 4 associations. */
    
{ char *sp, word[255];

 if ((sp = compound_name))
    {
    while (*sp != '\0')
       {
       if (*sp == ' ')
          {
          sp++;
          continue;
          }
       
       word[0] = '\0';
       sscanf(sp,"%250s",word);
       sp += strlen(word);

       Gr(consc,compound_name,a_contains,word,ALL_CONTEXTS);
       }
    }

return compound_name;
}

/************************************************************************************/

char *ServiceGr(FILE *fp,char *servicename, unsigned int portnumber)
{
 static char name[CGN_BUFSIZE];
 char port[CGN_BUFSIZE];
 
 snprintf(name,CGN_BUFSIZE,"%s on port %d",SService(servicename), portnumber);     // service ftp
 RoleGr(fp,name,SService(servicename),IPPort(portnumber),"service relationship");

 Gr(fp,SService(servicename),a_hasrole,"service","service relationship");
 Gr(fp,SService(servicename),a_hasfunction,servicename,"service relationship");

 snprintf(port,CGN_BUFSIZE,"%d",portnumber);
 RoleGr(fp,IPPort(portnumber),"ip portnumber",port,"service relationship");

 // ancillary notes
 
 Gr(fp,SServer(servicename),a_hasrole,"server","service relationship");
 Gr(fp,SClient(servicename),a_hasrole,"client","service relationship");

 Gr(fp,SClient(servicename),a_depends,SServer(servicename),"service relationship");  // service => client depends on server
 Gr(fp,SClient(servicename),a_uses,name,"service relationship");

 return name;
}

/************************************************************************************/

char *ServerInstanceGr(FILE *fp,char *servicename, unsigned int portnumber,char *servername,char *where)
{
 static char hub[CGN_BUFSIZE];
 ServiceGr(fp,servicename,portnumber);
 snprintf(hub,CGN_BUFSIZE,"%s %s",SServerInstance(servicename,servername),where);
 RoleGr(fp,hub,SServerInstance(servicename,servername),where,"service relationship instance");
 Gr(fp,SService(servicename),a_providedby,hub,"service relationship");
 return hub;
}

/************************************************************************************/

char *ClientInstanceGr(FILE *fp,char *servicename,char *clientname,char *where)
{
 static char hub[CGN_BUFSIZE];
 snprintf(hub,CGN_BUFSIZE,"%s %s",SClientInstance(servicename,clientname),where);
 RoleGr(fp,hub,SClientInstance(servicename,clientname),where,"service relationship instance");
 Gr(fp,hub,a_uses,SService(servicename),"service relationship");
 return hub;
}

/************************************************************************************/

char *GivePromiseGr(FILE *fp,char *S, char *R, char *body)
{
 char sender[CGN_BUFSIZE], receiver[CGN_BUFSIZE],attr[CGN_BUFSIZE];
 static char promisehub[CGN_BUFSIZE];
 snprintf(sender,CGN_BUFSIZE,"promiser %s",S);
 snprintf(receiver,CGN_BUFSIZE,"promisee %s",R);

 snprintf(promisehub,CGN_BUFSIZE,"%s promises to give %s to %s",sender,body,receiver);
 snprintf(attr,CGN_BUFSIZE,"%s,promise body +%s,%s",sender,body,receiver);
 RoleGr(fp,promisehub,"give-provide promise",attr,"promise keeping");
 
 Gr(fp,sender,a_depends,promisehub,"promise keeping");
 Gr(fp,promisehub,a_depends,sender,"promise keeping");

 return promisehub;
}

/************************************************************************************/

char *AcceptPromiseGr(FILE *fp,char *R, char *S, char *body)
{
 char sender[CGN_BUFSIZE], receiver[CGN_BUFSIZE],attr[CGN_BUFSIZE];
 static char promisehub[CGN_BUFSIZE];
 snprintf(receiver,CGN_BUFSIZE,"promiser %s",R);
 snprintf(sender,CGN_BUFSIZE,"promisee %s",S);

 snprintf(promisehub,CGN_BUFSIZE,"%s promises to accept %s to %s",receiver,body,sender);
 snprintf(attr,CGN_BUFSIZE,"%s,promise body -%s,%s",sender,body,receiver);
 RoleGr(fp,promisehub,"use-accept promise",attr,"promise keeping");

 Gr(fp,receiver,a_depends,promisehub,"promise keeping");
 Gr(fp,"use-accept promise",a_related_to,"client pull methods","promise keeping");
 
 return promisehub;
}

/************************************************************************************/

char *ImpositionGr(FILE *fp,char *S, char *R, char *body)
{
 char sender[CGN_BUFSIZE], receiver[CGN_BUFSIZE],attr[CGN_BUFSIZE];
 char *acceptance;
 static char promisehub[CGN_BUFSIZE];
 snprintf(sender,CGN_BUFSIZE,"imposer %s",S);
 snprintf(receiver,CGN_BUFSIZE,"imposee %s",R);

 snprintf(promisehub,CGN_BUFSIZE,"%s imposes body %s onto %s",sender,body,receiver);
 snprintf(attr,CGN_BUFSIZE,"%s,imposition body %s,%s",sender,body,receiver);
 RoleGr(fp,promisehub,"imposition",attr,"promise keeping");
 Gr(fp,"imposition",a_related_to,"client push methods","promise keeping");

 // Imposition only affects if there is an accept promise

 if ((acceptance = AcceptPromiseGr(fp,R,S,body)))
    {
    Gr(fp,promisehub,a_depends,acceptance,"promise keeping");
    Gr(fp,promisehub,a_depends,sender,"promise keeping");
    }
 
 return promisehub;
}

/************************************************************************************/

char *ClientQuery(FILE *fp,char *client, char *server, char *request, char *servicename, int portnumber)
{
 static char query[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE], id[CGN_BUFSIZE];

 snprintf(attr,CGN_BUFSIZE,"port %d",portnumber);
 char p[8];
 snprintf(p,8,"%d",portnumber);
 RoleGr(fp,attr,"port",p,"client service query");

 snprintf(query,CGN_BUFSIZE,"%s requests %s from %s on port %d",SClientInstance(servicename,client),request,SServerInstance(servicename,server),portnumber);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,port %d",SClientInstance(servicename,client),SServerInstance(servicename,server),portnumber);
 snprintf(id,CGN_BUFSIZE,"query request for %s",request);
 RoleGr(fp,query,id,attr,"service relationship");

 // Causal model

 snprintf(attr,CGN_BUFSIZE,"request %s from service %s port %d",request,servicename,portnumber);
 ImpositionGr(fp,SClientInstance(servicename,client),SServerInstance(servicename,server),attr);
 return query;
}

/************************************************************************************/

char *ClientPush(FILE *fp,char *client, char *server, char *request, char *servicename, int portnumber)
{
 static char query[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE], id[CGN_BUFSIZE];

 snprintf(attr,CGN_BUFSIZE,"port %d",portnumber);
 char p[8];
 snprintf(p,8,"%d",portnumber);
 RoleGr(fp,attr,"port",p,"client service query");

 snprintf(query,CGN_BUFSIZE,"%s pushes %s to %s on port %d",SClientInstance(servicename,client),request,SServerInstance(servicename,server),portnumber);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,port %d",SClientInstance(servicename,client),SServerInstance(servicename,server),portnumber);
 snprintf(id,CGN_BUFSIZE,"query pushes %s",request);
 RoleGr(fp,query,id,attr,"service relationship");

 // Causal model

 snprintf(attr,CGN_BUFSIZE,"push %s to service %s port %d",request,servicename,portnumber);
 ImpositionGr(fp,SClientInstance(servicename,client),SServerInstance(servicename,server),attr);
 return query;
}

/************************************************************************************/

char *ServerListenPromise(FILE *fp,char *servername, char *servicename, int port)
{
 static char listen[CGN_BUFSIZE];
 char ports[CGN_BUFSIZE],attr[CGN_BUFSIZE];
 
 snprintf(listen,CGN_BUFSIZE,"%s listens for requests on port %d",SServerInstance(servicename,servername),port);
 snprintf(attr,CGN_BUFSIZE,"%s,port %d",SServerInstance(servicename,servername),port);
 RoleGr(fp,listen,"listen on service port",attr,"service relationship");

  // Causation
 
 snprintf(ports,CGN_BUFSIZE,"listening on port %d",port);
 GivePromiseGr(fp,SServerInstance(servicename,servername),"ip INADDR_ANY",ports);
 return listen;
}

/************************************************************************************/

char *ServerAcceptPromise(FILE *fp,char *servername, char *fromclient, char *servicename, int port)
{
 static char accept[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE],id[CGN_BUFSIZE];

 snprintf(accept,CGN_BUFSIZE,"%s accept data from %s on port %d",SServerInstance(servicename,servername),SClientInstance(servicename,fromclient),port);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,%s",SServerInstance(servicename,servername),SClientInstance(servicename,fromclient),IPPort(port));
 snprintf(id,CGN_BUFSIZE,"accept data on port %d",port);
 RoleGr(fp,accept,id,attr,"service relationship");
 
 AcceptPromiseGr(fp,SServerInstance(servicename,servername),SClientInstance(servicename,fromclient),id); 
 return accept;
}

/************************************************************************************/

char *ServerReplyPromise(FILE *fp,char *servername, char *toclient, char *servicename, int port)
{
 static char reply[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE],id[CGN_BUFSIZE];

 snprintf(reply,CGN_BUFSIZE,"%s reply to %s from port %d",SServerInstance(servicename,servername),SClientInstance(servicename,toclient),port);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,%s",SServerInstance(servicename,servername),SClientInstance(servicename,toclient),IPPort(port));
 snprintf(id,CGN_BUFSIZE,"reply to queries from port %d",port);
 RoleGr(fp,reply,id,attr,"service relationship");
 GivePromiseGr(fp,SServerInstance(servicename,servername),SClientInstance(servicename,toclient),id); 
 return reply;
}

/************************************************************************************/

char *ClientWritePostData(FILE *fp,char *client, char *server, char *data,char *servicename, int portnumber)
{
 return ClientPush(fp,client,server,data,servicename,portnumber);
}

/************************************************************************************/

char *ClientReadGetData(FILE *fp,char *client, char *server, char *servicename, char *get, int portnumber)
{
 return ClientQuery(fp,client,server,get,servicename,portnumber);
}

/************************************************************************************/

char *ServerAcceptPostData(FILE *fp,char *server,char *client,char *servicename, char *data)
{
 char request[CGN_BUFSIZE];
 snprintf(request,CGN_BUFSIZE,"accept %.64s to %s request",data,SService(servicename));
 return AcceptPromiseGr(fp,server,client,request);
}

/************************************************************************************/

char *ServerReplyToGetData(FILE *fp,char *server,char *client,char *servicename, char *data)
{
 char request[CGN_BUFSIZE];
 snprintf(request,CGN_BUFSIZE,"conditional reply %.64s to %s request",data,SService(servicename));
 return GivePromiseGr(fp,server,client,request); 
}

/************************************************************************************/

char *ExceptionGr(FILE *fp,char *origin,char *logmessage)

// 2016-08-13T15:00:01.906160+02:00 linux-e2vo /usr/sbin/cron[23039]: pam_unix(crond:session): session opened for user root by (uid=0)
// When                             where      who                    what                                                  (new who)
// Why = (lifecycle state change, exception, ...)

{
 Gr(fp,origin,a_related_to,logmessage,"???? TBD");
 return NULL;
}

/************************************************************************************/

char *EventClue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext)
{
 static char event[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE];
 char *when;

 // Normal regular events do not depend on time, and should not record an irrelevant timestamp
 // Anomalies do depend on time and timestamp may be significant
 
 if (whentime > 0)
    {
    when = TimeGr(fp,whentime);
    }
 else
    {
    when = "repeated event";
    }
 
 snprintf(event,CGN_BUFSIZE,"%s saw %s %s location %s %s cause %s",who,what,when,where,how,why);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,%s,%s,%s",who,what,when,how,why);
 RoleGr(fp,event,"event",attr,icontext);
 
 RoleGr(fp,who,"who","",icontext);
 RoleGr(fp,what,"what","",icontext);
 RoleGr(fp,how,"how","",icontext);
 RoleGr(fp,why,"why","",icontext);

 Gr(fp,what,a_related_to,why,icontext);
 return event;
}

/************************************************************************************/

char *TimeGr(FILE *fp,time_t time)
{
 int tz;
 char hub[2][CGN_BUFSIZE];
 const char* tz_prefix[2] = { "", "GMT_" };
 const char* tz_function[2] = { "localtime_r", "gmtime_r" };
 struct tm tz_parsed_time[2];
 const struct tm* tz_tm[2] =
     {
     localtime_r(&time, &(tz_parsed_time[0])),
     gmtime_r(&time, &(tz_parsed_time[1]))
     };

 for (tz = 0; tz < 2; tz++)
    {
    char lifecycle[CGN_BUFSIZE];
    char year[CGN_BUFSIZE];
    char month[CGN_BUFSIZE];
    char dow[CGN_BUFSIZE];
    char day[CGN_BUFSIZE];
    char hour[CGN_BUFSIZE];
    char shift[CGN_BUFSIZE];
    char min[CGN_BUFSIZE];
    char mins[CGN_BUFSIZE];
    char quart[CGN_BUFSIZE];
        
    int day_text_index, quarter, interval_start, interval_end;
    
    if (tz_tm[tz] == NULL)
       {
       printf("Unable to parse passed time");
       perror(tz_function[tz]);
       return NULL;
       }
    
/* Lifecycle */

    snprintf(lifecycle, CGN_BUFSIZE, "%sLcycle_%d", tz_prefix[tz], ((tz_parsed_time[tz].tm_year + 1900) % 3));
    
/* Year */
    
    snprintf(year, CGN_BUFSIZE, "%sYr%04d", tz_prefix[tz], tz_parsed_time[tz].tm_year + 1900);
    
/* Month */
    
    snprintf(month, CGN_BUFSIZE, "%s%s", tz_prefix[tz], GR_MONTH_TEXT[tz_parsed_time[tz].tm_mon]);
    
/* Day of week */
    
/* Monday  is 1 in tm_wday, 0 in GR_DAY_TEXT
   Sunday  is 0 in tm_wday, 6 in GR_DAY_TEXT */
    day_text_index = (tz_parsed_time[tz].tm_wday + 6) % 7;
    snprintf(dow, CGN_BUFSIZE, "%s%s", tz_prefix[tz], GR_DAY_TEXT[day_text_index]);
    
/* Day */
    
    snprintf(day, CGN_BUFSIZE, "%sDay%d", tz_prefix[tz], tz_parsed_time[tz].tm_mday);

    
/* Shift */
    
    snprintf(shift, CGN_BUFSIZE, "%s%s", tz_prefix[tz], GR_SHIFT_TEXT[tz_parsed_time[tz].tm_hour / 6]);

    
/* Hour */
    
    snprintf(hour, CGN_BUFSIZE, "%sHr%02d", tz_prefix[tz], tz_parsed_time[tz].tm_hour);

//    snprintf(buf, CGN_BUFSIZE, "%sHr%d", tz_prefix[tz], tz_parsed_time[tz].tm_hour);

    
/* Quarter */
    
    quarter = tz_parsed_time[tz].tm_min / 15 + 1;

    snprintf(quart, CGN_BUFSIZE, "%sQ%d", tz_prefix[tz], quarter);
    
/* Minute */
    
    snprintf(min, CGN_BUFSIZE, "%sMin%02d", tz_prefix[tz], tz_parsed_time[tz].tm_min);

    
    interval_start = (tz_parsed_time[tz].tm_min / 5) * 5;
    interval_end = (interval_start + 5) % 60;
    
    snprintf(mins, CGN_BUFSIZE, "%sMin%02d_%02d", tz_prefix[tz], interval_start, interval_end);

    // Summary
    
    snprintf(hub[tz], CGN_BUFSIZE,"on %s %s %s %s %s at %s %s %s",shift,dow,day,month,year,hour,mins,quart);    

    char attributes[CGN_BUFSIZE];

    switch (tz)
       {
       case 0:
           snprintf(attributes, CGN_BUFSIZE,"%s,%s,%s,%s,%s,%s,%s,%s",shift,dow,day,month,year,hour,mins,quart);           
           RoleGr(fp,hub[0],"when",attributes,ContextGr(fp,"local clock time"));
           break;
       default:
           snprintf(attributes, CGN_BUFSIZE,"%s,%s,%s,%s,%s,%s,%s,%s",shift,dow,day,month,year,hour,mins,quart);
           RoleGr(fp,hub[1],"when GMT",attributes,ContextGr(fp,"GMT clock time"));
           break;
       }
    
    RoleGr(fp,shift,"time of day","work shift","time");
    RoleGr(fp,dow,"weekday","","clock time");
    RoleGr(fp,day,"day of month","","clock time");
    RoleGr(fp,month,"month","","clock time");
    RoleGr(fp,year,"year","","clock time");
    RoleGr(fp,hour,"hour","","clock time");
    RoleGr(fp,month,"minutes past the hour","minutes","clock time");       
    }

 static char retval[CGN_BUFSIZE];
 snprintf(retval, CGN_BUFSIZE,"%s,%s",hub[0],hub[1]);
 return retval;
}

/************************************************************************************/

char *WhereGr(FILE *fp,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6)
{
 // This model of "where?" is based on IP addresses and portnumbers, for cloud services
 // alternative models for "other worlds" can be added...
 
 static char where[CGN_BUFSIZE] = {0};
 char attr[CGN_BUFSIZE];
 
 if (domain == NULL || strlen(domain) == 0)
    {
    domain = "unknown domain";
    }

  if (ipv6 == NULL || strlen(ipv6) == 0)
    {
    ipv6 = "";
    }

  snprintf(where,CGN_BUFSIZE,"host location %s.%s IPv4 %s ipv6 %s",uqhn,domain,ipv4,ipv6);

  if (address && strlen(address) > 0)
     {
     snprintf(attr,CGN_BUFSIZE,"%s,%s,%s,%s,address %s",Hostname(uqhn),Domain(domain),IPv4(ipv4),IPv6(ipv6),address);
     }
  else
     {
     snprintf(attr,CGN_BUFSIZE,"%s,%s,%s,%s",Hostname(uqhn),Domain(domain),IPv4(ipv4),IPv6(ipv6));
     }

  RoleGr(fp,where,"where",attr, "host location identification");
  
  RoleGr(fp,Domain(domain),"dns domain name",domain,"host location identification");

  char *hostname = Hostname(uqhn);
  RoleGr(fp,hostname,"hostname",uqhn,"host location identification");
  Gr(fp,where,a_alias,hostname,"host location identification");  // Alias for quick association
  Gr(fp,Domain(domain),a_contains,hostname,"host location identification");

  char *identity = HostID(uqhn);
  Gr(fp,hostname,a_alias,identity,"host location identification");
  
  RoleGr(fp,IPv4(ipv4),"ipv4 address", ipv4,"host location identification");
  Gr(fp,where,a_alias,IPv4(ipv4),"host location identification");  // Alias for quick association
  Gr(fp,Domain(domain),a_contains,IPv4(ipv4),"host location identification");
  Gr(fp,IPv4(ipv4),a_alias,HostID(ipv4),"host location identification");

  if (ipv6 && strlen(ipv6) > 0)
     {
     RoleGr(fp,IPv6(ipv6),"ipv6 address", ipv6,"host location identification");
     Gr(fp,where,a_alias,IPv6(ipv6),"host location identification");  // Alias for quick association
     Gr(fp,Domain(domain),a_contains,IPv6(ipv6),"host location identification");
     identity = HostID(ipv6);
     Gr(fp,IPv6(ipv6),a_alias,identity,"host location identification");
     Gr(fp,hostname,a_alias,IPv6(ipv6),"host location identification");
     }

  if (address && address > 0)
     {
     char addressx[CGN_BUFSIZE];
     snprintf(addressx,CGN_BUFSIZE,"description address %s",address);
     RoleGr(fp,addressx,"description address",address,"host location identification");
     Gr(fp,Domain(domain),a_origin,addressx,"host location identification");
     Gr(fp,"description address",a_related_to,"street address","host location identification");
     }
  
  Gr(fp,hostname,a_alias,IPv4(ipv4),"host location identification");

  return where;
}

/**********************************************************************/

char *SClientInstance(char *service,char *client)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"%s client %s",service,client);
 return ret;
}


char *SServerInstance(char *service,char *server)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"%s server %s",service,server);
 return ret;
}


char *SClient(char *service)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"%s client",service);
 return ret;
}


char *SServer(char *service)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"%s server",service);
 return ret;
}

char *SService(char *servicename)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"service %s",servicename);
 return ret;
}

char *HostID(char *id) // Generic ID
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"host identity %s",id);
 return ret;
}

char *IPv4(char *id) // specific ID
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"ipv4 address %s",id);
 return ret;
}

char *IPv6(char *id) // specific ID
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"ipv6 address %s",id);
 return ret;
}

char *Hostname(char *id) // specific ID
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"hostname %s",id);
 return ret;
}

char *Domain(char *id) // specific ID
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"domain %s",id);
 return ret;
}

char *IPPort(int port)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"ip portnumber %d",port);
 return ret;
}

char *SUser(char *name)
{
 static char ret[CGN_BUFSIZE];
 snprintf(ret,CGN_BUFSIZE,"username %s",name);
 return ret;
}

/**********************************************************************/

char *SanitizeString(char *s)
{
 if (s == NULL)
    {
    return NULL;
    }
 
 char *sp, *str = strdup(s);
 for (sp = str; *sp != '\0'; sp++)
    {
    switch (*sp)
       {
       case ',':
           *sp = ' ';
           break;
       default:
           break;
       }
    }
 return str;
}


#endif
