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
#define INSIGNIFICANT 12
#define HIGH +1
#define LOW -1
#define APPEARED 2
#define NONE 0
#define HASHTABLESIZE 8197
#define DEBUG 0
#define Debug if (DEBUG) printf
#define Print if (PRINT && !GRAPHS) printf
#define Summary if (!PRINT && !GRAPHS) printf

/*****************************************************************************/
/* Add Item list Library from CFEngine */

#include "item.h"
#include "item.c"
#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

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
    ClusterContext;

/*****************************************************************************/
/* Global context                                                            */
/*****************************************************************************/

int GRAPHS = false;
int PRINT = false;
int SUMMARY = true;

int TOTALMESSAGES = 0;
int ANOMALY = 0;
int SECURITY = 0;
int CONTEXT = 0;

Averages METRICS;
char TIMEKEY[64];

redisContext *RC;
redisReply *RPLY;
char *REDIS_HOST = "127.0.0.1";
int REDIS_PORT = 6379;
struct timeval TIMEOUT = { 1, 500000 }; // 1.5 seconds

Item *APPL_CONTEXT = NULL;

char *URITABLE[HASHTABLESIZE];
char *IPTABLE[HASHTABLESIZE];
char *FRAGMENTTABLE[HASHTABLESIZE];

/*****************************************************************************/

char *Trunc(char *s);
ClusterContext ExtractContext(char *name);
char *MatchDateTime(char *buffer,char *timekey, time_t *stamp, char **start, char **end);
void CheckKeyValue(char *timekey,time_t stamp, ClusterContext context,char *name,double value);
char *MatchURI(char *buffer,char *URI,char *timekey, ClusterContext context);
char *MatchIPv4addr(char *buffer,char *addr, char **start, char **end);
char *MatchIPv6addr(char *buffer,char *addr, char **start, char **end);
int MatchJSON(char *buffer, FILE *fp, char *uri,char *timekey, ClusterContext context);
void ExtractMessages(char *msg,char *timekey, ClusterContext context,char *addr4,char *addr6,char *uri);
void IncrementCounter(char *timekey,char *namekey, ClusterContext context, int value);
char *TimeKey(struct tm tz);
char *MatchDatePosition(char *buffer);
unsigned int Hash(char* str, unsigned int tablelength);
void FlushContext(char *timekey,time_t stamp,ClusterContext context);
char *Canonify(char *name);
void ScanLog(char *name);
double WAverage(double old, double new);
void SetApplicationContext(char *key,int anomaly);
int TimeKeyChange(char *timekey);
void SaveInvariants(Item *list);
Item *LoadInvariants(void);
void DiffInvariants(Item **performance_syndrome,Item **security_syndrome,Item **invariants);
int StupidString(char *s);
void Erase(char *start, char *end);
void GraphIPConcept(char *concept);
void GraphURIConcept(char *concept);
int GraphFragmentConcept(char *concept);
void GraphConcepts(ClusterContext context);

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
    if (strcmp(argv[i],"graph") == 0)
       {
       GRAPHS = true;
       continue;
       }
    
    if (strcmp(argv[i],"print") == 0)
       {
       PRINT = true;
       continue;
       }

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
 double sum_count = 1;
 double lines = 0;

 if ((fp = fopen(name,"r")) == NULL)
    {
    return;
    }

 ClusterContext context = ExtractContext(name);

 TOTALMESSAGES = ANOMALY = SECURITY = CONTEXT = 0;

 while (!feof(fp))
    {
    memset(buffer,0,4096);
    memset(addr4,0,4096);
    memset(addr6,0,4096);
    memset(URI,0,4096);
    fgets(buffer,4096,fp);

    if (strlen(buffer) == 0 || feof(fp))
       {
       break;
       }

    lines++;
    char *mesg = buffer;
    
    Debug("---------------------------------------------\n");
    Debug("LOG ENTRY: %s\n",buffer);

    // Use a process of elimination to classify bits
    // Match IP addresses first, because they are easier to spot
    
    char *start, *end;
    
    MatchDateTime(buffer,timekey,&stamp,&start,&end);

    // Erase date stamp from buffer

    Erase(start,end);
    
    // Next dates, if they exist at all(!)

    if (TimeKeyChange(timekey))
       {
       strcpy(TIMEKEY,timekey);
       FlushContext(timekey,stamp,context);
       
       if (last_t > 0 && (stamp > last_t))
          {
          delta_t = stamp - last_t;
          IncrementCounter(timekey,"Message-interval *? log aggrEg+te",context,(int)delta_t);
          sum_delta += delta_t;

          // Max/min bursts
          }

       IncrementCounter(timekey,"lines of aggregate log messages per time interval sample",context,sum_count);
       sum_count = 0;
       last_t = stamp;
       }

    sum_count++;

    if (MatchIPv4addr(buffer,addr4,&start,&end))
       {
       if (strlen(addr4) > 8)
          {
          IncrementCounter(timekey,IPv4(addr4),context,1);
          GraphIPConcept(IPv4(addr4));
          }
       Erase(start,end);
       }

    if (MatchIPv6addr(buffer,addr6,&start,&end))
       {
       if (strlen(addr6) > 8)
          {
          IncrementCounter(timekey,IPv6(addr6),context,1);
          GraphIPConcept(IPv6(addr6));
          }
       Erase(start,end);
       }
    
    MatchURI(buffer,URI,timekey,context);    

    // Dumped JSON - is it significant?
    
    if (MatchJSON(buffer,fp,URI,timekey,context))
       {
//              deployment depends on, expresses "role ipv4" when/where?
       // diff file changes...and assume last URI is the filename
       continue;
       }

    // What remains, however improbable, must be noise
    ExtractMessages(buffer,timekey,context,addr4,addr6,URI);
    }

 // Final flush at exit
 strcpy(TIMEKEY,timekey);
 FlushContext(timekey,stamp,context);

  // Measure average time jump in logging

 IncrementCounter(timekey,"log lines",context,lines);

 GraphConcepts(context);

 double avdt = (double)sum_delta/sum_count;
 double avrate = avdt ? lines/avdt : 0;

 double total_per_lines = ((double)TOTALMESSAGES)/lines;

 Summary("\n# %s\n# (lines=%.0lf,buckets=%.0lf,<dt>=%.2lfs,extracted=%d,anom=%d,sec=%d,ctx=%d,line/sec=%.2lf)\n",Trunc(name),lines,sum_count,avdt,TOTALMESSAGES,ANOMALY,SECURITY,CONTEXT,avrate);
 fclose(fp);
}

