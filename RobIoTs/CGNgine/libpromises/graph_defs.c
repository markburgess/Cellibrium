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
 char server[CGN_BUFSIZE],client[CGN_BUFSIZE];
 char port[CGN_BUFSIZE], service[CGN_BUFSIZE];
 
 snprintf(name,CGN_BUFSIZE,"service %s on port %d",servicename, portnumber);     // service ftp

 snprintf(service,CGN_BUFSIZE,"service %s",servicename); 
 snprintf(port,CGN_BUFSIZE,"ip portnumber %d",portnumber);
 RoleGr(fp,name,service,port,"service relationship");

 Gr(fp,service,a_hasrole,"service","service relationship");
 Gr(fp,service,a_hasfunction,servicename,"service relationship");

 snprintf(service,CGN_BUFSIZE,"ip portnumber %d",portnumber);
 snprintf(port,CGN_BUFSIZE,"%d",portnumber);
 RoleGr(fp,service,"ip portnumber",port,"service relationship");

 // ancillary notes
 
 snprintf(server,CGN_BUFSIZE,"%s server",servicename);   // ftp server
 Gr(fp,server,a_hasrole,"server","service relationship");

 snprintf(client,CGN_BUFSIZE,"%s client",servicename);   // ftp client
 Gr(fp,client,a_hasrole,"client","service relationship");

 Gr(fp,client,a_depends,server,"service relationship");  // service => client depends on server
 Gr(fp,client,a_uses,name,"service relationship");

 return name;
}

/************************************************************************************/

char *ServerInstanceGr(FILE *fp,char *servicename, unsigned int portnumber,char *servername,char *where)
{
 ServiceGr(fp,servicename,portnumber);
 return ServiceInstance(fp,"server",servername,servicename,where);
}

/************************************************************************************/

char *ClientInstanceGr(FILE *fp,char *servicename,char *clientname,char *where)
{
 return ServiceInstance(fp,"client",clientname,servicename,where);
}

/************************************************************************************/

char *ServiceInstance(FILE *fp,char *role, char *instancename,char *servicename, char *where)
{
 static char rolehub[CGN_BUFSIZE], attr[CGN_BUFSIZE];
 char location[CGN_BUFSIZE], service[CGN_BUFSIZE], instance[CGN_BUFSIZE];

 snprintf(location,CGN_BUFSIZE,"located at %s",where);
 snprintf(service,CGN_BUFSIZE,"%s %s",servicename,role);  // e.g. ftp server
 snprintf(instance,CGN_BUFSIZE,"instance %s %s",role,instancename); // e.g. instance server myhost

 snprintf(rolehub,CGN_BUFSIZE,"%s %s %s",service,instance,location); //(ftp server) (instance server myhost) (located at WHERE)
 snprintf(attr,CGN_BUFSIZE,"%s,%s",service,location); //(ftp server),(located at WHERE)

 // Top level hub
 RoleGr(fp,rolehub,instance,attr,"service relationship");

 // Attr hierarchy
 RoleGr(fp,location,"where",where,"service relationship instance");
 RoleGr(fp,service,role,servicename,"service relationship instance");

 snprintf(attr,CGN_BUFSIZE,"%s,%s",role,instancename); //(ftp server),(located at WHERE)
 RoleGr(fp,instance,role,instancename,"service relationship instance");

 // Causation
 
 if (strcmp(role,"client") == 0)
    {
    Gr(fp,rolehub,a_depends,servicename,"service relationship");
    }
 else //server
    {
    Gr(fp,servicename,a_providedby,rolehub,"service relationship");
    }

 return rolehub;
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
 char attr[CGN_BUFSIZE];

 snprintf(attr,CGN_BUFSIZE,"client %s",client);
 RoleGr(fp,attr,"client",client,"client service query");
 
 snprintf(attr,CGN_BUFSIZE,"request %s",request);
 RoleGr(fp,attr,"service request",request,"client service query");

 snprintf(attr,CGN_BUFSIZE,"server %s",server);
 RoleGr(fp,attr,"server",server,"client service query");

 snprintf(attr,CGN_BUFSIZE,"service %s",servicename);
 RoleGr(fp,attr,"service",servicename,"client service query");

 snprintf(attr,CGN_BUFSIZE,"port %d",portnumber);
 char p[8];
 snprintf(p,8,"%d",portnumber);
 RoleGr(fp,attr,"port",p,"client service query");

 snprintf(query,CGN_BUFSIZE,"client %s requests %s from service %s at server %s on port %d",client,request,servicename,server,portnumber);
 snprintf(attr,CGN_BUFSIZE,"client %s,request %s,server %s,service %s,port %d",client,request,server,servicename,portnumber);
 RoleGr(fp,query,"client query",attr,"service relationship");

 // Causal model

 snprintf(attr,CGN_BUFSIZE,"request %s from service %s port %d",request,servicename,portnumber),
 ImpositionGr(fp,client,server,attr);
 return query;
}

