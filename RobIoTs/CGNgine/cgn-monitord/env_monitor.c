/*
   Copyright (C) Mark Burgess

   This file is derived in part from the code written by MB for CFEngine.
   It has been modified extensively in concept, but some attempt has been
   made for preserve compatibility with CFEngine 3.x.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <history.h>
#include <probes.h>
#include <env_monitor.h>
#include <monitoring.h>
#include <eval_context.h>
#include <mon.h>
#include <granules.h>
#include <dbm_api.h>
#include <policy.h>
#include <promises.h>
#include <item_lib.h>
#include <conversion.h>
#include <ornaments.h>
#include <expand.h>
#include <scope.h>
#include <sysinfo.h>
#include <signals.h>
#include <locks.h>
#include <exec_tools.h>
#include <generic_agent.h> // WritePID
#include <files_lib.h>
#include <unix.h>
#include <verify_measurements.h>
#include <verify_classes.h>
#include <unix_iface.h>
#include <time_classes.h>
#include <graph.h>

/*****************************************************************************/
/* Globals                                                                   */
/*****************************************************************************/

#define cf_noise_threshold 6    /* number that does not warrant large anomaly status */
#define MON_THRESHOLD_HIGH 1000000      // samples should stay below this threshold
#define LDT_BUFSIZE 10
#define ITER_RESET  300
#define CF_ENVNEW_FILE   "env_data.new"
#define CF_GRAPHNEW_FILE "env_graph.new"
#define CF_GRAPH_FILE    "env_graph"


/*****************************************************************************/

static char ENVFILE_NEW[CF_BUFSIZE] = "";
static char ENVFILE[CF_BUFSIZE] = "";

static char GRAPHFILE_NEW[CF_BUFSIZE] = "";
static char GRAPHFILE[CF_BUFSIZE] = "";

static double HISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS] = { { { 0.0 } } };
static Averages MAX, MIN;

/* persistent observations */

static double CF_THIS[CF_OBSERVABLES] = { 0.0 };

/* Work */

int SLEEPTIME = 60;  

static long ITER = ITER_RESET;               /* Iteration since start */
static double AGE = 0.0, WAGE = 0.0;        /* Age and weekly age of database */
double FORGETRATE = 0.7;

/* Leap Detection vars */

static double LDT_BUF[CF_OBSERVABLES][LDT_BUFSIZE] = { { 0 } };
static double LDT_SUM[CF_OBSERVABLES] = { 0 };
static double LDT_AVG[CF_OBSERVABLES] = { 0 };
static double CHI_LIMIT[CF_OBSERVABLES] = { 0 };
static double CHI[CF_OBSERVABLES] = { 0 };
static double LDT_MAX[CF_OBSERVABLES] = { 0 };
static int LDT_POS = 0;
static int LDT_FULL = false;

int NO_FORK = false;

/*******************************************************************/
/* Prototypes                                                      */
/*******************************************************************/

static void GetDatabaseAge(void);
static void LoadHistogram(void);
static void SaveHistogram(void);
static void GetQ(EvalContext *ctx, const Policy *policy);
static void EvalAvQ(EvalContext *ctx, char *t, Averages *newvals, Timescales *newt);
static void BuildConsciousState(EvalContext *ctx, Averages newvals, Timescales newt);
static void GatherPromisedMeasures(EvalContext *ctx, const Policy *policy);
static void ReadAverages(void);
static void LeapDetection(void);
static Averages *GetCurrentAverages(char *timekey);
static void UpdateAverages(EvalContext *ctx, char *timekey, Averages *newvals);
static void UpdateDistributions(EvalContext *ctx, char *timekey, Averages *av);
static double WAverage(double newvals, double oldvals, double age);
static double SituationHereAndNow(EvalContext *ctx, FILE *consc, char *now, char *namespace, char *name, char *desc, double variable, double dq, double av_expect, double av_var, double t_expect,double t_dev, Item **classlist);
static void SetVariable(FILE *consc, char *name, double now, double average, double stddev, Item **list);
static double RejectAnomaly(double new, double av, double var);
static void ZeroArrivals(void);
static PromiseResult KeepMonitorPromise(EvalContext *ctx, const Promise *pp, void *param);
static void GetNamespace(int index, char *buffer);
static void AnnotateContext(EvalContext *ctx, FILE *fp, char *now);
static void AnnotateNumbers(FILE *consc,char *now,char *origin, char *name, char *gradient, char *state, char* level, double q, double E, double sig, double Et, double tsig, char *description);
static void AnnotateOrigin(FILE *consc,char *now,char *origin,char *name, char *description);

/****************************************************************/

void MonitorInitialize(void)
{
 int i, j, k;
 char vbuff[CF_BUFSIZE];
 
 snprintf(vbuff, sizeof(vbuff), "%s/state/cf_users", CFWORKDIR);
 MapName(vbuff);
 CreateEmptyFile(vbuff);
 
 snprintf(ENVFILE_NEW, CF_BUFSIZE, "%s/state/%s", CFWORKDIR, CF_ENVNEW_FILE);
 MapName(ENVFILE_NEW);
 
 snprintf(ENVFILE, CF_BUFSIZE, "%s/state/%s", CFWORKDIR, CF_ENV_FILE);
 MapName(ENVFILE);

 snprintf(GRAPHFILE_NEW, CF_BUFSIZE, "%s/state/%s", CFWORKDIR, CF_GRAPHNEW_FILE);
 MapName(GRAPHFILE_NEW);
 
 snprintf(GRAPHFILE, CF_BUFSIZE, "%s/state/%s", CFWORKDIR, CF_GRAPH_FILE);
 MapName(GRAPHFILE);

 //MonEntropyClassesInit();
 
 GetDatabaseAge();
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    for (j = 0; j < 7; j++)
       {
       for (k = 0; k < CF_GRAINS; k++)
          {
          HISTOGRAM[i][j][k] = 0;
          }
       }
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    CHI[i] = 0;
    CHI_LIMIT[i] = 0.1;
    LDT_AVG[i] = 0;
    LDT_SUM[i] = 0;
    }
 
 srand((unsigned int) time(NULL));
 LoadHistogram();
 
