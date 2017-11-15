/*****************************************************************************/
/*                                                                           */
/* File: scan_logs.c                                                         */
/*                                                                           */
/* Scan log files, machine learn, and flag anomalies                         */
/* This is all about approximation and reduction to countable invariants     */
/*                                                                           */
/*****************************************************************************/

// gcc -g scan_logs.c -o scan_logs -lhiredis -lm

#define _XOPEN_SOURCE 
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <hiredis/hiredis.h>

#define true 1
#define false 0
#define CGN_BUFSIZE 1024
#define INSIGNIFICANT 10
#define HIGH +1
#define LOW -1
#define HASHTABLESIZE 8197
#define DEBUG 0
#define Debug if (DEBUG) printf

/*****************************************************************************/
/* Add Item list Library from CFEngine */

#include "item.h"
#include "item.c"

/*****************************************************************************/
/* include some definitions from CFEngine monitor */

typedef struct
{
   double q;
   double expect;
   double var;
   double dq;
   double dq_ex;
   double dq_var;
} QPoint;

typedef struct
{
    char *name[HASHTABLESIZE];
  QPoint Q[HASHTABLESIZE];
}
    Averages;

typedef struct
{
   char *namespace;
   char *pod;
   char *container;
   char *appsysname;
}
    ClusterLocation;

/*****************************************************************************/
/* Global context                                                            */
/*****************************************************************************/

int TOTALMESSAGES = 0;
Averages METRICS;
char TIMEKEY[64];

redisContext *RC;
redisReply *RPLY;
char *REDIS_HOST = "127.0.0.1";
int REDIS_PORT = 6379;
struct timeval TIMEOUT = { 1, 500000 }; // 1.5 seconds

Item *APPL_CONTEXT = NULL;

/*****************************************************************************/

ClusterLocation ExtractLocation(char *name);
char *MatchDateTime(char *buffer,char *timekey, time_t *stamp);
void CheckKeyValue(char *timekey,ClusterLocation location,char *name,double value);
char *MatchURI(char *buffer,char *URI,char *timekey, ClusterLocation location);
char *MatchIPv4addr(char *buffer,char *addr,char *timekey, ClusterLocation location);
char *MatchIPv6addr(char *buffer,char *addr,char *timekey, ClusterLocation location);
int MatchJSON(char *buffer, FILE *fp, char *uri,char *timekey, ClusterLocation location);
void ExtractMessages(char *msg,char *timekey, ClusterLocation location);
void IncrementCounter(char *timekey,char *namekey, ClusterLocation location, int value);
char *TimeKey(struct tm tz);
char *MatchDatePosition(char *buffer);
unsigned int Hash(char* str, unsigned int tablelength);
void FlushContext(char *timekey,ClusterLocation location);
char *Canonify(char *name);
void ScanLog(char *name);
double WAverage(double old, double new);
void SetApplicationContext(char *key,int anomaly);
int TimeKeyChange(char *timekey);
void SaveInvariants(Item *list);
Item *LoadInvariants(void);
void DiffInvariants(Item **performance_syndrome,Item **security_syndrome,Item **invariants);

/*****************************************************************************/
/* BEGIN                                                                     */
/*****************************************************************************/

void main(int argc, char** argv)
{
 if (argc < 2)
    {
    printf("Syntax: scan <input file>\n");
    exit(1);
    }
 int i;

 RC = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, TIMEOUT);
 
 if (RC == NULL || RC->err)
    {
    if (RC)
       {
       printf("Connection error: %s\n", RC->errstr);
       redisFree(RC);
       }
    else
       {
       printf("Connection error: can't allocate redis context\n");
       }
    exit(1);
    }

 for (i = 1; argv[i] != NULL; i++)
    {
    printf("Processing %s\n",argv[i]);
    ScanLog(argv[i]);
    }

 redisFree(RC);

}

/*****************************************************************************/

void ScanLog(char *name)