/************************************************************************************/

char *ClientPush(FILE *fp,char *client, char *server, char *request, char *servicename, int portnumber)
{
 static char query[CGN_BUFSIZE]; 
 char attr[CGN_BUFSIZE];
 
 snprintf(query,CGN_BUFSIZE,"client %s pushes %s to service %s at server %s on port %d",client,request,servicename,server,portnumber);

 snprintf(attr,CGN_BUFSIZE,"client %s",client);
 RoleGr(fp,attr,"client",client,"client service query");
 
 snprintf(attr,CGN_BUFSIZE,"request %s",request);
 RoleGr(fp,attr,"service request",request,"client service query");

 snprintf(attr,CGN_BUFSIZE,"server %s",server);
 RoleGr(fp,attr,"server",server,"client service query");

 snprintf(attr,CGN_BUFSIZE,"service %s",servicename);
 RoleGr(fp,attr,"service",servicename,"client service query");

 snprintf(attr,CGN_BUFSIZE,"port %d",portnumber);
 char p[8];
 snprintf(p,8,"%d",portnumber);
 RoleGr(fp,attr,"port",p,"client service query");

 snprintf(attr,CGN_BUFSIZE,"client %s,request %s,server %s,service %s,port %d",client,request,server,servicename,portnumber);
 RoleGr(fp,query,"client push",attr,"service relationship");

 // Causal model

 snprintf(attr,CGN_BUFSIZE,"request %s from service %s port %d",request,servicename,portnumber),
 ImpositionGr(fp,client,server,attr); 
 RoleGr(fp,query,"client push",attr,"service relationship");

 return query;
}

/************************************************************************************/

char *ServerListen(FILE *fp,char *servername, char *servicename, int port)
{
 static char query[CGN_BUFSIZE];
 char ports[CGN_BUFSIZE],attr[CGN_BUFSIZE];
 
 snprintf(query,CGN_BUFSIZE,"server %s listens for service %s requests on port %d",servername,servicename,port);
 snprintf(attr,CGN_BUFSIZE,"server %s,service %s,port %d",servername,servicename,port);
 RoleGr(fp,query,"server",attr,"service relationship");
 
 snprintf(ports,CGN_BUFSIZE,"listening on port %d",port);
    
 // Causation
 
 GivePromiseGr(fp,servername,"ip INADDR_ANY",ports);
 return query;
}

/************************************************************************************/

char *ServerAccept(FILE *fp,char *servername, char *fromclient, char *servicename, int port)
{
 static char query[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE];
 snprintf(query,CGN_BUFSIZE,"server %s accept data from client %s via port %d for service %s",servername,fromclient,port,servicename);
 snprintf(attr,CGN_BUFSIZE,"server %s,service %s,port %d,client %s",servername,servicename,port,fromclient);
 RoleGr(fp,query,"server accept",attr,"service relationship");
 AcceptPromiseGr(fp,servername,fromclient,"accept data"); 
 return query;
}

/************************************************************************************/

char *ServerReply(FILE *fp,char *server, char *toclient, char *servicename, int port)
{
 static char data[CGN_BUFSIZE];
 snprintf(data,CGN_BUFSIZE,"port query result %d",port);
 return ReplyToGetData(fp,server,toclient,servicename,data);
}

/************************************************************************************/

char *WritePostData(FILE *fp,char *client, char *server, char *data,char *servicename, int portnumber)
{
 return ClientPush(fp,client,server,data,servicename,portnumber);
}

/************************************************************************************/

char *ReadGetData(FILE *fp,char *client, char *server, char *servicename, char *get, int portnumber)
{
 return ClientQuery(fp,client,server,get,servicename,portnumber);
}

/************************************************************************************/

char *AcceptPostData(FILE *fp,char *server,char *client,char *servicename, char *data)
{
 char request[CGN_BUFSIZE];
 snprintf(request,CGN_BUFSIZE,"conditional reply %.64s to service %s request",data,servicename);
 return AcceptPromiseGr(fp,server,client,request);
}

/************************************************************************************/