/*****************************************************************************/
/* Smart Probes                                                              */
/*****************************************************************************/

ClusterContext ExtractContext(char *pathname)
{
 ClusterContext loc;
 char ns[CGN_BUFSIZE],nsp[CGN_BUFSIZE];
 char pod[CGN_BUFSIZE];
 char ctnr[CGN_BUFSIZE];
 char app[CGN_BUFSIZE];
 char *name;

 char *context = "kubernetes cluster log output";
 
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

 char namespace[1024],deployment[1024];
 snprintf(namespace,1024,"kubernetes namespace %s",ns);
 snprintf(deployment,1024,"kubernetes deployment %s",pod);

 if (GRAPHS)
    {
    Gr(stdout,namespace,a_contains,deployment,context);
    }
 
 return loc;
}

/*****************************************************************************/

int TimeKeyChange(char *timekey)
{
 return (strcmp(timekey,"NoTime") != 0) && (strcmp(timekey,TIMEKEY) != 0);
}

/*****************************************************************************/

char *MatchDateTime(char *buffer,char *timekey, time_t *stamp, char **start, char **end)
{
 // Should return pointer to after the date for later elimination
 
 struct tm tm;
 static char timestring[CGN_BUFSIZE]; // may capture whole line
 char *sp = buffer,*spt = timestring;
 int glog = false;

 switch (*sp)
    {
    // “[IWEF]mmdd hh:mm:ss.uuuuuu threadid file:line] msg”   (FATAL, ERROR, WARNING, and INFO)

    case 'I':
    case 'W':
    case 'E':
    case 'F':
        glog = true;
        *start = sp;
        sp++;
        break;

    // [2017-10-23 20:27:46.373][11][warning][main] external/envoy/source/server/server.cc:332] starting main ...
    // [restful] 2017/11/01 02:18:24 log.go:33: [restful/swagger] available at https://10.184.147.252:6443/s...

    case '[':
        *start = sp = MatchDatePosition(buffer);
        if (sp == NULL)
           {
           strncpy(timekey,"NoTime",64);
           *stamp = 0;
           *end = NULL;
           return buffer;
           }

    // t=2017-10-03T17:52:44+0000 lvl=info msg="Config loaded from" logger=settings file=/etc/grafana/grafana.ini

    case 't':
        if (strncmp(sp,"t=",2) == 0)
           {
           *start = sp;
           sp += 2;
           }

        if (strncmp(sp,"time=\"",6) == 0)
           {
           *start = sp;
           sp += 6;
           }

        break;
    default:
        *start = sp;
        break;
    }
 
 *timestring = '\0';

 // copy timestring

 while (isalnum(*sp) || *sp == ':' || *sp == '-'  || *sp == ' '|| *sp == '/')
    {
    *spt++ = *sp++;
    //*(sp-1) = ' ';
    if (*sp == '\0')
       {
       break;
       }
    }

 *spt = '\0';
 *end = sp;
 
 time_t now = 0;
 localtime_r(&now, &tm);

 if (glog)
    {
    // Format defines no year(!) so what to do? Set it to 9999
    strptime(timestring,"%m%d %H:%M:%S.", &tm);
    tm.tm_year = 9999-1900;
    Debug("Extracted time format (glog incomplete) : %s\n", TimeKey(tm));
    *stamp = mktime(&tm);
    strncpy(timekey,TimeKey(tm),64);
    return sp-1;
    }

 // new syslog format 2016-08-13T12:00:01.512298+02:00

 if (timestring[11] == 'T')
    {
    timestring[11] == ' ';
    }
 
 // Simple new format

 strptime(timestring,"%Y-%m-%d %H:%M:%S.", &tm);

 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 1: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return timestring;
    }

 strptime(timestring,"%Y/%m/%d %H:%M:%S.", &tm);

 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 2: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return timestring;
    }

  // [httpd] 192.168.54.14 - root [03/Oct/2017:17:57:05 +0000] "POST /write?con

 strptime(timestring,"%d/%B/%Y:%H:%M:%S ", &tm);
 
 if (tm.tm_year > 70)
    {
    Debug("Extracted time format 3: %s\n", TimeKey(tm));
    strncpy(timekey,TimeKey(tm),64);
    *stamp = mktime(&tm);
    return timestring;
    }

 strncpy(timekey,"NoTime",64);
 *stamp = 0;
 *start = *end = NULL;
 return NULL;
}