{
 char buffer[4096],prefix[4096],timekey[4096],URI[4096] = {0};
 char addr4[4096] = {0},addr6[4096] = {0};
 FILE *fp;
 time_t stamp, last_t = 0, delta_t = 0, sum_delta = 0;
 int sum_count = 1;
 int lines = 0;

 if ((fp = fopen(name,"r")) == NULL)
    {
    return;
    }

 ClusterLocation location = ExtractLocation(name);
 
 while (!feof(fp))
    {
    memset(buffer,0,4096);
    fgets(buffer,4096,fp);

    if (strlen(buffer) == 0 || feof(fp))
       {
       break;
       }

    lines++;
    char *mesg = buffer;
    
    if (mesg = MatchDateTime(buffer,timekey,&stamp))
       {       
       while (!isalpha(*(mesg+1)))
          {
          mesg++;
          }
       }

    Debug("---------------------------------------------\n");
    Debug("LOG ENTRY: %s\n",buffer);
    
    if (TimeKeyChange(timekey))
       {
       strcpy(TIMEKEY,timekey);
       FlushContext(timekey,location);
       
       if (last_t > 0 && (stamp > last_t))
          {
          delta_t = stamp - last_t;
          IncrementCounter(timekey,"message interval",location,(int)delta_t);
          sum_delta += delta_t;
          }

       IncrementCounter(timekey,"lines per sample",location,sum_count);
       sum_count = 0;
       last_t = stamp;
       }

    sum_count++;
    
    if (MatchIPv4addr(buffer,addr4,timekey,location))
       {
       }
    
    if (MatchIPv6addr(buffer,addr6,timekey,location))
       {
       }

    if (MatchURI(buffer,URI,timekey,location))
       {
       }

    if (MatchJSON(buffer,fp,URI,timekey,location))
       {
       // diff file changes...and assume last URI is the filename
       continue;
       }
    
    ExtractMessages(mesg,timekey,location);
    }

 // Final flush at exit
 strcpy(TIMEKEY,timekey);
 FlushContext(timekey,location);

  // Measure average time jump in logging

 IncrementCounter(timekey,"log lines",location,lines);
 
 printf("\nSUMMARY of %d lines\n\n",lines);
 printf("Average interval between (%d) log bursts = %.2lf seconds\n",sum_count,(double)sum_delta/(double)sum_count);

 fclose(fp);
}

/*****************************************************************************/
/* Smart Probes                                                              */
/*****************************************************************************/

ClusterLocation ExtractLocation(char *pathname)
{
 ClusterLocation loc;
 char ns[CGN_BUFSIZE],nsp[CGN_BUFSIZE];
 char pod[CGN_BUFSIZE];
 char ctnr[CGN_BUFSIZE];
 char app[CGN_BUFSIZE];
 char *name;

 for (name = pathname+strlen(pathname); name > pathname && *name != '/'; name--)
    {
    }

 name++;

 ns[0] = '\0';
 nsp[0] = '\0';
 pod[0] = '\0';
 ctnr[0] = '\0';
 app[0] = '\0';
 
 strcpy(pod,"pod?");
 strcpy(ctnr,"ctnr?");
 strcpy(app,"app?");
        
 // Don't know if this simple approach will generalize .. works for now

 if (strncmp(name,"default",7) == 0)
    {
    strcpy(ns,"default");
    sscanf(name+8,"%[^.]",pod);
    }
 else // assume hyphenated for now
    {
    sscanf(name,"%[^-]",ns);
    sscanf(name+strlen(ns)+1,"%[^-.]",nsp);
    strcat(ns,"-");
    strcat(ns,nsp);
    sscanf(name+strlen(ns)+1,"%[^.]",pod);
    }
 
 loc.namespace = strdup(ns);
 loc.pod = strdup(pod);
 loc.container = strdup(ctnr);
 loc.appsysname = strdup(app);

 printf("name(%s,%s,%s,%s)\n",ns,pod,ctnr,app);
 return loc;
}

/*****************************************************************************/

int TimeKeyChange(char *timekey)
{
 return (strcmp(timekey,"NoTime") != 0) && (strcmp(timekey,TIMEKEY) != 0);
}