/* Look for local sensors - this is unfortunately linux-centric */
 
 MonTempInit();
 MonOtherInit();
 
 Log(LOG_LEVEL_DEBUG, "Finished with monitor initialization");
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

static void GetDatabaseAge()
{
 CF_DB *dbp;
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }
 
 if (ReadDB(dbp, "DATABASE_AGE", &AGE, sizeof(double)))
    {
    WAGE = AGE / SECONDS_PER_WEEK * CF_MEASURE_INTERVAL;
    Log(LOG_LEVEL_DEBUG, "Previous DATABASE_AGE %f", AGE);
    }
 else
    {
    Log(LOG_LEVEL_DEBUG, "No previous DATABASE_AGE");
    AGE = 0.0;
    }
 
 CloseDB(dbp);
}

/*********************************************************************/

static void LoadHistogram(void)
{
 FILE *fp;
 int i, day, position;
 double maxval[CF_OBSERVABLES];
 
 char filename[CF_BUFSIZE];
 
 snprintf(filename, CF_BUFSIZE, "%s/state/histograms", CFWORKDIR);
 
 if ((fp = fopen(filename, "r")) == NULL)
    {
    Log(LOG_LEVEL_VERBOSE, "Unable to load histogram data. (fopen: %s)", GetErrorStr());
    return;
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    maxval[i] = 1.0;
    }
 
 for (position = 0; position < CF_GRAINS; position++)
    {
    if (fscanf(fp, "%d ", &position) != 1)
       {
       Log(LOG_LEVEL_ERR, "Format error in histogram file '%s' - aborting", filename);
       break;
       }
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       for (day = 0; day < 7; day++)
          {
          if (fscanf(fp, "%lf ", &(HISTOGRAM[i][day][position])) != 1)
             {
             Log(LOG_LEVEL_VERBOSE, "Format error in histogram file '%s'. (fscanf: %s)", filename, GetErrorStr());
             HISTOGRAM[i][day][position] = 0;
             }
          
          if (HISTOGRAM[i][day][position] < 0)
             {
             HISTOGRAM[i][day][position] = 0;
             }
          
          if (HISTOGRAM[i][day][position] > maxval[i])
             {
             maxval[i] = HISTOGRAM[i][day][position];
             }
          
          HISTOGRAM[i][day][position] *= 1000.0 / maxval[i];
          }
       }
    }
 
 fclose(fp);
}

/*********************************************************************/

static void SaveHistogram(void)
{
 FILE *fp;
 int i, day, position;
 
 char filename[CF_BUFSIZE];

 snprintf(filename, CF_BUFSIZE, "%s/state/histograms", CFWORKDIR);
 
 if ((fp = fopen(filename, "w")) == NULL)
    {
    Log(LOG_LEVEL_VERBOSE, "Unable to save histogram data. (fopen: %s)", GetErrorStr());
    return;
    }
 
 for (position = 0; position < CF_GRAINS; position++)
    {
    fprintf(fp, "%d ", position);
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       for (day = 0; day < 7; day++)
          {
          fprintf(fp, "%lf ", HISTOGRAM[i][day][position]);
          }
       }
    }
 
 fclose(fp);
}

/*********************************************************************/

void MonitorStartServer(EvalContext *ctx, const Policy *policy)
{
 char timekey[CF_SMALLBUF];
 Averages averages;
 Timescales times;
 Policy *monitor_cfengine_policy = PolicyNew();
 Promise *pp = NULL;
 
 Bundle *bp = PolicyAppendBundle(monitor_cfengine_policy, NamespaceDefault(), "monitor_cfengine_bundle", "agent", NULL, NULL);
 PromiseType *tp = BundleAppendPromiseType(bp, "monitor_cfengine");
 pp = PromiseTypeAppendPromise(tp, "the monitor daemon", (Rval) { NULL, RVAL_TYPE_NOPROMISEE }, NULL);
 assert(pp);
 CfLock thislock;
 
#ifdef __MINGW32__
 
 if (!NO_FORK)
    {
    Log(LOG_LEVEL_VERBOSE, "Windows does not support starting processes in the background - starting in foreground");
    }
 
#else /* !__MINGW32__ */
 
 if ((!NO_FORK) && (fork() != 0))
    {
    Log(LOG_LEVEL_INFO, "cf-monitord: starting");
    _exit(EXIT_SUCCESS);
    }
 
 if (!NO_FORK)
    {
    ActAsDaemon();
    }
 
#endif /* !__MINGW32__ */
 
 TransactionContext tc =
     {
         .ifelapsed = 0,
         .expireafter = 0,
     };
 
 thislock = AcquireLock(ctx, pp->promiser, VUQNAME, CFSTARTTIME, tc, pp, false);
 if (thislock.lock == NULL)
    {
    PolicyDestroy(monitor_cfengine_policy);
    return;
    }
 
 WritePID("cf-monitord.pid");
 
 while (!IsPendingTermination())
    {
    EvalContextClear(ctx);
    DetectEnvironment(ctx);
    UpdateTimeClasses(ctx, time(NULL));
    
    GetQ(ctx, policy);
    snprintf(timekey, sizeof(timekey), "%s", GenTimeKey(time(NULL)));
    EvalAvQ(ctx, timekey, &averages, &times);
    LeapDetection();
    BuildConsciousState(ctx, averages,times);

    sleep(SLEEPTIME);
    ZeroArrivals();

    if (ITER-- == 0)
       {
       Mon_DumpSlots();
       SaveHistogram();
       ITER = ITER_RESET;
       }
    }

 PolicyDestroy(monitor_cfengine_policy);
 YieldCurrentLock(thislock);
}

/*********************************************************************/

static void GetQ(EvalContext *ctx, const Policy *policy)
{
 ZeroArrivals();

 Banner(" * SAMPLING SENSORS - basic system environment");
 
 MonProcessesGatherData(CF_THIS);
 MonDiskGatherData(CF_THIS);
#ifndef __MINGW32__
 MonLoadGatherData(CF_THIS);
 MonCPUGatherData(CF_THIS);
 MonTempGatherData(CF_THIS);
 
 Banner(" * SAMPLING SENSORS - well known network ports");
 MonNetworkGatherData(CF_THIS);

#endif /* !__MINGW32__ */
 MonOtherGatherData(CF_THIS);

 Banner(" * PROMISED SAMPLED");

 GatherPromisedMeasures(ctx, policy);
}