/*****************************************************************************/

void FlushContext(char *timekey,time_t tstamp,ClusterContext context)
{
 int i;

 Debug("## %s ###############################################\n",TIMEKEY);

 for (i = 0; i < HASHTABLESIZE; i++)
    {
    if (METRICS.name[i])
       {
       //printf("* %6.0lf %s\n",METRICS.Q[i].q,METRICS.name[i]);

       // Store in REDIS
       // First store the long-term weekly pattern

       //CheckKeyValue(TIMEKEY,tstamp,context,METRICS.name[i],METRICS.Q[i].q);

       // Then the recent aggregate

       CheckKeyValue("recent",tstamp,context,METRICS.name[i],METRICS.Q[i].q);
       
       free(METRICS.name[i]);
       METRICS.name[i] = NULL;
       METRICS.Q[i].q = 0;
       }
    }

 IncrementCounter(timekey,"aggregate messages",context,TOTALMESSAGES);

 Item *performance_syndrome = NULL, *security_syndrome = NULL, *invariants = NULL, *ip;

 // for (ip = APPL_CONTEXT; ip != NULL; ip=ip->next)
 //   {
 //   Print("CONTEXT: %s (%d)\n", ip->name,ip->counter);
 //   }

 DiffInvariants(&performance_syndrome,&security_syndrome,&invariants);

 
 for (ip = performance_syndrome; ip != NULL; ip=ip->next)
    {
    Print(" ANOMALOUS EVENT: %s\n", ip->name);
    ANOMALY++;
    }

 for (ip = security_syndrome; ip != NULL; ip=ip->next)
    {
    Print(" SECURITY EVENT: %s\n", ip->name);
    SECURITY++;
    }

  // <CONTEXT NAME> = slowly varying TIME, CONTEXT, background set
 
  for (ip = invariants; ip != NULL; ip=ip->next)
    {
    // Edge event had background context
    Debug(" BACKGROUND CONTEXT: %s\n", ip->name);
    CONTEXT++;
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
       
       if (ip1 && strstr(ip1->name,"access"))
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

    if (ip2 && strstr(ip2->name,"access"))
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

void CheckKeyValue(char *timekey,time_t tstamp, ClusterContext context,char *name,double q)

{
 char key[1024];
 char value[1024];
 double oldq = 0,oldav = 0,oldvar = 0,newav,newvar,var;
 time_t lastseen = 0, avdt = 0, dtvar = 0;
 int anomaly;

 snprintf(key,1024,"(%s,%s,%s,%s,%s)",context.namespace,context.pod,context.appsysname,timekey,name);

 RPLY = redisCommand(RC,"GET %s",key);

 if (RPLY->str)
    {
    sscanf(RPLY->str,"(%lf,%lf,%lf,%ld,%ld,%ld)",&oldq,&oldav,&oldvar,&lastseen,&avdt,&dtvar);
    anomaly = NONE;
    }
 else
    {
    anomaly = APPEARED;
    oldav = 0;
    oldvar = 0;
    lastseen = 0;
    avdt = 3600*24;         // now the timescale is a logging time not a sampling time? 5mins invariant
    dtvar = 3600*24;
    }
 
 freeReplyObject(RPLY);

 newav = WAverage(oldav,q);
 var = (q-newav)*(q-newav);
 newvar = WAverage(oldvar,var);

 time_t dt = tstamp - lastseen;
 double newavdt = WAverage (avdt,dt), newdtvar = WAverage(dtvar,(newavdt-dt)*(newavdt-dt));

 snprintf(value,1024,"(%.2lf,%.2lf,%.2lf,%ld,%.2lf,%.2lf)",q,newav,newvar,tstamp,newavdt,newdtvar);

 // Expire data after 8 days without update, allow weekly updates at most (TBD)

 RPLY = redisCommand(RC,"SET %s %s ex %d",key,value,3600*24*8);
 freeReplyObject(RPLY);
 
 double bar = 3.0 * sqrt(newvar);

 // For logs, the usual criteria are not appropriate, since we don't get enough entropy in the signal
 // to enable consistent separation of average and fluctuation - we're not looking for names that are
 // anomalous in quantity, but for the signals that are anomalous in NAME (new or very rare) - when
 // signals are rare they will NOT follow the weekly pattern, so that could be a prerequisite
 
 if (q > (newav + bar))
    {
    // Quantity anomaly - verbosity
    anomaly = HIGH;    
    SetApplicationContext(name,anomaly);
    }
 else if (q < (newav - bar))
    {
    // Quantity anomaly - unusually quiet
    anomaly = LOW;
    SetApplicationContext(name,anomaly);
    }
 else if ((anomaly==APPEARED) || (dt > 3600*24)) // What is the natural sampling timescale?
    {
    // Tolerance anomaly - system not immunized against this pathogen
    SetApplicationContext(name,APPEARED);
    }

// Learn lastseen time too ..
 
 // This process is only run over aggregate application/pod

}

/*****************************************************************************/

void SetApplicationContext(char *key,int anomaly)
{
 char anomaly_name[256];

 switch (anomaly)
    {
    case HIGH:
        snprintf(anomaly_name,256,"high_%s",key);
        break;
    case LOW:
        snprintf(anomaly_name,256,"low_%s",key);
        break;
    case APPEARED:
        snprintf(anomaly_name,256,"rare_%s",key);
        break;        
    default:
        return;
    }

 // Import minimal CFEngine ItemList library..

 Print("ANOMALY %s\n",anomaly_name);
 IdempPrependItem(&APPL_CONTEXT,anomaly_name,NULL);
}

/*****************************************************************************/

char *MatchIPv4addr(char *buffer,char *addr, char **start, char **end)
{
 char *regex4 = "[HTTPhttp:/]*[0-9][0-9]*[0-9]*[._][0-9][0-9]*[0-9]*[._][0-9][0-9]*[0-9]*[._][0-9][0-9]*[0-9]*[:0-9]*[/:_][0-9]*";
 regex_t    preg;
 int        rc;
 size_t     nmatch = 2;
 regmatch_t pmatch[2];
 
 if (strlen(buffer) < 2)
    {
    *start = *end = NULL;
    return NULL;
    }
 
 if ((rc = regcomp(&preg,regex4, 0)) == 0)
    {
    if ((rc = regexec(&preg, buffer, nmatch, pmatch,0)) == 0)
       {
       *start = (buffer+pmatch[0].rm_so);
       *end = (buffer+pmatch[0].rm_eo);

       strncpy(addr,*start,*end - *start);
       regfree(&preg);
       
       Debug("matched ipv4 (%s) in %s\n",addr,buffer);
       return addr;       
       }

    regfree(&preg);
    }
}

/*****************************************************************************/

char *MatchIPv6addr(char *buffer,char *addr, char **start, char **end)
{
 char *regex6 = "[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][:][a-fA-F0-9:]*[:][a-fA-F0-9:]*";
 regex_t    preg;
 int        rc;
 size_t     nmatch = 2;
 regmatch_t pmatch[2];
 
 if (strlen(buffer) < 2)
    {
    *start = *end = NULL;
    return NULL;
    }

 if ((rc = regcomp(&preg,regex6, 0)) == 0)
    {
    if ((rc = regexec(&preg, buffer, nmatch, pmatch,0)) == 0)
       {
       *start = buffer+pmatch[0].rm_so;
       *end = buffer+pmatch[0].rm_eo;

       strncpy(addr,*start,*end-*start);
       regfree(&preg);

       if (strlen(addr) > 13) // Don't muddle a date string
          {
          Debug("matched ipv6 (%s)\n",addr);
          return addr;
          }
       }
    regfree(&preg);
    }

 return NULL;
}

/*****************************************************************************/

int MatchJSON(char *buffer, FILE *fp, char *uri,char *timekey, ClusterContext context)
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
                Print("CHANGE in embedded JSON (%s) from (%.64s) to (%.64)\n",uri,sp1,sp2);
                snprintf(newname,256,"json file change in %s",uri);
                IncrementCounter(timekey,newname,context,1);
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

char *MatchURI(char *buffer,char *URI,char *timekey, ClusterContext context)
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
       // roll back to ws in case doesn't start with /
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
       }
    while(c == '/' || c == '_' || c == '-' || c == '.' || !isspace(c) && !ispunct(c));
    
    *(spt-1) = '\0';
    
    Debug("URI (%s)\n",URI);
    char uri[CGN_BUFSIZE];
    snprintf(uri,CGN_BUFSIZE,"uri:%s",URI);
    
    IncrementCounter(timekey,uri,context,1);
    GraphURIConcept(URIname(URI));
    }

 return URI;
}