/*****************************************************************************/

char *MatchDateTime(char *buffer,char *timekey, time_t *stamp)
{
 // Should return pointer to after the date for later elimination
 
 struct tm tm;
 static char timestring[CGN_BUFSIZE]; // may capture whole line
 char *sp = buffer,*spt = timestring;
 int glog = false;
 char *ret;
 
 switch (*sp)
    {
    // “[IWEF]mmdd hh:mm:ss.uuuuuu threadid file:line] msg”   (FATAL, ERROR, WARNING, and INFO)

    case 'I':
    case 'W':
    case 'E':
    case 'F':
        glog = true;
        sp++;
        break;

    // [2017-10-23 20:27:46.373][11][warning][main] external/envoy/source/server/server.cc:332] starting main ...
    // [restful] 2017/11/01 02:18:24 log.go:33: [restful/swagger] available at https://10.184.147.252:6443/s...

    case '[':
        sp = MatchDatePosition(buffer);
        if (sp == NULL)
           {
           strncpy(timekey,"NoTime",64);
           *stamp = 0;
           return buffer;
           }

    // t=2017-10-03T17:52:44+0000 lvl=info msg="Config loaded from" logger=settings file=/etc/grafana/grafana.ini

    case 't':
        if (strncmp(sp,"t=",2) == 0)
           {
           sp += 2;
           }
        break;
    default:
        break;
    }
 
 *timestring = '\0';

 // copy timestring

 while (isalnum(*sp) || *sp == ':' || *sp == '-'  || *sp == ' '|| *sp == '/')
    {
    *spt++ = *sp++;
    *(sp-1) = ' ';
    if (*sp == '\0')
       {
       break;
       }
    }

 *spt = '\0';

 time_t now = 0;
 localtime_r(&now, &tm);

 if (glog)
    {
    // Format defines no year(!) so what to do? Set it to 9999
    strptime(timestring,"%m%d %H:%M:%S.", &tm);
    tm.tm_year = 9999-1900;
    Debug("Extracted time format (glog incomplete) : %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    return sp-1;
    }

 // new syslog format 2016-08-13T12:00:01.512298+02:00

 if (timestring[11] == 'T')
    {
    timestring[11] == ' ';
    }
 
 // Simple new format

 ret = strptime(timestring,"%Y-%m-%d %H:%M:%S.", &tm);

 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 1: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return sp-1;
    }

 ret = strptime(timestring,"%Y/%m/%d %H:%M:%S.", &tm);

 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 2: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return sp-1;
    }

  // [httpd] 192.168.54.14 - root [03/Oct/2017:17:57:05 +0000] "POST /write?con

 ret = strptime(timestring,"%d/%B/%Y:%H:%M:%S ", &tm);
 
 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 3: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return sp-1;
    }

 strncpy(timekey,"NoTime",64);
 *stamp = 0;
 return timestring;
}

/*****************************************************************************/

void FlushContext(char *timekey,ClusterLocation location)
{
 int i;

 Debug("## %s ###############################################\n",TIMEKEY);

 for (i = 0; i < HASHTABLESIZE; i++)
    {
    if (METRICS.name[i])
       {
       Debug("%6.0lf %s\n",METRICS.name[i],METRICS.Q[i].q);

       // Store in REDIS
       // First store the long-term weekly pattern

       CheckKeyValue(TIMEKEY,location,METRICS.name[i],METRICS.Q[i].q);

       // Then the recent aggregate

       CheckKeyValue("recent",location,METRICS.name[i],METRICS.Q[i].q);
       
       free(METRICS.name[i]);
       METRICS.name[i] = NULL;
       METRICS.Q[i].q = 0;
       }
    }

 IncrementCounter(timekey,"aggregate messages",location,TOTALMESSAGES);
 TOTALMESSAGES = 0;

 Item *performance_syndrome = NULL, *security_syndrome = NULL, *invariants = NULL, *ip;
 
 DiffInvariants(&performance_syndrome,&security_syndrome,&invariants);

 for (ip = performance_syndrome; ip != NULL; ip=ip->next)
    {
    printf("ANOMALOUS EVENT: %s in context <CONTEXT NAME>\n", ip->name);
    }

// <CONTEXT NAME> = slowly varying TIME, LOCATION, background set
 
  for (ip = invariants; ip != NULL; ip=ip->next)
    {
    // Edge event had background context
    printf("BACKGROUND CONTEXT: %s was part of <CONTEXT NAME>\n", ip->name);
    }

// Causal epochs - changes - aggregated by context clusters. So keep a slowly varying context approximation too?
  // Or just keep classes with Delta t >> 1
  
 DeleteItemList(APPL_CONTEXT);
 DeleteItemList(performance_syndrome);
 DeleteItemList(security_syndrome);
 DeleteItemList(invariants);
 APPL_CONTEXT = NULL;
}