/*********************************************************************/

static void EvalAvQ(EvalContext *ctx, char *t, Averages *newvals, Timescales *newt)
{
 Averages *lastweek_vals;
 static Averages last5_q;
 static Timescales last5_t;
 char name[CF_MAXVARSIZE];
 time_t now = time(NULL);
 int i;
 
 Banner(" * SENSOR ANOMALY DETECTION - Detecting and sampling standard active sensors (weekly model)");
 
 if ((lastweek_vals = GetCurrentAverages(t)) == NULL)
    {
    Log(LOG_LEVEL_ERR, "Error reading average database");
    exit(EXIT_FAILURE);
    }

/* Discard any apparently anomalous behaviour before renormalizing database */
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    double delta2;
    char desc[CF_BUFSIZE];
    double This;
    name[0] = '\0';
    GetObservable(i, name, desc);
    
    /* Overflow protection */
    
    if (lastweek_vals->Q[i].expect < 0)
       {
       lastweek_vals->Q[i].expect = 0;
       }
    
    if (lastweek_vals->Q[i].q < 0)
       {
       lastweek_vals->Q[i].q = 0;
       }
   
    if (lastweek_vals->Q[i].var < 0)
       {
       lastweek_vals->Q[i].var = 0;
       }
    
    // lastweek_vals is last week's stored data
    
    This = RejectAnomaly(CF_THIS[i], lastweek_vals->Q[i].expect, lastweek_vals->Q[i].var);
    
    newvals->Q[i].q = This;

    // First record the time scales and their averages, assuming that good ones are fairly stable
    
    newt->T[i].last_seen = now;  // Record the freshness of this slot

    int change = false;

    newvals->Q[i].dq = newvals->Q[i].q - last5_q.Q[i].q;
    newvals->Q[i].dq_ex =  WAverage(newvals->Q[i].dq, last5_q.Q[i].dq_ex, WAGE);
    double dq2 = newvals->Q[i].dq * newvals->Q[i].dq;
    newvals->Q[i].dq_var =  WAverage(dq2, last5_q.Q[i].dq_var, WAGE);
    
    if (newvals->Q[i].dq != 0)
       {
       newt->T[i].last_change = now;
       change = true;
       }
    else
       {
       newt->T[i].last_change = last5_t.T[i].last_change;
       }

    newt->T[i].dchange = newt->T[i].last_change - last5_t.T[i].last_change;    
    newt->T[i].dchange_expect = WAverage(newt->T[i].dchange, last5_t.T[i].dchange, WAGE);
    double dt_change2 = newt->T[i].dchange * newt->T[i].dchange;
    newt->T[i].dchange_var = WAverage(dt_change2, last5_t.T[i].dchange_var, WAGE);

    // Now the sensors
    
    newvals->Q[i].expect = WAverage(This, lastweek_vals->Q[i].expect, WAGE);
        
    delta2 = (This - lastweek_vals->Q[i].expect) * (This - lastweek_vals->Q[i].expect);
    
    if (lastweek_vals->Q[i].var > delta2 * 2.0)
       {
       /* Clean up past anomalies */
       newvals->Q[i].var = delta2;
       }
    else
       {
       newvals->Q[i].var = WAverage(delta2, lastweek_vals->Q[i].var, WAGE);
       }

    if (This > 0 || change)
       {
       Log(LOG_LEVEL_VERBOSE, "[%d] %s Sample=%.2lf, Average=%.2lf -> Deviation=%.2lf", i, name, newvals->Q[i].q, newvals->Q[i].expect, sqrt(newvals->Q[i].var));
       Log(LOG_LEVEL_VERBOSE, " %c   Dq = %.2lf (%.2lf#%.2lf)\n     Dt(Dq)/av = %.2lf/%.2lf", change ? '*' : ' ',  newvals->Q[i].dq, newvals->Q[i].dq_ex, sqrt(newvals->Q[i].dq_var), newt->T[i].dchange, newt->T[i].dchange_expect);
       }

    last5_q.Q[i] = newvals->Q[i];
    last5_t.T[i] = newt->T[i];
    }
 
 UpdateAverages(ctx, t, newvals);
 UpdateDistributions(ctx, t, lastweek_vals);        /* Distribution about mean */
}

/*********************************************************************/

static void LeapDetection(void)
{
 int i, last_pos = LDT_POS;
 double n1, n2, d;
 double padding = 0.2;
 
 if (++LDT_POS >= LDT_BUFSIZE)
    {
    LDT_POS = 0;
    
    if (!LDT_FULL)
       {
       Log(LOG_LEVEL_DEBUG, "LDT Buffer full at %d", LDT_BUFSIZE);
       LDT_FULL = true;
       }
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    /* First do some anomaly rejection. Sudden jumps must be numerical errors. */
    
    if ((LDT_BUF[i][last_pos] > 0) && ((CF_THIS[i] / LDT_BUF[i][last_pos]) > 1000))
       {
       CF_THIS[i] = LDT_BUF[i][last_pos];
       }
    
    /* Note AVG should contain n+1 but not SUM, hence funny increments */
    
    LDT_AVG[i] = LDT_AVG[i] + CF_THIS[i] / ((double) LDT_BUFSIZE + 1.0);
    
    d = (double) (LDT_BUFSIZE * (LDT_BUFSIZE + 1)) * LDT_AVG[i];
    
    if (LDT_FULL && (LDT_POS == 0))
       {
       n2 = (LDT_SUM[i] - (double) LDT_BUFSIZE * LDT_MAX[i]);
       
       if (d < 0.001)
          {
          CHI_LIMIT[i] = 0.5;
          }
       else
          {
          CHI_LIMIT[i] = padding + sqrt(n2 * n2 / d);
          }
       
       LDT_MAX[i] = 0.0;
       }
    
    if (CF_THIS[i] > LDT_MAX[i])
       {
       LDT_MAX[i] = CF_THIS[i];
       }
    
    n1 = (LDT_SUM[i] - (double) LDT_BUFSIZE * CF_THIS[i]);
    
    if (d < 0.001)
       {
       CHI[i] = 0.0;
       }
    else
       {
       CHI[i] = sqrt(n1 * n1 / d);
       }
    
    LDT_AVG[i] = LDT_AVG[i] - LDT_BUF[i][LDT_POS] / ((double) LDT_BUFSIZE + 1.0);
    LDT_BUF[i][LDT_POS] = CF_THIS[i];
    LDT_SUM[i] = LDT_SUM[i] - LDT_BUF[i][LDT_POS] + CF_THIS[i];
    }
}