/*****************************************************************************/

void ExtractMessages(char *msg,char *timekey, ClusterContext context,char *addr4,char *addr6,char *uri)
{
 char qstring[1024];
 char stripped[512] = {0};
 char *sp = msg, *spt = stripped, *spq = qstring;
 char concept[2048];

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
          if ((*sp == ' ') && (*(sp+1) == ' '))
             {
             sp++;
             }
          else
             {
             *spq++ = *sp++;
             }
          }
       
       *spq = '\0';
       
       if (!StupidString(qstring))
          {
          Debug("QSTRING (\"%s\")\n",qstring);
          IncrementCounter(timekey,qstring,context,1);
          snprintf(concept,1024,"message fragment %s",qstring);
          GraphFragmentConcept(concept);
          }
       
       spq = qstring;
       memset(qstring,0,1024);
       sp++;
       }
    else
       {
       if (*sp != '\0' && isalpha(*sp)|| *sp == '.' || *sp == ':' || *sp == ' ') // strip numbers and junk
          {
          if ((*sp == ' ') && (*(sp+1) != ' '))
             {
             *spt++ = *sp;          
             }
          }
       sp++;
       if (spt > stripped + 510)
          {
          break;
          }
       }
    }

 *spt = '\0';
 
 for (sp = stripped + strlen(stripped); sp > stripped; sp--)
    {
    if (isspace(*sp))
       {
       *sp = '\0';
       }
    }

 for (sp = stripped; isspace(*sp) && *sp != '\0'; sp++)
    {
    }

 if (!StupidString(sp))
    {
    IncrementCounter(timekey,sp,context,1);
    snprintf(concept,1024,"message fragment %s",sp);
    GraphFragmentConcept(sp);
    
    // If these are coactivated, significant cluster (take only the last qstring, probably fine but lazy coding..)
    
    int l4 = strlen(addr4);
    int l6 = strlen(addr6);
    int luri = strlen(uri);
    int lq = strlen(qstring);
    int ls = strlen(sp);
    int tot = l4 + l6 + luri + lq + ls;
    
    if ((l4||l6||luri||lq) && (tot > 0) && (tot < 2000))
       {
       snprintf(concept,1024,"message event `%s' concerning: ",sp);
       
       char *attr = strchr(concept,':') + 1;
       
       if (l4)
          {
          strcat(concept,addr4);
          }
       
       if (l6)
          {
          if (l4)
             {
             strcat(concept,",");
             }
          strcat(concept,addr6);
          }
       
       if (luri)
          {
          if (l4||l6)
             {
             strcat(concept,",");
             }
          strcat(concept,uri);
          }
       
       if (strlen(qstring) >0)
          {
          if (l4||l6||luri)
             {
             strcat(concept,",");
             }
          strcat(concept,qstring);
          }

       strcat(concept,",");
       strcat(concept,sp);

       if (!GraphFragmentConcept(concept))
          {
          RoleGr(stdout,concept,"message event",attr,"kubernetes cluster log output");
          }
       }
    }
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

    regfree(&preg);
    }

 return NULL;
}