/*********************************************************************/

void DiffInvariants(Item **performance_syndrome,Item **security_syndrome,Item **invariants)
{
 Item *current_state = NULL;
 Item *prev_state = LoadInvariants(); // Assume pre-sorted, no tampering
 Item *ip;
 
 SaveInvariants(APPL_CONTEXT);

 // Now separate the lists into (invariant/intesect + delta/NOT-intersect) sets

 Item *ip1,*ip2 = prev_state;
 
 for (ip1 = APPL_CONTEXT; ip1 != NULL; ip1=ip1->next)
    {
    if (IsItemIn(prev_state,ip1->name))
       {
       PrependItem(invariants,ip1->name,NULL);
       DeleteItemLiteral(&prev_state,ip1->name);
       }
    else
       {
       char name[CF_BUFSIZE];
       snprintf(name,CF_BUFSIZE,"+%s",ip1->name);
       
       if (ip1 && (strstr(ip1->name,"access") == 0))
          {
          PrependItem(security_syndrome,name,NULL);
          }
       else
          {
          PrependItem(performance_syndrome,name,NULL);
          }
       }
    }
 
 for (ip2 = prev_state; ip2 != NULL; ip2=ip2->next)
    {
    char name[CF_BUFSIZE];
    snprintf(name,CF_BUFSIZE,"-%s",ip2->name);

    if (ip2 && (strstr(ip2->name,"access") == 0))
       {
       PrependItem(security_syndrome,name,NULL);
       }
    else
       {
       PrependItem(performance_syndrome,name,NULL);
       }
    }
 
 DeleteItemList(prev_state);
}

/*****************************************************************************/

void CheckKeyValue(char *timekey,ClusterLocation location,char *name,double q)

{
 char key[1024];
 char value[1024];
 double oldq = 0,oldav = 0,oldvar = 0,newav,newvar,var;
 int anomaly = 0;

 snprintf(key,1024,"(%s,%s,%s,%s,%s)",location.namespace,location.pod,location.appsysname,timekey,name);

 RPLY = redisCommand(RC,"GET %s",key);
 Debug("%s: %s\n",key,RPLY->str);

 if (RPLY->str)
    {
    sscanf(RPLY->str,"(%lf,%lf,%lf)",&oldq,&oldav,&oldvar);
    }
 else
    {
    anomaly = HIGH;
    }
 
 freeReplyObject(RPLY);

 newav = WAverage(oldav,q);
 var = (q-newav)*(q-newav);
 newvar = WAverage(oldvar,var);

 snprintf(value,1024,"(%.2lf,%.2lf,%.2lf)",q,newav,newvar);
 
 // Expire data after 30 days without update
 RPLY = redisCommand(RC,"SET %s %s ex %d",key,value,3600*24*30);
 Debug("SET: %s\n", RPLY->str);
 freeReplyObject(RPLY);

 double bar = 3.0 * sqrt(newvar);
 
 if (q > newav + bar)
    {
    anomaly = HIGH;
    }
 else if (q < newav - bar)
    {
    anomaly = LOW;
    }
 
 // This process is only run over aggregate application/pod
 
 SetApplicationContext(key,anomaly);

}