char *ReplyToGetData(FILE *fp,char *server,char *client,char *servicename, char *data)
{
 char request[CGN_BUFSIZE];
 snprintf(request,CGN_BUFSIZE,"conditional reply %.64s to service %s request",data,servicename);
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

char *Clue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext)
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
    when = "regular check";
    }
 
 snprintf(event,CGN_BUFSIZE,"%s saw %s %s location %s %s cause %s",who,what,when,where,how,why);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,%s,%s,%s",who,what,when,how,why);
 RoleGr(fp,event,"event",attr,icontext);
 
 RoleGr(fp,who,"who","",icontext);
 RoleGr(fp,what,"what",what,icontext);
 RoleGr(fp,how,"how",how,icontext);
 RoleGr(fp,why,"why","",icontext);

 Gr(fp,what,a_caused_by,why,icontext);
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
    
    snprintf(hub[tz], CGN_BUFSIZE,"on %s %s %s %s %s at %s %s %s",shift,dow,day,month,year,hour,min,quart);    

    char attributes[CGN_BUFSIZE];

    switch (tz)
       {
       case 0:
           snprintf(attributes, CGN_BUFSIZE,"%s,%s,%s,%s,%s,%s,%s,%s",shift,dow,day,month,year,hour,min,quart);           
           RoleGr(fp,hub[0],"when",attributes,ContextGr(fp,"local clock time"));
           break;
       default:
           snprintf(attributes, CGN_BUFSIZE,"%s,%s,%s,%s,%s,%s,%s,%s",shift,dow,day,month,year,hour,min,quart);
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
    domain = "no ipv6";
    }

  snprintf(where,CGN_BUFSIZE,"host location %s.%s IPv4 %s ipv6 %s",uqhn,domain,ipv4,ipv6);

  if (address && strlen(address) > 0)
     {
     snprintf(attr,CGN_BUFSIZE,"hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s",uqhn,domain,ipv4,ipv6,address);
     }
  else
     {
     snprintf(attr,CGN_BUFSIZE,"hostname %s,domain %s,IPv4 %s,IPv6 %s",uqhn,domain,ipv4,ipv6);
     }

  RoleGr(fp,where,"where",attr, "host location identification");
  
  char domainx[CGN_BUFSIZE];
  snprintf(domainx,CGN_BUFSIZE,"domain %s",domain);
  RoleGr(fp,domainx,"dns domain name",domain,"host location identification");

  char hostname[CGN_BUFSIZE];
  snprintf(hostname,CGN_BUFSIZE,"hostname %s",uqhn);
  RoleGr(fp,hostname,"hostname",uqhn,"host location identification");
  Gr(fp,where,a_alias,hostname,"host location identification");  // Alias for quick association
  Gr(fp,domainx,a_contains,hostname,"host location identification");

  char identity[CGN_BUFSIZE];
  snprintf(identity,CGN_BUFSIZE,"host identity %s",uqhn);
  Gr(fp,hostname,a_alias,identity,"host location identification");
  
  char ipv4x[CGN_BUFSIZE];
  snprintf(ipv4x,CGN_BUFSIZE,"ipv4 address %s",ipv4);
  RoleGr(fp,ipv4x,"ipv4 address", ipv4,"host location identification");
  Gr(fp,where,a_alias,ipv4x,"host location identification");  // Alias for quick association
  Gr(fp,domainx,a_contains,ipv4x,"host location identification");

  snprintf(identity,CGN_BUFSIZE,"host identity %s",ipv4);
  Gr(fp,ipv4x,a_alias,identity,"host location identification");

  if (ipv6 && strlen(ipv6) > 0)
     {
     char ipv6x[CGN_BUFSIZE];
     snprintf(ipv6x,CGN_BUFSIZE,"ipv6 address %s",ipv6);
     RoleGr(fp,ipv6x,"ipv6 address", ipv6,"host location identification");
     Gr(fp,where,a_alias,ipv6x,"host location identification");  // Alias for quick association
     Gr(fp,domainx,a_contains,ipv6x,"host location identification");
     snprintf(identity,CGN_BUFSIZE,"host identity %s",ipv6);
     Gr(fp,ipv6x,a_alias,identity,"host location identification");
     Gr(fp,hostname,a_alias,ipv6x,"host location identification");
     }

  if (address && address > 0)
     {
     char addressx[CGN_BUFSIZE];
     snprintf(addressx,CGN_BUFSIZE,"description address %s",address);
     RoleGr(fp,addressx,"description address",address,"host location identification");
     Gr(fp,domainx,a_origin,addressx,"host location identification");
     Gr(fp,"description address",a_related_to,"street address","host location identification");
     }
  
  Gr(fp,hostname,a_alias,ipv4x,"host location identification");

  return where;
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
       case '/':
       case '\\':
           *sp = '%';
           break;
       default:
           break;
       }
    }
 return str;
}


#endif