/************************************************************************************/

void GraphIPConcept(char *concept)
{
 unsigned int hash = Hash(concept,HASHTABLESIZE);

 if (!GRAPHS)
    {
    return;
    }
 
 if (IPTABLE[hash] == NULL)
    {
    IPTABLE[hash] = strdup(concept);
    }
 else
    {
    if (strncmp(concept,IPTABLE[hash],4) != 0)
       {
       Print("COLLISION in IP\n");
       }
    }
}

/************************************************************************************/

void GraphURIConcept(char *concept)
{
 unsigned int hash = Hash(concept,HASHTABLESIZE);

 if (!GRAPHS)
    {
    return;
    }

 if (URITABLE[hash] == NULL)
    {
    URITABLE[hash] = strdup(concept);
    }
 else
    {
    if (strncmp(concept,URITABLE[hash],4) != 0)
       {
       Debug("COLLISION in URI\n");
       }
    }
}

/************************************************************************************/

int GraphFragmentConcept(char *concept)
{
 unsigned int hash = Hash(concept,HASHTABLESIZE);

  if (!GRAPHS)
    {
    return;
    }

  if (FRAGMENTTABLE[hash] == NULL)
    {
    FRAGMENTTABLE[hash] = strdup(concept);
    return false;
    }
 else
    {
    if (strncmp(concept,FRAGMENTTABLE[hash],4) != 0)
       {
       Debug("COLLISION in fragments\n");
       }
    return true;
    }
}