/*****************************************************************************/

void SetApplicationContext(char *key,int anomaly)
{
 char anomaly_name[256];

 switch (anomaly)
    {
    case HIGH:
        snprintf(anomaly_name,256,"%s_high",key);
        break;
    case LOW:
        snprintf(anomaly_name,256,"%s_low",key);
        break;
    default:
        return;
    }

 // Import minimal CFEngine ItemList library..
 
 PrependItem(&APPL_CONTEXT,anomaly_name,NULL);
}

/*****************************************************************************/

char *MatchIPv4addr(char *buffer,char *addr,char *timekey, ClusterLocation location)
{
 char *regex4 = "[0-9][0-9]*[0-9]*[.][0-9][0-9]*[0-9]*[.][0-9][0-9]*[0-9]*[.][0-9][0-9]*[0-9]*[:0-9]*";
 regex_t    preg;
 int        rc;
 size_t     nmatch = 2;
 regmatch_t pmatch[2];
 
 if (strlen(buffer) < 2)
    {
    return NULL;
    }
 
 if ((rc = regcomp(&preg,regex4, 0)) == 0)
    {
    if ((rc = regexec(&preg, buffer, nmatch, pmatch,0)) == 0)
       {
       strncpy(addr,buffer+pmatch[0].rm_so,pmatch[0].rm_eo - pmatch[0].rm_so);
       regfree(&preg);
       
       Debug("matched ipv4 (%s)\n",addr);
       IncrementCounter(timekey,addr,location,1);
       return addr;       
       }
    }
}

/*****************************************************************************/

char *MatchIPv6addr(char *buffer,char *addr,char *timekey, ClusterLocation location)
{
 char *regex6 = "[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][:][a-fA-F0-9:]*[:][a-fA-F0-9:]*";
 regex_t    preg;
 int        rc;
 size_t     nmatch = 2;
 regmatch_t pmatch[2];
 
 if (strlen(buffer) < 2)
    {
    return NULL;
    }

 if ((rc = regcomp(&preg,regex6, 0)) == 0)
    {
    if ((rc = regexec(&preg, buffer, nmatch, pmatch,0)) == 0)
       {
       strncpy(addr,buffer+pmatch[0].rm_so,pmatch[0].rm_eo - pmatch[0].rm_so);
       regfree(&preg);

       if (strlen(addr) > 13) // Don't muddle a date string
          {
          Debug("matched ipv6 (%s)\n",addr);
          IncrementCounter(timekey,addr,location,1);
          return addr;
          }
       }
    }

 return NULL;
}

/*****************************************************************************/