/*********************************************************************/

static void PublishEnvironment(Item *classes)
{
 FILE *fp;
 Item *ip;
 
 unlink(ENVFILE_NEW);
 
 if ((fp = fopen(ENVFILE_NEW, "a")) == NULL)
    {
    return;
    }
 
 for (ip = classes; ip != NULL; ip = ip->next)
    {
    fprintf(fp, "%s\n", ip->name);
    }
 
 fclose(fp);
 rename(ENVFILE_NEW, ENVFILE);
}

/*********************************************************************/

static void AddOpenPorts(const char *name, const Item *value, Item **mon_data)
{
 Writer *w = StringWriter();
 WriterWriteF(w, "@%s=", name);
 PrintItemList(value, w);
 if (StringWriterLength(w) <= 1500)
    {
    AppendItem(mon_data, StringWriterData(w), NULL);
    }
 WriterClose(w);
}

/*********************************************************************/

static void BuildConsciousState(EvalContext *ctx, Averages av, Timescales t)
{
 double sigma;
 Item *ip, *mon_data = NULL;
 int i, j, k;
 char buff[CF_BUFSIZE], ldt_buff[CF_BUFSIZE], name[CF_MAXVARSIZE], namespace[CF_MAXVARSIZE];
 static int anomaly[CF_OBSERVABLES][LDT_BUFSIZE] = { { 0 } };
 extern Item *ALL_INCOMING;
 extern Item *MON_UDP4, *MON_UDP6, *MON_TCP4, *MON_TCP6, *MON_RAW4, *MON_RAW6;
 int count = 1;

 char now[CF_SMALLBUF];
 time_t nowt = time(NULL);
 snprintf(now,CF_SMALLBUF, "t_%s", GenTimeKey(nowt));
 UpdateTimeClasses(ctx, nowt);

 unlink(GRAPHFILE_NEW);
 FILE *consc = fopen(GRAPHFILE_NEW, "w");

 // Device source

 // Mobile container? Find about processes
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char desc[CF_BUFSIZE];
    
    GetObservable(i, name, desc);
    GetNamespace(i, namespace);
    
    sigma = SituationHereAndNow(ctx,consc,now,namespace, name, desc, CF_THIS[i], av.Q[i].dq, av.Q[i].expect, av.Q[i].var, t.T[i].dchange_expect, sqrt(t.T[i].dchange_var), &mon_data);

    SetVariable(consc, name, CF_THIS[i], av.Q[i].expect, sigma, &mon_data);
    Gr(consc,now,a_contains,name);
    AnnotateOrigin(consc,now,"observations",name,desc);

    /* LDT */
    
    ldt_buff[0] = '\0';
    anomaly[i][LDT_POS] = false;
    
    if (!LDT_FULL)
       {
       anomaly[i][LDT_POS] = false;
       }
    
    if (LDT_FULL && (CHI[i] > CHI_LIMIT[i]))
       {
       anomaly[i][LDT_POS] = true; /* Remember the last anomaly value */
       
       Log(LOG_LEVEL_VERBOSE, "Short timescale change LDT(%d) in %s chi = %.2f thresh %.2f ", LDT_POS, name, CHI[i], CHI_LIMIT[i]);
       
       /* Last printed element is now */
       
       for (j = LDT_POS + 1, k = 0; k < LDT_BUFSIZE; j++, k++)
          {
          if (j == LDT_BUFSIZE)   /* Wrap */
             {
             j = 0;
             }
          
          if (anomaly[i][j])
             {
             snprintf(buff, CF_BUFSIZE, " *%.2f*", LDT_BUF[i][j]);
             }
          else
             {
             snprintf(buff, CF_BUFSIZE, " %.2f", LDT_BUF[i][j]);
             }
          
          strcat(ldt_buff, buff);
          }
       
       if (CF_THIS[i] > av.Q[i].expect)
          {
          snprintf(buff, CF_BUFSIZE, "spike_%s_high", name);
          }
       else
          {
          snprintf(buff, CF_BUFSIZE, "spike_%s_low", name);
          }
       
       AppendItem(&mon_data, buff, "2");
       EvalContextHeapPersistentSave(ctx, buff, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
       EvalContextClassPutSoft(ctx, buff, CONTEXT_SCOPE_NAMESPACE, "");
       }
    else
       {
       for (j = LDT_POS + 1, k = 0; k < LDT_BUFSIZE; j++, k++)
          {
          if (j == LDT_BUFSIZE)   /* Wrap */
             {
             j = 0;
             }
          
          if (anomaly[i][j])
             {
             snprintf(buff, CF_BUFSIZE, " *%.2f*", LDT_BUF[i][j]);
             }
          else
             {
             snprintf(buff, CF_BUFSIZE, " %.2f", LDT_BUF[i][j]);
             }
          strcat(ldt_buff, buff);
          }
       }
    }

 Banner(" * SINGLE MEASUREMENT SAMPLE ANOMALY DETECTION");
 
 SetMeasurementPromises(&mon_data);

 Banner(" * OPEN PORT SAMPLING - sensing and recording listening ports");
 
 Log(LOG_LEVEL_VERBOSE, "Autodetecting listening/open TCP/UDP ports");

 // Report on the open ports, in various ways
 
 AddOpenPorts("listening_ports", ALL_INCOMING, &mon_data);
 AddOpenPorts("listening_udp6_ports", MON_UDP6, &mon_data);
 AddOpenPorts("listening_udp4_ports", MON_UDP4, &mon_data);
 AddOpenPorts("listening_tcp6_ports", MON_TCP6, &mon_data);
 AddOpenPorts("listening_tcp4_ports", MON_TCP4, &mon_data);
 AddOpenPorts("listening_raw4_ports", MON_RAW4, &mon_data);
 AddOpenPorts("listening_raw6_ports", MON_RAW6, &mon_data);
 
 // Port addresses
 
 if (ListLen(MON_TCP6) + ListLen(MON_TCP4) > 512)
    {
    Log(LOG_LEVEL_INFO, "Disabling address information of TCP ports in LISTEN state: more than 512 listening ports are detected");
    }
 else
    {
    for (ip = MON_TCP6; ip != NULL; ip=ip->next)
       {
       snprintf(buff,CF_BUFSIZE,"tcp6_port_addr[%s]=%s",ip->name,ip->classes);
       AppendItem(&mon_data, buff, NULL);
       Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
       }
    
    for (ip = MON_TCP4; ip != NULL; ip=ip->next)
       {
       snprintf(buff,CF_BUFSIZE,"tcp4_port_addr[%s]=%s",ip->name,ip->classes);
       AppendItem(&mon_data, buff, NULL);
       Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
       }
    }
 
 for (ip = MON_UDP6; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"udp6_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    }
 
 for (ip = MON_UDP4; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"udp4_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    }
 
 for (ip = MON_RAW6; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"raw6_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    }
 
 for (ip = MON_RAW4; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"raw4_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    }
 
 PublishEnvironment(mon_data); 
 DeleteItemList(mon_data);

 if (consc)
    {
    Gr(consc,VUQNAME,a_name,"host device");
    if (VDOMAIN[0] != '\0')
       {
       Gr(consc,VDOMAIN,a_name,"DNS domain or Workspace");
       }

    AnnotateContext(ctx, consc, now);
    }

 if (consc)
    {
    fclose(consc);    
    }

 rename(GRAPHFILE_NEW, GRAPHFILE);
}

/***********************************************************************/

static Averages *GetCurrentAverages(char *timekey)
{
 CF_DB *dbp;
 static Averages entry; /* No need to initialize */
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return NULL;
    }
 
 memset(&entry, 0, sizeof(entry));
 
 AGE++;
 WAGE = AGE / SECONDS_PER_WEEK * CF_MEASURE_INTERVAL;
 
 if (ReadDB(dbp, timekey, &entry, sizeof(Averages)))
    {
    int i;
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       Log(LOG_LEVEL_DEBUG, "Previous values (%lf,..) for time index '%s'", entry.Q[i].expect, timekey);
       }
    }
 else
    {
    Log(LOG_LEVEL_DEBUG, "No previous value for time index '%s'", timekey);
    }
 
 CloseDB(dbp);
 return &entry;
}


/*****************************************************************************/

static void UpdateAverages(ARG_UNUSED EvalContext *ctx, char *timekey, Averages *newvals)
{
 CF_DB *dbp;
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }
 
 Log(LOG_LEVEL_INFO, "Updated averages at '%s'", timekey);
 
 WriteDB(dbp, timekey, newvals, sizeof(Averages));
 WriteDB(dbp, "DATABASE_AGE", &AGE, sizeof(double));
 
 CloseDB(dbp);
 //HistoryUpdate(ctx, newvals);
}

/*****************************************************************************/

static int Day2Number(const char *datestring)
{
 int i = 0;
 
 for (i = 0; i < 7; i++)
    {
    if (strncmp(datestring, DAY_TEXT[i], 3) == 0)
       {
       return i;
       }
    }
 
 return -1;
}

/*****************************************************************************/

static void UpdateDistributions(ARG_UNUSED EvalContext *ctx, char *timekey, Averages *av)
{
 int position, day, i;
 static int counter = 0;

 Banner(" Updating equilibrated HISTOGRAM characteristics");

 if (counter == 0)
    {
    ReadAverages();
    }
 
/* Take an interval of 4 standard deviations from -2 to +2, divided into CF_GRAINS
   parts. Centre each measurement on CF_GRAINS/2 and scale each measurement by the
   std-deviation for the current time.
*/

 day = Day2Number(timekey);
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    if (MAX.Q[i].q <= 0)
       {
       continue;
       }
    
    position = CF_GRAINS * (0.5 + (int) (CF_THIS[i] - av->Q[i].expect)/(1.0 + sqrt(MAX.Q[i].var)));

    if (CF_THIS[i])
       {
       Log(LOG_LEVEL_VERBOSE," -> Histogram [%d] %.2lf/%.2lf (max %.2lf) at class position %d/%d\n", i, CF_THIS[i], av->Q[i].expect, MAX.Q[i].q, position,CF_GRAINS);
       }
    
    if (0 <= position)
       {
       HISTOGRAM[i][day][0]++;
       }
    else if (position < CF_GRAINS)
       {
       HISTOGRAM[i][day][CF_GRAINS-1]++;
       }
    else
       {
       HISTOGRAM[i][day][position]++;
       }
    }

if (counter == 0)
   {
   Log(LOG_LEVEL_VERBOSE, " -> Saving histograms");
   SaveHistogram();
   }

counter++;
}