/************************************************************************************/

void GraphConcepts(ClusterContext context)
{
 char ictx[1024];
 int i;

  if (!GRAPHS)
    {
    return;
    }

 if (context.namespace)
    {
    snprintf(ictx,1024,"kubernetes deployment %s in namespace",context.pod,context.namespace);
    }
 else if (context.pod)
    {
    snprintf(ictx,1024,"kubernetes deployment %s",context.pod);
    }
 else
    {
    snprintf(ictx,1024,"kubernetes deployment");
    }
 
 for (i = 0; i < HASHTABLESIZE; i++)
    {
    if (IPTABLE[i])
       {
       RoleGr(stdout,IPTABLE[i],"ipv4 address",IPTABLE[i]+strlen("ipv4 address "),ictx);
       free(IPTABLE[i]);
       }

    if (URITABLE[i])
       {
       RoleGr(stdout,URITABLE[i],"resource path or URI",URITABLE[i]+strlen("resource path or URI "),ictx);
       free(URITABLE[i]);
       }

    if (FRAGMENTTABLE[i])
       {
       RoleGr(stdout,FRAGMENTTABLE[i],"message fragment",FRAGMENTTABLE[i]+strlen("message fragment "),ictx);
       free(FRAGMENTTABLE[i]);
       } 
    }
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

void IncrementCounter(char *timekey,char *namekey, ClusterContext context, int value)
{
 // When we are dealing with aggregate, centralized logging, we may need to retain origin context
 // which shouldn't have been propagated in the first place

 // We want quickly to turn the namekey into an easily recognizable canonical invariant,
 // shorter than 256 characters, and document it in the semantic graph

 char canon[1024];
 char *sp = namekey,*spt = canon;
 char last = 'x';

 if (strlen(namekey) == 0)
    {
    return;
    }
 
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

 if (strlen(canon) < INSIGNIFICANT || StupidString(canon))
    {
    return;
    }

 // The keyname (context/name,timekey, canonized metric name)

 unsigned int hash = Hash(canon,HASHTABLESIZE);

 Canonify(canon);
 
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
       Print("COLLISION at slot %d!!!! \n - %s (%d)\n - %s (%d)\n",hash,canon,strlen(canon),METRICS.name[hash],strlen(METRICS.name[hash]));
       }
    
    METRICS.Q[hash].q += value;
    }

 TOTALMESSAGES++;
 
 Debug("Increment (%s,<%s,%s,%s>,%s) +=%d\n",timekey,context.namespace,context.pod,context.appsysname,canon,value);    
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