int MatchJSON(char *buffer, FILE *fp, char *uri,char *timekey, ClusterLocation location)
{
 char *sp, *c;
 char jsonbuffer[1024*1024];
 int level = 0;
 int maxlines = 1024;

 memset(jsonbuffer,0,1024*1024);
 
 if (sp = strchr(buffer,'{'))
    {
    Debug("Emebedded JSON (relating to %s ?)\n",uri);

    strcpy(jsonbuffer,sp);
    
    for (c = buffer; *c != '\0'; c++)
       {
       switch (*c)
          {
          case '{':
              level++;
              continue;
          case '}':
              level--;
              continue;
          default:
              continue;
          }
       }

    while (level > 0)
       {
       buffer[0] = '\0';
       fgets(buffer,4096,fp);
       strcat(jsonbuffer,buffer);
       if (feof(fp))
          {
          break;
          }
       
       for (c = buffer; *c != '\0'; c++)
          {
          switch (*c)
             {
             case '{':
                 level++;
                 continue;
             case '}':
                 level--;
                 continue;
             default:
                 continue;
             }
          }
       }
    
    for (c = jsonbuffer; *c != '\0'; c++)
       {
       switch (*c)
          {
          case '{':
              level++;
              continue;
          case '}':
              level--;
              if (level == 0)
                 {
                 *c = '\0';
                 break;
                 }
              continue;
          default:
              continue;
          }
       }
    
    Debug("CONCAT name:((( %s )))\n\n",jsonbuffer);


    if (strlen(uri) == 0)
       {
       // no specific filename so random string
       return false;
       }
    
    FILE *fin,*fout;
    char *jsonfile_old, oldname[256], newname[256];

    snprintf(oldname,1024,"/tmp/json-%s.old",uri);
    snprintf(newname,1024,"/tmp/json-%s.new",uri);

    Canonify(oldname+strlen("/tmp/"));
    Canonify(newname+strlen("/tmp/"));
    
    rename(newname,oldname);

    if ((fout = fopen(newname,"w")) != NULL)
       {
       fwrite(jsonbuffer,strlen(jsonbuffer),1,fp);
       fclose(fout);
       }

    // ------ diff with the old ----

    struct stat statbuf;
    
    if (stat(oldname, &statbuf) != 0)
       {
       return false; // Need to bootstrap
       }
    
    jsonfile_old = (char*)malloc(statbuf.st_size);
    
    if (jsonfile_old == NULL)
       {
       fprintf(stderr, "Memory error: unable to allocate file\n");
       return 1;
       }
    
    if ((fin = fopen(oldname, "r")) == NULL)
       {
       fprintf(stderr, "Unable to open previous json\n");
       free(jsonfile_old);
       }
    else 
       {
       if (fread(jsonfile_old,statbuf.st_size, 1, fin) != 1)
          {
          char *sp1,*sp2;
          
          for (sp1 = jsonbuffer, sp2=jsonfile_old; (*sp1 != '\0') && (*sp2 != '\0'); sp1++,sp2++)
             {
             if (*sp1 != *sp2)
                {
                Debug("CHANGE in embedded JSON (%s) from (%.64s) to (%.64)\n",uri,sp1,sp2);
                snprintf(newname,256,"json file change in %s",uri);
                IncrementCounter(timekey,newname,location,1);
                fclose(fin);
                free(jsonfile_old);
                return false;
                }
          }
          
          free(jsonfile_old);
          }

       Debug("NO CHANGE in embedded JSON (%s)\n",uri);
       }
    
    fclose(fin);      
    return true;
    }

 return false;
}

/*****************************************************************************/

char *MatchURI(char *buffer,char *URI,char *timekey, ClusterLocation location)
{
 char *sp, *spt, *iterator = buffer;
 char c;

 for (iterator = buffer; iterator < buffer + strlen(buffer); iterator = sp)
    {
    spt = URI;

    for (sp = iterator; *sp != '/'; sp++)
       {
       if (*sp == '\0')
          {
          return NULL;
          }
       }

    *URI = '\0';

    if (sp > iterator)
       {
       // roll baclk to ws in case doesn't start with /
       while(*sp == '/' || *sp == '_' || *sp == '-' || *sp == '.' || !isspace(*sp) && !ispunct(*sp))
          {
          sp--;
          }
       }

    // copy the path in URI buffer
    
    do
       {
       *spt++ = *++sp;
       c = *sp;
       *sp = ' ';
       }
    while(c == '/' || c == '_' || c == '-' || c == '.' || !isspace(c) && !ispunct(c));
    
    *(spt-1) = '\0';
    
    Debug("URI (%s)\n",URI);
    char uri[CGN_BUFSIZE];
    snprintf(uri,CGN_BUFSIZE,"uri:%s",URI);
    IncrementCounter(timekey,uri,location,1);
    }

 return sp;
}

/*****************************************************************************/