/*****************************************************************************/

static double WAverage(double anew, double aold, double age)
{
 double av, cf_sane_monitor_limit = 9999999.0;
 double wnew, wold;
 
/* First do some database corruption self-healing */
 
 if ((aold > cf_sane_monitor_limit) && (anew > cf_sane_monitor_limit))
    {
    return 0;
    }
 
 if (aold > cf_sane_monitor_limit)
    {
    return anew;
    }
 
 if (aold > cf_sane_monitor_limit)
    {
    return aold;
    }
 
/* Now look at the self-learning */
 
 if ((FORGETRATE > 0.9) || (FORGETRATE < 0.1))
    {
    FORGETRATE = 0.6;
    }
 
 if (age < 2.0)              /* More aggressive learning for young database */
    {
    wnew = FORGETRATE;
    wold = (1.0 - FORGETRATE);
    }
 else
    {
    wnew = (1.0 - FORGETRATE);
    wold = FORGETRATE;
    }
 
 if ((aold == 0) && (anew == 0))
    {
    return 0;
    }
 
/*
 * AV = (Wnew*Anew + Wold*Aold) / (Wnew + Wold).
 *
 * Wnew + Wold always equals to 1, so we omit it for better precision and
 * performance.
 */

 av = (wnew * anew + wold * aold);
 
 if (av < 0)
    {
    /* Accuracy lost - something wrong */
    return 0.0;
    }
 
 return av;
}