/************************************************************************************/

void Erase(char *start, char *end)

{ char *sp;

 if (start != NULL)
    {
    for (sp = start; *sp != '\0' && sp <= end; sp++)
       {
       *sp = ' ';
       }
    }
}

/*********************************************************************/

char *Trunc(char *s)

{ char *sp = s,*last = NULL;
 
 while (sp = strchr(sp,'/'))
    {
    last = ++sp;    
    }
 
 return last;
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

/*********************************************************************/

int StupidString(char *s)
{
 char c,*sp, max=' ', min='z';
 int len = 0, onlyhex = true; 

 for (sp = s; *sp != '\0'; sp++)
    {
    c = toupper(*sp);
    len++;

    if (!isxdigit(*sp) && (*sp != '_') && (*sp != ' '))
       {
       onlyhex = false;
       }
    
    if (c == ' ' || c == '_' || c == '-')
       {
       continue;
       }
    
    if (c < min)
       {
       min = c;
       }
    
    if (c > max)
       {
       max = c;
       }
    }

 if (onlyhex || (max-min) < ('F'-'A')+('9'-'0'))
    {
    //printf("STUPID '%s' - (%c/%d)-(%c/%d)\n",s,min,min,max,max);
    return true;  // maybe just a hex string
    }
 else
    {
    if (len > INSIGNIFICANT)
       {
       //printf("NSTUPID '%s' - (%c/%d)-(%c/%d) %d <=> %d\n",s,min,min,max,max,max-min,('F'-'A')+('9'-'0'));
       return false;
       }
    return true;
    }
}