void ExtractMessages(char *msg,char *timekey, ClusterLocation location)
{
 char qstring[1024];
 char stripped[512] = {0};
 char *sp = msg, *spt = stripped, *spq = qstring;

 if (msg == NULL)
    {
    return;
    }
 
 *stripped = '\0';

 while (*sp != '\0')
    {
    if (isspace(*sp))
       {
       while (isspace(*sp))
          {
          sp++;
          }
       
       *spt++ = ' ';       
       }

    if (*sp == '\"')
       {
       sp++;
       while ((*sp != '\"') && (*sp != '\0'))
          {
          *spq++ = *sp++;
          }

       *spq = '\0';

       Debug("Extracted string \"%s\"\n",qstring);
       IncrementCounter(timekey,qstring,location,1);

       spq = qstring;
       memset(qstring,0,1024);
       sp++;
       }
    else
       {
       if (isalpha(*sp)|| *sp == '.' || *sp == ':') // strip numbers and junk
          {
          *spt++ = *sp;
          }
       sp++;
       if (spt > stripped + 510)
          {
          break;
          }
       }
    }

 *spt = '\0';
 
 Debug("Remaining message: %s\n",stripped);
 IncrementCounter(timekey,stripped,location,1);
}

/************************************************************************************/

char *MatchDatePosition(char *buffer)
{
 char *regex = "[0-2][0-9]:[0-5][0-9]:[0-5][0-9]";
 regex_t    preg;
 int        rc;
 size_t     nmatch = 2;
 regmatch_t pmatch[2];
 char *sp;
 
 if ((rc = regcomp(&preg,regex, 0)) == 0)
    {
    if ((rc = regexec(&preg, buffer, nmatch, pmatch,0)) == 0)
       {
       regfree(&preg);

       for (sp = buffer+pmatch[0].rm_so; !(ispunct(*sp) || *sp == '[') || *sp == '/' || *sp == ':'|| *sp == '-'; sp--)
          {
          }

       return sp+1;
       }
    }

 return NULL;
}

/************************************************************************************/
/* Generic lib                                                                      */
/************************************************************************************/

char *TimeKey(struct tm tz)
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

 if (tz.tm_year <= 70)
    {
    return "No timestamp given";
    }
 
/* Lifecycle */
 
 snprintf(lifecycle, CGN_BUFSIZE, "Lcycle_%d", ((tz.tm_year + 1900) % 3));
    
/* Year */
 
 snprintf(year, CGN_BUFSIZE, "Yr%04d", tz.tm_year + 1900);
 
/* Month */
 
 snprintf(month, CGN_BUFSIZE, "%s", GR_MONTH_TEXT[tz.tm_mon]);
 
/* Day of week */
 
/* Monday  is 1 in tm_wday, 0 in GR_DAY_TEXT
   Sunday  is 0 in tm_wday, 6 in GR_DAY_TEXT */
 day_text_index = (tz.tm_wday + 6) % 7;
 snprintf(dow, CGN_BUFSIZE, "%s", GR_DAY_TEXT[day_text_index]);
 
/* Day */
 
 snprintf(day, CGN_BUFSIZE, "Day%d", tz.tm_mday);
 
 
/* Shift */
 
 snprintf(shift, CGN_BUFSIZE, "%s", GR_SHIFT_TEXT[tz.tm_hour / 6]);
 
 
/* Hour */
 
 snprintf(hour, CGN_BUFSIZE, "Hr%02d", tz.tm_hour);
 
//    snprintf(buf, CGN_BUFSIZE, "%sHr%d", tz_prefix[tz], tz.tm_hour);
 
 
/* Quarter */
 
 quarter = tz.tm_min / 15 + 1;
 
 snprintf(quart, CGN_BUFSIZE, "Q%d", quarter);
 
/* Minute */
 
 snprintf(min, CGN_BUFSIZE, "Min%02d", tz.tm_min);
 
 
 interval_start = (tz.tm_min / 5) * 5;
 interval_end = (interval_start + 5) % 60;
 
 snprintf(mins, CGN_BUFSIZE, "Min%02d_%02d", interval_start, interval_end);
 
 // Summary

 char hub[CGN_BUFSIZE];
 static char timekey[CGN_BUFSIZE];

 snprintf(hub, CGN_BUFSIZE,"on %s %s %s %s %s at %s %s %s",shift,dow,day,month,year,hour,mins,quart);    
 snprintf(timekey,CGN_BUFSIZE,"%s:%s:%s",dow,hour,mins);           

 Debug("When description: %s\n", hub);
 return timekey;
}

/************************************************************************************/