/*****************************************************************************/

static double SituationHereAndNow(ARG_UNUSED EvalContext *ctx, FILE *consc, char *now, char *namespace, char *name, char *desc, double variable, double dq, double av_expect, double av_var, double t_expect,double t_dev, Item **classlist)
{
 char buffer[CF_BUFSIZE], buffer2[CF_BUFSIZE];
 char level[CF_SMALLBUF], degree[CF_SMALLBUF];
 double dev, delta, sigma;

if (strcmp(name,"spare") == 0)
   {
   return 0;
   }

delta = variable - av_expect;
sigma = sqrt(av_var);

if ((variable == 0.0) && (av_expect == 0))
   {
   return 0.0;
   }
 
buffer[0] = '\0';
strcpy(buffer, name);

if (delta > 0)
   {
   strcat(buffer, "_high");
   strcpy(level, "high");
   }
else if (delta < 0)
   {
   strcat(buffer, "_low");
   strcpy(level, "low");
   }
else
   {
   strcat(buffer, "_normal");
   strcpy(level, "flat");
   }

dev = sqrt(delta * delta / (1.0 + sigma * sigma));

char *diff = "=";
if (dq > 0)
   {
   diff = "+";
   }
if (dq < 0)
   {
   diff = "-";
   }

if (dev > 3.0 * sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_anomaly");
   strcpy(degree, "3sigma");
   AppendItem(classlist, buffer2, "3");
   if (consc)
      {
      AnnotateNumbers(consc,now,namespace,name,diff,degree,level,variable,av_expect,sigma,t_expect,t_dev,desc);
      }

   //EvalContextHeapPersistentSave(ctx, buffer2, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
   //EvalContextClassPutSoft(ctx, buffer2, CONTEXT_SCOPE_NAMESPACE, "");
   return sigma;
   }

if (dev > 2.0 * sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_dev2");
   strcpy(degree, "2sigma");
   AppendItem(classlist, buffer2, "2");
   if (consc)
      {
      AnnotateNumbers(consc,now,namespace,name,diff,degree,level,variable,av_expect,sigma,t_expect,t_dev,desc);
      }
   //EvalContextHeapPersistentSave(ctx, buffer2, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
   //EvalContextClassPutSoft(ctx, buffer2, CONTEXT_SCOPE_NAMESPACE, "");
   return sigma;
   }

if (dev <= sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_normal");
   strcpy(degree, "normal");
   AppendItem(classlist, buffer2, "0");
   if (consc)
      {
      AnnotateNumbers(consc,now,namespace,name,diff,degree,level,variable,av_expect,sigma,t_expect,t_dev,desc);
      }
   return sigma;
   }
else
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_dev1");
   strcpy(degree, "sigma");
   AppendItem(classlist, buffer2, "0");
   if (consc)
      {
      AnnotateNumbers(consc,now,namespace,name,diff,degree,level,variable,av_expect,sigma,t_expect,t_dev,desc);
      }

   return sigma;
   }
}

/*****************************************************************************/

static void SetVariable(FILE *consc, char *name, double value, double average, double stddev, Item **classlist)
{
 char var[CF_BUFSIZE];
 
 snprintf(var, CF_MAXVARSIZE, "value_%s=%.2lf", name, value);
 AppendItem(classlist, var, "");
 Gr(consc,var,a_hasrole,"value/state");
 GrQ(consc,name,a_hasvalue,value);
 
 snprintf(var, CF_MAXVARSIZE, "av_%s=%.2lf", name, average);
 AppendItem(classlist, var, "");
 Gr(consc,name,a_hasrole,"expectation value");
    
 snprintf(var, CF_MAXVARSIZE, "dev_%s=%.2lf", name, stddev);
 AppendItem(classlist, var, "");
 Gr(consc,name,a_hasrole,"standard deviation");
    
 Number(consc,value);
 Number(consc,average);
 Number(consc,stddev);
}

/*****************************************************************************/

static void ZeroArrivals()
{
 memset(CF_THIS, 0, sizeof(CF_THIS));
}

/*****************************************************************************/

static double RejectAnomaly(double new, double average, double variance)
{
 double dev = sqrt(variance);     /* Geometrical average dev */
 double delta;
 int bigger;
 
 if (average == 0)
    {
    return new;
    }
 
 if (new > MON_THRESHOLD_HIGH * 4.0)
    {
    return 0.0;
    }
 
 if (new > MON_THRESHOLD_HIGH)
    {
    return average;
    }
 
 if ((new - average) * (new - average) < cf_noise_threshold * cf_noise_threshold)
    {
    return new;
    }
 
 if (new - average > 0)
    {
    bigger = true;
    }
 else
    {
    bigger = false;
    }
 
/* This routine puts some inertia into the changes, so that the system
   doesn't respond to every little change ...   IR and UV cutoff */
 
 delta = sqrt((new - average) * (new - average));
 
 if (delta > 4.0 * dev)      /* IR */
    {
    srand48((unsigned int) time(NULL));
    
    if (drand48() < 0.7)    /* 70% chance of using full value - as in learning policy */
       {
       return new;
       }
    else
       {
       if (bigger)
          {
          return average + 2.0 * dev;
          }
       else
          {
          return average - 2.0 * dev;
          }
       }
    }
 else
    {
    return new;
    }
}

/***************************************************************/
/* Level 5                                                     */
/***************************************************************/

