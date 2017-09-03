/*****************************************************************************/
/*                                                                           */
/* File: graph_defs.c                                                        */
/*                                                                           */
/* (C) Mark Burgess                                                          */
/*                                                                           */
/*****************************************************************************/

#ifndef GRAPHDEF
#define GRAPHDEF 1

/* In this semantic basis, contains and exhibits are not completely orthogonal.
 Expresses implies an exterior promise from a sub-agent or from
 within, while contains alone is an interior agent membership property */

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

void InitialCluster(FILE *fp)
{
 // Basic axioms about causation (upstream/downstream principle)

 ContextCluster(fp,"service relationship");
 ContextCluster(fp,"system diagnostics");
 ContextCluster(fp,"lifecycle state change");
 ContextCluster(fp,"software exception");

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

char *RoleCluster(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context)

/* Document a compound Split a comma separated list, with head
   we can use it for context or for conceptual
   RoleCluster(fp, "compound name", "hasrole unique identifier", "hasttr part1,hasttr part2", "naming unique identity")
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

char *ContextCluster(FILE *consc,char *compound_name)

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

char *ServiceCluster(FILE *fp,char *servicename)
{
 static char name[CGN_BUFSIZE],server[CGN_BUFSIZE],client[CGN_BUFSIZE];

 snprintf(name,CGN_BUFSIZE,"service %s",servicename);
 Gr(fp,name,a_hasrole,"service","service relationship");
 Gr(fp,name,a_hasfunction,servicename,"service relationship");

 snprintf(server,CGN_BUFSIZE,"%s server",servicename);
 Gr(fp,server,a_hasrole,"server","service relationship");

 snprintf(client,CGN_BUFSIZE,"%s client",servicename);
 Gr(fp,client,a_hasrole,"client","service relationship");

 Gr(fp,client,a_depends,server,"service relationship");
 Gr(fp,client,a_uses,name,"service relationship");

 return name;
}

/************************************************************************************/

char *ServerCluster(FILE *fp,char *servicename,char *servername,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6, unsigned int portnumber)
{
 char *where = WhereCluster(fp,address,uqhn,domain,ipv4,ipv6,portnumber);
 ServiceCluster(fp,servicename);
 return ServiceInstance(fp,"server",servername,servicename,where);
}

/************************************************************************************/

char *ClientCluster(FILE *fp,char *servicename,char *clientname,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6)
{
 char *where = WhereCluster(fp,address,uqhn,domain,ipv4,ipv6,0);
 ServiceCluster(fp,servicename);
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
 RoleCluster(fp,rolehub,instance,attr,"service relationship");

 // Attr hierarchy
 RoleCluster(fp,location,"where",where,"service relationship instance");
 RoleCluster(fp,service,role,servicename,"service relationship instance");

 snprintf(attr,CGN_BUFSIZE,"%s,%s",role,instancename); //(ftp server),(located at WHERE)
 RoleCluster(fp,instance,role,instancename,"service relationship instance");

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

char *ExceptionCluster(FILE *fp,char *origin,char *logmessage)

// 2016-08-13T15:00:01.906160+02:00 linux-e2vo /usr/sbin/cron[23039]: pam_unix(crond:session): session opened for user root by (uid=0)
// When                             where      who                    what                                                  (new who)
// Why = (lifecycle state change, exception, ...)

{
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
    when = TimeCluster(fp,whentime);
    }
 else
    {
    when = "regular check";
    }
 
 snprintf(event,CGN_BUFSIZE,"%s saw %s %s location %s %s cause %s",who,what,when,where,how,why);
 snprintf(attr,CGN_BUFSIZE,"%s,%s,%s,%s,%s",who,what,when,how,why);
 RoleCluster(fp,event,"event",attr,icontext);
 
 RoleCluster(fp,who,"who","",icontext);
 RoleCluster(fp,what,"what",what,icontext);
 RoleCluster(fp,how,"how",how,icontext);
 RoleCluster(fp,why,"why","",icontext);

 Gr(fp,what,a_caused_by,why,icontext);
 return event;
}

/************************************************************************************/

char *TimeCluster(FILE *fp,time_t time)
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
           RoleCluster(fp,hub[0],"when",attributes,ContextCluster(fp,"local clock time"));
           break;
       default:
           snprintf(attributes, CGN_BUFSIZE,"%s,%s,%s,%s,%s,%s,%s,%s",shift,dow,day,month,year,hour,min,quart);
           RoleCluster(fp,hub[1],"when GMT",attributes,ContextCluster(fp,"GMT clock time"));
           break;
       }
    
    RoleCluster(fp,shift,"time of day","work shift","time");
    RoleCluster(fp,dow,"weekday","","clock time");
    RoleCluster(fp,day,"day of month","","clock time");
    RoleCluster(fp,month,"month","","clock time");
    RoleCluster(fp,year,"year","","clock time");
    RoleCluster(fp,hour,"hour","","clock time");
    RoleCluster(fp,month,"minutes past the hour","minutes","clock time");       
    }

 static char retval[CGN_BUFSIZE];
 snprintf(retval, CGN_BUFSIZE,"%s,%s",hub[0],hub[1]);
 return retval;
}

/************************************************************************************/

char *WhereCluster(FILE *fp,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6, unsigned int portnumber)
{
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

  if (portnumber > 0)
     {
     snprintf(where,CGN_BUFSIZE,"host %s.%s IPv4 %s ipv6 %s at %s port %d",uqhn,domain,ipv4,ipv6,address,portnumber);
     snprintf(attr,CGN_BUFSIZE,"hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s,ip portnumber %d",uqhn,domain,ipv4,ipv6,address,portnumber);
     }
  else
     {
     snprintf(where,CGN_BUFSIZE,"host %s.%s IPv4 %s ipv6 %s at %s",uqhn,domain,ipv4,ipv6,address);
     snprintf(attr,CGN_BUFSIZE,"hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s",uqhn,domain,ipv4,ipv6,address);
     }
         
 RoleCluster(fp,where,"where",attr, "location");

 snprintf(attr,CGN_BUFSIZE,"hostname %s",uqhn);
 RoleCluster(fp,attr,"hostname",uqhn,"location");

 snprintf(attr,CGN_BUFSIZE,"domain %s",domain);
 RoleCluster(fp,attr,"dns domain name",domain,"location");
 
 snprintf(where,CGN_BUFSIZE,"ipv4 address %s",ipv4);
 RoleCluster(fp,where,"ipv4 address", ipv4,"location");

 snprintf(where,CGN_BUFSIZE,"ipv6 address %s",ipv6);
 RoleCluster(fp,where,"ipv6 address", ipv6,"location");

 if (portnumber > 0)
    {
    snprintf(where,CGN_BUFSIZE,"ip portnumber %d",portnumber);
    snprintf(attr,CGN_BUFSIZE,"%d",portnumber);
    RoleCluster(fp,where,"ip portnumber",attr,"location");
    }
 
 snprintf(where,CGN_BUFSIZE,"decription address %s",address);
 RoleCluster(fp,where,"description address",address,"location");
 
 Gr(fp,domain,a_contains,uqhn, ContextCluster(fp,"location"));
 Gr(fp,domain,a_contains,ipv4, "location");
 Gr(fp,domain,a_contains,ipv6, "location");
 Gr(fp,"description address",a_related_to,"street address", "location");

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