void IncrementCounter(char *timekey,char *namekey, ClusterLocation location, int value)
{

 // When we are dealing with aggregate, centralized logging, we may need to retain origin context
 // which shouldn't have been propagated in the first place

 // We want quickly to turn the namekey into an easily recognizable canonical invariant,
 // shorter than 256 characters, and document it in the semantic graph

 char canon[256];
 char *sp = namekey,*spt = canon;
 char last = 'x';

 *spt = '\0';
 
 // Strip leading junk
 while (!isalpha(*sp))
    {
    sp++;
    }

 // Now compress name gently into an invariant form
 
 while ((*sp != '\0') && (spt < canon+255))
    {
    // No commas in canonical name, as we use this as a key/tuple separator
    if (*sp == ',')
       {
       sp++;
       }
    
    if (!(isalpha(*sp) || ispunct(*sp) || *sp == ' '))
       {
       sp++;
       continue;
       }

    if ((*sp != last) || isalpha(*sp))
       {
       last = *spt++ = *sp;
       }

    sp++;
    }

 *spt = '\0';

 // This is a temporary bucket count for current timekey - and aggregate over past few (LDT)

 if (strlen(canon) < INSIGNIFICANT)
    {
    Debug("Message \"%s\" too short to count ..\n", canon);
    return;
    }

 // The keyname (location/name,timekey, canonized metric name)
 
 unsigned int hash = Hash(canon,HASHTABLESIZE);

 // We want to store by timekey and aggregate (timekey->0)

 if (METRICS.name[hash] == NULL)
    {
    METRICS.name[hash] = strdup(canon);
    METRICS.Q[hash].q = value;
    }
 else
    {
    if (strncmp(canon,METRICS.name[hash],4) != 0)
       {
       printf("COLLISION at slot %d!!!! \n - %s\n - %s\n",hash,canon,METRICS.name[hash]);
       }
    
    METRICS.Q[hash].q += value;
    }

 TOTALMESSAGES++;
 
 Debug("Increment (%s,<%s,%s,%s>,%s) +=%d\n",timekey,location.namespace,location.pod,location.appsysname,canon,value);    
}

/************************************************************************************/

unsigned int Hash(char *str, unsigned int tablelength)
{
 unsigned long hash = 15381;
 int c;
 
 while (c = *str++)
    {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
 
 return (unsigned int) hash % tablelength;
}

/************************************************************************************/

char *Canonify(char *name)
{
 char *sp;

 for (sp = name; *sp != '\0'; sp++)
    {
    if (!isalnum(*sp))
       {
       *sp = '_';
       }
    }
}

/************************************************************************************/

double WAverage(double old, double new)
{
 double forget_rate = 0.55;

 return old * forget_rate + (1.0-forget_rate) * new;
}

/*********************************************************************/
/* Import fron Cellibrium                                            */
/*********************************************************************/

#define INVFILE "/tmp/cellibrium_invs"
#define INVFILE_NEW "/tmp/cellibrium_new"
#define INVFILE_OLD "/tmp/cellibrium_old"

/*********************************************************************/
    
void SaveInvariants(Item *list)
{
 FILE *fp;
 Item *ip;
 
 rename(INVFILE, INVFILE_OLD);

 if ((fp = fopen(INVFILE_NEW, "w")) == NULL)
    {
    return;
    }

 for (ip = list; ip != NULL; ip=ip->next)
    {
    fprintf(fp, "%s\n", ip->name);
    }

fclose(fp);
rename(INVFILE_NEW, INVFILE);
}

/*********************************************************************/

Item *LoadInvariants()
{
 FILE *fp;
 Item *list = NULL;
 char class[CF_BUFSIZE];

 if ((fp = fopen(INVFILE, "r")) == NULL)
    {
    return NULL;
    }
 
 while(!feof(fp))
    {
    class[0] = '\0';
    fscanf(fp,"%s",class);
    if (class[0] != '\0')
       {
       AppendItem(&list,class,NULL);
       }
    }
 
 fclose(fp);
 return list;
}