static void GatherPromisedMeasures(EvalContext *ctx, const Policy *policy)
{
 for (size_t i = 0; i < SeqLength(policy->bundles); i++)
    {
    const Bundle *bp = SeqAt(policy->bundles, i);
    EvalContextStackPushBundleFrame(ctx, bp, NULL, false);

    if ((strcmp(bp->type, CF_AGENTTYPES[AGENT_TYPE_MONITOR]) == 0) || (strcmp(bp->type, CF_AGENTTYPES[AGENT_TYPE_COMMON]) == 0))
       {
       for (size_t j = 0; j < SeqLength(bp->promise_types); j++)
          {
          PromiseType *sp = SeqAt(bp->promise_types, j);
                       
          EvalContextStackPushPromiseTypeFrame(ctx, sp);

          for (size_t ppi = 0; ppi < SeqLength(sp->promises); ppi++)
             {
             Banner(" SAMPLE MEASUREMENT");
             Promise *pp = SeqAt(sp->promises, ppi);
             ExpandPromise(ctx, pp, KeepMonitorPromise, NULL);
             }
          EvalContextStackPopFrame(ctx);
          }
       }
    
    EvalContextStackPopFrame(ctx);
    }
}

/*********************************************************************/
/* Level                                                             */
/*********************************************************************/

static PromiseResult KeepMonitorPromise(EvalContext *ctx, const Promise *pp, ARG_UNUSED void *param)
{
 assert(param == NULL);

 if (strcmp("vars", pp->parent_promise_type->name) == 0)
    {
    return PROMISE_RESULT_NOOP;
    }
 else if (strcmp("classes", pp->parent_promise_type->name) == 0)
    {
    return VerifyClassPromise(ctx, pp, NULL);
    }
 else if (strcmp("measurements", pp->parent_promise_type->name) == 0)
    {
    PromiseResult result = VerifyMeasurementPromise(ctx, CF_THIS, pp);
    return result;
    }
 else if (strcmp("reports", pp->parent_promise_type->name) == 0)
    {
    return PROMISE_RESULT_NOOP;
    }
 
 assert(false && "Unknown promise type");
 return PROMISE_RESULT_NOOP;
}

/*********************************************************************/

static void ReadAverages()
{
 Averages entry;
 char timekey[CF_MAXVARSIZE];
 char interval[CF_MAXVARSIZE];
 time_t now = time(NULL);
 CF_DB *dbp;
 int i;

 Log(LOG_LEVEL_VERBOSE, " -> Scanning for minimax values");
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }

 strcpy(interval, GenTimeKey(now));
 
 if (ReadDB(dbp, "DATABASE_AGE", &AGE, sizeof(double)))
    {
    Log(LOG_LEVEL_VERBOSE, " -> Database age is %.2f (weeks)\n", AGE / SECONDS_PER_WEEK * CF_MEASURE_INTERVAL);
    }

 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    MAX.Q[i].var = MAX.Q[i].expect = MAX.Q[i].q = -99999999.0;
    MIN.Q[i].var = MIN.Q[i].expect = MIN.Q[i].q = 99999999.0;
    }
 
 for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING + SECONDS_PER_WEEK; now += CF_MEASURE_INTERVAL)
    {
    strcpy(timekey, GenTimeKey(now));
    
    if (ReadDB(dbp, timekey, &entry, sizeof(Averages)))
       {
       for (i = 0; i < CF_OBSERVABLES; i++)
          {
          if (entry.Q[i].expect > MAX.Q[i].expect)
             {
             MAX.Q[i].expect = entry.Q[i].expect;
             }
          
          if (entry.Q[i].q > MAX.Q[i].q)
             {
             MAX.Q[i].q = entry.Q[i].q;
             }
          
          if (entry.Q[i].var > MAX.Q[i].var)
             {
             MAX.Q[i].var = entry.Q[i].var;
             }
          
          if (entry.Q[i].expect < MIN.Q[i].expect)
             {
             MIN.Q[i].expect = entry.Q[i].expect;
             }
          
          if (entry.Q[i].q < MIN.Q[i].q)
             {
             MIN.Q[i].q = entry.Q[i].q;
             }
          
          if (entry.Q[i].var < MIN.Q[i].var)
             {
             MIN.Q[i].var = entry.Q[i].var;
             }
          }
       }
    }

 CloseDB(dbp);
}

/*********************************************************************/

static void GetNamespace(int index, char *buffer)
{
 switch (index)
    {
    case ob_users:
        strcpy(buffer,"human");
        break;
    case ob_rootprocs:
        strcpy(buffer,"system");
        break;
    case ob_otherprocs:
        strcpy(buffer,"system|human");
        break;
    case ob_diskfree:
    case ob_loadavg:
        strcpy(buffer,"system");
        break;
    case ob_netbiosns_in:
    case ob_netbiosns_out:
    case ob_netbiosdgm_in:
    case ob_netbiosdgm_out:
    case ob_netbiosssn_in:
    case ob_netbiosssn_out:
    case ob_imap_in:
    case ob_imap_out:
    case ob_cfengine_in:
    case ob_cfengine_out:
    case ob_nfsd_in:
    case ob_nfsd_out:
    case ob_smtp_in:
    case ob_smtp_out:
    case ob_www_in:
    case ob_www_out:
    case ob_ftp_in:
    case ob_ftp_out:
    case ob_ssh_in:
    case ob_ssh_out:
    case ob_wwws_in:
    case ob_wwws_out:
    case ob_dns_in:
    case ob_dns_out:
        strcpy(buffer,"services");
        break;
    case ob_webaccess:
    case ob_weberrors:
    case ob_syslog:
    case ob_messages:
        strcpy(buffer,"logs");
        break;
    case ob_temp0:
    case ob_temp1:
    case ob_temp2:
    case ob_temp3:
        strcpy(buffer,"physical");
        break;
    case ob_cpuall:
    case ob_cpu0:
    case ob_cpu1:
    case ob_cpu2:
    case ob_cpu3:
        strcpy(buffer,"system");
        break;
        
    case ob_microsoft_ds_in:
    case ob_microsoft_ds_out:
    case ob_www_alt_in:
    case ob_www_alt_out:
    case ob_imaps_in:
    case ob_imaps_out:
    case ob_ldap_in:
    case ob_ldap_out:
    case ob_ldaps_in:
    case ob_ldaps_out:
    case ob_mongo_in:
    case ob_mongo_out:
    case ob_mysql_in:
    case ob_mysql_out:
    case ob_postgresql_in:
    case ob_postgresql_out:
    case ob_ipp_in:
    case ob_ipp_out:
        strcpy(buffer,"services");
        break;
    case ob_ospf_in:
    case ob_ospf_out:
    case ob_bgp_in:
    case ob_bgp_out:
        strcpy(buffer,"routing");
        break;
    default:                      // Empirically these seem to be the system probes Mikhail reorganized to random addresses
        strcpy(buffer,"system");
        break;
    }
}

