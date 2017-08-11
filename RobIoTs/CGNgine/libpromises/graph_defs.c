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

/********************************************************************************/

const char *const GR_DAY_TEXT[] =
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

const char *const GR_MONTH_TEXT[] =
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

const char *const GR_SHIFT_TEXT[] =
{
    "Night",
    "Morning",
    "Afternoon",
    "Evening",
    NULL
};

/******************************************************************************/

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

char *Clue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext)
{
 static char event[CGN_BUFSIZE];
 char attr[CGN_BUFSIZE];
 char *when;
 
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
    RoleCluster(fp,month,"year","","clock time");
    RoleCluster(fp,hour,"hour","","clock time");
    RoleCluster(fp,month,"minutes past the hour","minutes","clock time");       
    }

 static char retval[CGN_BUFSIZE];
 snprintf(retval, CGN_BUFSIZE,"%s,%s",hub[0],hub[1]);
 return retval;
}

/************************************************************************************/

char *WhereCluster(FILE *fp,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6)
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

 snprintf(where,CGN_BUFSIZE,"host %s.%s IPv4 %s ipv6 %s at %s",uqhn,domain,ipv4,ipv6,address);
 snprintf(attr,CGN_BUFSIZE,"hostname %s,domain %s,IPv4 %s,IPv6 %s,address %s",uqhn,domain,ipv4,ipv6,address);
 RoleCluster(fp,where,"where",attr, "location");

 snprintf(attr,CGN_BUFSIZE,"hostname %s",uqhn);
 RoleCluster(fp,attr,"hostname",uqhn,"location");

 snprintf(attr,CGN_BUFSIZE,"domain %s",domain);
 RoleCluster(fp,attr,"dns domain name",domain,"location");
 
 snprintf(where,CGN_BUFSIZE,"ipv4 address %s",ipv4);
 RoleCluster(fp,where,"ipv4 address", ipv4,"location");

 snprintf(where,CGN_BUFSIZE,"ipv6 address %s",ipv6);
 RoleCluster(fp,where,"ipv6 address", ipv6,"location");

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
           *sp = '-';
           break;
       default:
           break;
       }
    }
 return str;
}


#endif