/*****************************************************************************/

static void AnnotateContext(EvalContext *ctx, FILE *consc, char *now)
{
if (consc == NULL)
   {
   return;
   }

char here_and_now[CF_BUFSIZE];
char buff[CF_BUFSIZE];

Gr(consc,"sample times",a_contains,now);

 // The name/context is a semantic coordinate for the instances (like an array index)

snprintf(here_and_now, CF_BUFSIZE, "%s:%s",VFQNAME,now);
Gr(consc,VUQNAME,a_contains,VFQNAME);
Gr(consc,VFQNAME,a_contains,here_and_now);
Gr(consc,now,a_contains,here_and_now);

ClassTableIterator *iter = EvalContextClassTableIteratorNewGlobal(ctx, NULL, true, true);
Class *cls = NULL;

while ((cls = ClassTableIteratorNext(iter)))
   {
   Gr(consc,here_and_now,a_hasattr,cls->name);
   StringSet *tagset = EvalContextClassTags(ctx, cls->ns, cls->name);
   StringSetIterator iter = StringSetIteratorInit(tagset);
   char *name = NULL;
   while ((name = StringSetIteratorNext(&iter)))
      {
      if (strstr(name,"=") == 0)
         {
         Gr(consc,cls->name,a_related_to,name);
         Gr(consc,cls->name,a_hasrole,"class/context label");

         if (cls->ns)
            {
            Gr(consc,cls->ns,a_contains,cls->name);
            Gr(consc,cls->ns,a_hasrole,"namespace");
            }
         else
            {
            Gr(consc,"default",a_contains,cls->name);
            Gr(consc,"default",a_hasrole,"namespace");
            }
         }
      else if (strncmp(name,"name=",5) == 0)
         {
         Gr(consc,cls->name,a_hasrole,name+5);
         }
      else if (strncmp(name,"source=",7) == 0)
         {
         Gr(consc,cls->name,a_origin,name+7);
         }
      else
         {
         //??
         }
      }
   }
 
ClassTableIteratorDestroy(iter);
}

/**********************************************************************/

static void AnnotateOrigin(FILE *consc,char *now,char *origin,char *name, char *description)
{
 char here_and_now[CF_BUFSIZE];

 // The name is a semantic coordinate for the instance
 snprintf(here_and_now, CF_BUFSIZE, "%s:%s:%s",VFQNAME,now,name);

 Gr(consc,now,a_contains,here_and_now);
 Gr(consc,here_and_now,a_contains,name);
 Gr(consc,name,a_interpreted,description);
 Gr(consc,here_and_now,a_interpreted,description);
  
 // Explain meaning of name
 Gr(consc,origin,a_origin,here_and_now);

}

/**********************************************************************/

static void AnnotateNumbers(FILE *consc,char *now,char *origin, char *name, char *gradient, char *state, ARG_UNUSED char* level, double q, double E, double sig, double Et, double tsig, char *description)
{ 
 char here_and_now[CF_BUFSIZE];
 char buff[CF_BUFSIZE];

 // The name is a semantic coordinate for the instance
 snprintf(here_and_now, CF_BUFSIZE, "%s:%s:%s",VFQNAME,now,name);

 // anchor basis for value cluster

 Gr(consc,name,a_generalizes,here_and_now);
 Gr(consc,name,a_interpreted,description);
 Gr(consc,origin,a_contains,name);

 // label attributes (like object definition) and attach number symbols
 
 snprintf(buff,CF_BUFSIZE,"%s:q",here_and_now);
 Gr(consc,name,a_hasattr,buff);
 GrQ(consc,buff,a_hasvalue,q);
 Number(consc,q);
 
 snprintf(buff,CF_BUFSIZE,"%s:E",here_and_now);
 Gr(consc,name,a_hasattr,buff);
 GrQ(consc,buff,a_hasvalue,E);
 Number(consc,E);
 
 snprintf(buff,CF_BUFSIZE,"%s:sig",here_and_now);
 Gr(consc,name,a_hasattr,buff);
 GrQ(consc,buff,a_hasvalue,sig);
 Number(consc,sig);
 
 snprintf(buff,CF_BUFSIZE,"%s:Et",here_and_now);
 Gr(consc,name,a_hasattr,buff);
 GrQ(consc,buff,a_hasvalue,Et);
 Number(consc,Et);
 
 snprintf(buff,CF_BUFSIZE,"%s:tsig",here_and_now);
 Gr(consc,name,a_hasattr,buff);
 GrQ(consc,buff,a_hasvalue,tsig);
 Number(consc,tsig);
 
 // attach number meanings
 snprintf(buff,CF_BUFSIZE,"%s:q",here_and_now);
 IGr(consc,buff,a_name,"the current observed value");
 snprintf(buff,CF_BUFSIZE,"%s:E",here_and_now);
 IGr(consc,buff,a_name,"the running mean value");
 snprintf(buff,CF_BUFSIZE,"%s:sig",here_and_now);
 IGr(consc,buff,a_name,"a std deviation over recent samples");
 snprintf(buff,CF_BUFSIZE,"%s:Et",here_and_now);
 IGr(consc,buff,a_name,"a mean time to change");
 snprintf(buff,CF_BUFSIZE,"%s:tsig",here_and_now);
 IGr(consc,buff,a_name,"a mean time deviation");

}

