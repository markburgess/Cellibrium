//
// CGN_MONITORD - a proof of concept cognitive monitoring agent
//

/*
   Copyright (C) Mark Burgess

   This file is derived in part from the code written by MB for ECG/CFEngine 1999-2013.
   It has been modified extensively in concept, but some attempt has been
   made for preserve compatibility with CFEngine 3.6-.

   I have, however, abandoned the idea of generic platform support, to get appropriate
   functionality, and this is now GNU/Linux specific, as suitable for cloud, IoT, etc.

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

*/

#include <math.h>
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
#include <files_names.h>
#include <unix.h>
#include <verify_measurements.h>
#include <verify_classes.h>
#include <unix_iface.h>
#include <time_classes.h>
#include <file_lib.h>
#include <item_lib.h>
#include <graph.h>
#include <processes_select.h>

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
#define CF_INV_FILE      "invariant_classes"

/*****************************************************************************/

static char ENVFILE_NEW[CF_BUFSIZE] = "";
static char ENVFILE[CF_BUFSIZE] = "";
static char GRAPHFILE_NEW[CF_BUFSIZE] = "";
static char GRAPHFILE[CF_BUFSIZE] = "";
static char INVFILE[CF_BUFSIZE],INVFILE_NEW[CF_BUFSIZE],INVFILE_OLD[CF_BUFSIZE];

static double HISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS] = { { { 0.0 } } };
static Averages MAX, MIN;

/* persistent observations */

static double CF_THIS[CF_OBSERVABLES] = { 0.0 };

static char *MY_LOCATION = "Mark's hideout address in Oslo"; // Need some way of reading this, e.g. serial number

static Item *USERS = NULL;
static Item *ARGS = NULL;

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
int JUST_REVIVED = true;

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
static void AnnotateAnomaly(EvalContext *ctx,FILE *consc,time_t now,Item *process_syndrome,Item *performance_syndrome,Item *security_syndrome,Item *invariants);
static void AnnotateOpenPort(FILE *consc, char *type, char *number, char *address);
static Item *LoadInvariants(void);
static void SaveInvariants(Item *list);
static void PublishEnvironment(Item *classes);
static void DiffInvariants(EvalContext *ctx,Item **process_syndrome,Item **performance_syndrome,Item **security_syndrome,Item **invariants);
char *MakeAnomalyGrName(FILE *fp,char *title,Item *list);
char *MakeFlatList(FILE *fp,Item *list);
char *HereGr(FILE *fp, char *address);
void ClassifyProcessState(EvalContext *ctx, FILE *fp);
static void UpdateProcessGroup(char *value,int *process_group_0, int *process_group_1,int *process_group_2,int *process_group_user);
static void CountDefunctProcesses(FILE *fp,char *command, int *def);
static void CommandConcepts(FILE *fp,char *user, char *command);
static void UsernameConcepts(FILE *fp,char *user);
int LoadSpecialQ(char *name,double *oldq, double *oldvar);
int SaveSpecialQ(char *name,double oldq, double oldvar);
void UpdateStrQResourceImpact(EvalContext *ctx, char *name,char *value,char *user,char *args);
void UpdateRealQResourceImpact(EvalContext *ctx, char *qname,double newq);
static void SaveStateList(Item *list,char *name);
static Item *LoadStateList(char *name);
void ClassifyListChanges(EvalContext *ctx, Item *current_state, char *comment);
void ActiveUsers(FILE *fp,char *hub);
void CheckExpectedSpacetime(EvalContext *ctx);

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

 snprintf(INVFILE, CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_INV_FILE);
 snprintf(INVFILE_NEW, CF_BUFSIZE,"%s.new",INVFILE);
 snprintf(INVFILE_OLD, CF_BUFSIZE,"%s.prev",INVFILE);
 MapName(INVFILE);
 MapName(INVFILE_NEW);
 MapName(INVFILE_OLD);
 
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

 if (PROCESSTABLE)
    {
    DeleteItemList(PROCESSTABLE);
    }
 PROCESSTABLE = MonGetProcessState();  
 
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
 Item *openports = NULL;
 int count = 1;

 char now[CF_SMALLBUF];
 time_t nowt = time(NULL);

 UpdateTimeClasses(ctx, nowt);
 CheckExpectedSpacetime(ctx);

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

    //
    // Add graph semantics
    //
    
    char cname[CF_BUFSIZE];
    snprintf(cname,CF_BUFSIZE,"measurement type %s",name);
    RoleGr(consc,cname, "measurement type", name, ContextGr(consc,"system monitoring measurement"));
    RoleGr(consc,"normal state","state","background,normal","system monitoring measurement");
    RoleGr(consc,"anomalous state","state","change,anomaly","system monitoring measurement");
    Gr(consc,cname,a_interpreted,desc,"system monitoring measurement");
    ContextGr(consc,"measurement anomaly");
    ContextGr(consc,"measurement type");
    snprintf(cname,CF_BUFSIZE,"%s_high",name);
    Gr(consc,cname,a_depends,name,"system monitoring measurement");
    snprintf(cname,CF_BUFSIZE,"%s_low",name);
    Gr(consc,cname,a_depends,name,"system monitoring measurement");
    snprintf(cname,CF_BUFSIZE,"%s_high_anomaly",name);
    Gr(consc,cname,a_depends,name,"system monitoring measurement");
    snprintf(cname,CF_BUFSIZE,"%s_low_anomaly",name);
    Gr(consc,cname,a_depends,name,"system monitoring measurement");

    //
    
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

       // Note the semantic connection
       snprintf(cname,CF_BUFSIZE,"anomaly %s",buff);
       RoleGr(consc,cname, "measurement anomaly", buff, ContextGr(consc,"system monitoring measurement"));
       //
       
       AppendItem(&mon_data, buff, "2");
       EvalContextHeapPersistentSave(ctx, buff, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
       EvalContextClassPutSoft(ctx, buff, CONTEXT_SCOPE_NAMESPACE, "process state");
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
    EvalContextClassPutSoft(ctx,"listening_IPport_overflow", CONTEXT_SCOPE_NAMESPACE, "process state");
    PrependItem(&openports,buff,NULL);
    }
 else
    {
    for (ip = MON_TCP6; ip != NULL; ip=ip->next)
       {
       snprintf(buff,CF_BUFSIZE,"tcp6_port_addr[%s]=%s",ip->name,ip->classes);
       AppendItem(&mon_data, buff, NULL);
       Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
       snprintf(buff,CF_BUFSIZE,"tcp6_port_%s_listen_%s",ip->name,ip->classes);
       AnnotateOpenPort(consc,buff,ip->name,ip->classes);
       EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
       PrependItem(&openports,buff,NULL);
       }
    
    for (ip = MON_TCP4; ip != NULL; ip=ip->next)
       {
       snprintf(buff,CF_BUFSIZE,"tcp4_port_addr[%s]=%s",ip->name,ip->classes);
       AppendItem(&mon_data, buff, NULL);
       Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
       snprintf(buff,CF_BUFSIZE,"tcp4_port_%s_listen_%s",ip->name,ip->classes);
       AnnotateOpenPort(consc,buff,ip->name,ip->classes);
       EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
       PrependItem(&openports,buff,NULL);
       }
    }
 
 for (ip = MON_UDP6; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"udp6_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    snprintf(buff,CF_BUFSIZE,"udp6_port_%s_listen_%s",ip->name,ip->classes);
    AnnotateOpenPort(consc,buff,ip->name,ip->classes);
    EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
    PrependItem(&openports,buff,NULL);
    }
 
 for (ip = MON_UDP4; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"udp4_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    snprintf(buff,CF_BUFSIZE,"udp4_port_%s_listen_%s",ip->name,ip->classes);
    AnnotateOpenPort(consc,buff,ip->name,ip->classes);
    EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
    PrependItem(&openports,buff,NULL);
    }
 
 for (ip = MON_RAW6; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"raw6_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    snprintf(buff,CF_BUFSIZE,"raw6_port_%s_listen_%s",ip->name,ip->classes);
    AnnotateOpenPort(consc,buff,ip->name,ip->classes);
    EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
    PrependItem(&openports,buff,NULL);
    }
 
 for (ip = MON_RAW4; ip != NULL; ip=ip->next)
    {
    snprintf(buff,CF_BUFSIZE,"raw4_port_addr[%s]=%s",ip->name,ip->classes);
    AppendItem(&mon_data, buff, NULL);
    Log(LOG_LEVEL_VERBOSE, "  [%d] %s", count++, buff);
    snprintf(buff,CF_BUFSIZE,"raw4_port_%s_listen_%s",ip->name,ip->classes);
    AnnotateOpenPort(consc,buff,ip->name,ip->classes);
    EvalContextClassPutSoft(ctx,buff, CONTEXT_SCOPE_NAMESPACE, "process state");
    PrependItem(&openports,buff,NULL);
    }
 
 PublishEnvironment(mon_data); 
 DeleteItemList(mon_data);

 // Now build the semantic graph based on these `smart sensor' inputs

 if (consc)
    {
    // Get process table state for change detection
 
    ClassifyProcessState(ctx,consc);
    ClassifyListChanges(ctx,openports,"IP port");
    DeleteItemList(openports);

    // We can't afford to remember every moment in time, but we should remember unusual moments - anomalies
    // State what = syndrome_name, when=now, where=host_addr, how=syndrome, why=anomaly degree
    // Express: system monitoring sample at when-where-why
    
    Item *performance_syndrome = NULL;
    Item *process_syndrome = NULL;
    Item *security_syndrome = NULL;
    Item *invariants = NULL;
    
    if (JUST_REVIVED)
       {
       // why = ContextGr(consc,"monitoring restarted");
       // Event....
       char *who = "cgn_montord";
       char *what = "monitor restarted";
       char *why = "unknown"; // or "unknown"
       time_t when = time(NULL);
       char *how = "restart monitor service";
       char *howattr = "restart,service,monitor";
       char *icontext = "system monitoring measurment";
       char *where =  HereGr(consc,MY_LOCATION);

       Log(LOG_LEVEL_VERBOSE, "\n ***********\n * DETECTED A RESTART anomaly\n ***********\n");
       EventClue(consc,who,what,when,where,how,why,icontext);
       RoleGr(consc,how,"how",howattr,"system monitoring measurement");
       }

    DiffInvariants(ctx,&process_syndrome,&performance_syndrome,&security_syndrome,&invariants);
    AnnotateAnomaly(ctx,consc,nowt,process_syndrome,performance_syndrome,security_syndrome,invariants);

    DeleteItemList(performance_syndrome);
    DeleteItemList(security_syndrome);
    DeleteItemList(process_syndrome);
    DeleteItemList(invariants);
    }

 JUST_REVIVED = false;
 
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

if (dev > 3.0 * sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_anomaly");
   strcpy(degree, "3sigma");
   AppendItem(classlist, buffer2, "3");

   //EvalContextHeapPersistentSave(ctx, buffer2, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
   //EvalContextClassPutSoft(ctx, buffer2, CONTEXT_SCOPE_NAMESPACE, "");

   // Note the semantic connection
   char topic[CF_BUFSIZE];
   snprintf(topic,CF_BUFSIZE,"anomaly %s",buffer2);
   RoleGr(consc,topic, "measurement anomaly", buffer2,"system monitoring measurement");
   //

   return sigma;
   }

if (dev > 2.0 * sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_dev2");
   strcpy(degree, "2sigma");
   AppendItem(classlist, buffer2, "2");

   //EvalContextHeapPersistentSave(ctx, buffer2, CF_PERSISTENCE, CONTEXT_STATE_POLICY_PRESERVE, "");
   //EvalContextClassPutSoft(ctx, buffer2, CONTEXT_SCOPE_NAMESPACE, "");

   // Note the semantic connection
   char topic[CF_BUFSIZE];
   snprintf(topic,CF_BUFSIZE,"anomaly %s",buffer2);
   RoleGr(consc,topic, "measurement anomaly", buffer2,"system monitoring measurement");
   //

   return sigma;
   }

if (dev <= sqrt(2.0))
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_normal");
   strcpy(degree, "normal");
   AppendItem(classlist, buffer2, "0");
   return sigma;
   }
else
   {
   strcpy(buffer2, buffer);
   strcat(buffer2, "_dev1");
   strcpy(degree, "sigma");
   AppendItem(classlist, buffer2, "0");
   return sigma;
   }

// TODO - handle t_expect, t_dev, etc


}

/*****************************************************************************/

static void SetVariable(FILE *consc, char *name, double value, double average, double stddev, Item **classlist)
{
 char var[CF_BUFSIZE];
 
 snprintf(var, CF_MAXVARSIZE, "value_%s=%.2lf", name, value);
 AppendItem(classlist, var, "");
 snprintf(var, CF_MAXVARSIZE, "av_%s=%.2lf", name, average);
 AppendItem(classlist, var, "");
 snprintf(var, CF_MAXVARSIZE, "dev_%s=%.2lf", name, stddev);
 AppendItem(classlist, var, "");


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
        strcpy(buffer,"human aspects");
        break;
    case ob_rootprocs:
        strcpy(buffer,"system aspects");
        break;
    case ob_otherprocs:
        strcpy(buffer,"system and human aspects");
        break;
    case ob_diskfree:
    case ob_loadavg:
        strcpy(buffer,"system aspects");
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
        strcpy(buffer,"application service measurements");
        break;
    case ob_webaccess:
    case ob_weberrors:
    case ob_syslog:
    case ob_messages:
        strcpy(buffer,"system logs");
        break;
    case ob_temp0:
    case ob_temp1:
    case ob_temp2:
    case ob_temp3:
        strcpy(buffer,"physical environment measurements");
        break;
    case ob_cpuall:
    case ob_cpu0:
    case ob_cpu1:
    case ob_cpu2:
    case ob_cpu3:
        strcpy(buffer,"processor related measurements");
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
        strcpy(buffer,"application service services");
        break;
    case ob_ospf_in:
    case ob_ospf_out:
    case ob_bgp_in:
    case ob_bgp_out:
        strcpy(buffer,"routing measurements");
        break;
    default:                      // Empirically these seem to be the system probes Mikhail reorganized to random addresses
        strcpy(buffer,"unknown measurement type");
        break;
    }
}

/*****************************************************************************/

static void AnnotateAnomaly(EvalContext *ctx,FILE *consc,time_t now,Item *process_syndrome,Item *performance_syndrome,Item *security_syndrome,Item *invariants)
{
 // Look for an anomalous state change as a criterion for a significant event to feed into this
 // EventClue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext)
 // but use the full state to describe the conditions for an anomaly
 
 Banner("Describe state/anomaly semantics");

 // if the list contains process changes as well as performance changes, assume the process changes may cause performance

 Item *ip;
 
 char *when = TimeGr(consc,now);
 char *where = HereGr(consc,MY_LOCATION);
 char *icontext = ContextGr(consc,"cgn_montord system monitoring");
 char *how;
 char what[CF_BUFSIZE];
 char why[CF_BUFSIZE];
 char attr[CF_BUFSIZE];

 // Report
 int dimension = 0;
 int invariant = 0;
 
 for (ip = process_syndrome; ip != NULL; ip=ip->next)
    {
    Log(LOG_LEVEL_VERBOSE,"  PROCSYMPTOMS : %s\n",ip->name);
    dimension++;
    }
 for (ip = performance_syndrome; ip != NULL; ip=ip->next)
    {
    Log(LOG_LEVEL_VERBOSE,"  PERFSYMPTOMS : %s\n",ip->name);
    dimension++;
    } 
 for (ip = security_syndrome; ip != NULL; ip=ip->next)
    {
    Log(LOG_LEVEL_VERBOSE,"  SECURITY : %s\n",ip->name);
    dimension++;
    }

 for (ip = invariants; ip != NULL; ip=ip->next)
    {
    //Log(LOG_LEVEL_VERBOSE,"  persistent : %s\n",ip->name);
    invariant++;
    }
 
 if (dimension)
    {
    Log(LOG_LEVEL_VERBOSE,"\n  ANOMALY OF DIMENSION : %d, with %d persistent anomalies\n\n",dimension,invariant);
    }

 // End reporting

 if (security_syndrome)
    {
    how = MakeAnomalyGrName(consc,"security anomaly",security_syndrome);
    Gr(consc,how,a_hasrole,"how",icontext);
    
    snprintf(what,CF_BUFSIZE,"event %s at %s %s",how,where,when);
    snprintf(attr,CF_BUFSIZE,"%s,%s,%s",how,where,when); 
    
    RoleGr(consc,what,"security event",attr,icontext);
    Gr(consc,what,a_hasattr,"security","cgn_monitord system monitoring");
    Gr(consc,what,a_hasattr,"event","cgn_monitord system monitoring");
    Log(LOG_LEVEL_VERBOSE,"  SECURITY cluster : %s\n",what);
    }

 char hub[CF_BUFSIZE];
 
 if (performance_syndrome && process_syndrome)
    {
    ActiveUsers(consc,hub);
    char *who = hub;

    // Hypothesis workload changes cause performance changes - not generally true...
    strcpy(what,MakeAnomalyGrName(consc,"workload anomaly",performance_syndrome)); 
    strcpy(why,MakeAnomalyGrName(consc,"performance anomaly",process_syndrome));
    
    how = "unknown";
    
    EventClue(consc,who,what,now,where,how,why,icontext);
    Gr(consc,what,a_hasattr,"performance","cgn_monitord system monitoring");
    Gr(consc,what,a_hasattr,"workload","cgn_monitord system monitoring");
    Gr(consc,what,a_hasattr,"event","cgn_monitord system monitoring");

    Log(LOG_LEVEL_VERBOSE,"--- Hypothesis workload changes cause performance changes --------------\n");
    Log(LOG_LEVEL_VERBOSE,"  WHO  : %s\n", who);
    Log(LOG_LEVEL_VERBOSE,"  WHAT : %s\n", what);
    Log(LOG_LEVEL_VERBOSE,"  HOW : %s\n", how);
    Log(LOG_LEVEL_VERBOSE,"  WHY : %s\n", why);
    }
 
/* 
 // Acausal clusters form concepts with unknown origin
 
 char *who = "cgn_montord";
 char *what = "anomalous state change";
 char *why = "unknown"; // or "unknown"
 when = now;
 char *how = MakeAnomalyGrName(consc,"anomaly",syndrome);
 char *howattr = MakeFlatList(consc,syndrome);
 char *icontext = "system monitoring measurement";
 char *where =  HereGr(consc,MY_LOCATION);

 EventClue(consc,who,what,when,where,how,why,icontext);
 char *hub = RoleGr(consc,how,"how",howattr,"system monitoring");
 Gr(consc,"anomalous state",a_contains,how,"system monitoring measurement");
 
 for (ip = syndrome; ip != NULL; ip = ip->next)
    {
    if (ip->name)
       {
       char anomaly[CF_BUFSIZE];
       snprintf(anomaly,CF_BUFSIZE,"anomaly %s",ip->name);
       Gr(consc,hub,a_caused_by,anomaly,"measurement anomaly");
       Log(LOG_LEVEL_VERBOSE," - current state `%.25s' contains %s\n",how,ip->name);
       }
    }

 how = MakeAnomalyGrName(consc,"background",invariants);
 who = "cgn_montord";
 what = "normal state";
 why = "unknown"; // or "unknown"
 when = now;
 howattr = MakeFlatList(consc,invariants);
 icontext = "system monitoring";

 // Normal background
 EventClue(consc,who,what,when,where,how,why,icontext);
 hub = RoleGr(consc,how,"how",howattr,"system monitoring measurement");

 // Superhub
 Gr(consc,"normal state",a_contains,how,"system monitoring measurement"); */
}

/*****************************************************************************/

static void AnnotateOpenPort(FILE *consc, char *type, char *number, char *address)
{
 // Look for an anomalous state change as a criterion for a significant event to feed into this
 // EventClue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext)

 //        AnnotateOpenPort(consc,"ipv4 TCP listening port",ip->name,ip->classes);

 char what[CGN_BUFSIZE],attr[CGN_BUFSIZE],hub[CGN_BUFSIZE];
 int portnr = 0;

 sscanf(number,"%d",&portnr);

 if (portnr == 0)
    {
    Log(LOG_LEVEL_VERBOSE,"Failed to parse port number");
    return;
    }

 ActiveUsers(consc,hub);
 char *who = hub;
 char *why = ServerListenPromise(consc,"","",portnr);
 char *where = HereGr(consc,MY_LOCATION);
 char *how = type;
 char *icontext = ContextGr(consc,"system monitoring netstat");
 
 snprintf(what,CGN_BUFSIZE,"%s listening to %s",IPPort(portnr),address);
 snprintf(attr,CGN_BUFSIZE,"%s,%s",IPPort(portnr),address);
 RoleGr(consc,what,"listen on service port",attr,icontext);

 time_t when = 0; // Don't want to remember every single sample
 
 EventClue(consc,who,what,when,where,how,why,icontext);
 RoleGr(consc,how,"how",attr,"system monitoring");
}


/*********************************************************************/

void ClassifyProcessState(EvalContext *ctx, FILE *fp)
{
 char *titles = PROCESSTABLE->name;
 time_t pstime = time(NULL);
 int total_processes = 0;
 double process_group_0 = 0,process_group_1 = 0,process_group_2 = 0, process_group_user = 0;
 double defuncts = 0;
 char *column[CF_PROCCOLS] = {0};
 char *names[CF_PROCCOLS] = {0};
 int start[CF_PROCCOLS] = {0};
 int i,end[CF_PROCCOLS] = {0};
 Item *ip;
 char hub[CF_BUFSIZE];

 GetProcessColumnNames(titles, &names[0], start, end);

 for (ip = PROCESSTABLE->next; ip != NULL; ip=ip->next)
    {
    if (!SplitProcLine(ip->name, pstime, names, start, end, column))
       {
       return;
       }

    // Sanitize some specifics to extract invariant parts by removing IDs

    if (*column[11] == '[')
       {
       for (char *sp = column[11]; *sp != '\0'; sp++)
          {
          if (isdigit(*sp)) // replace and specific instance numbers with N
             {
             *sp = 'N';
             }
          }
       }

    total_processes++;

    // 0=USER, 1=PID, 2=PPID, 3=PGID, 4=%CPU, 5=%MEM, 6=VSZ, 7=NI, 8=RSS, 9=NLWP, 10=STIME, 11=COMMAND 

    UpdateProcessGroup(column[1],&process_group_0,&process_group_1,&process_group_2,&process_group_user);

    UpdateStrQResourceImpact(ctx,"%CPU",column[4],column[0],column[11]);
    UpdateStrQResourceImpact(ctx,"%MEM",column[5],column[0],column[11]);
    UpdateStrQResourceImpact(ctx,"NIce",column[7],column[0],column[11]);
    
    CommandConcepts(fp,column[0],column[11]);

    CountDefunctProcesses(fp,column[11],&defuncts);
    
    // Count processes per user
    IdempPrependItem(&USERS,column[0],NULL);
    IncrementItemListCounter(USERS,column[0]);
    
    IdempPrependItem(&ARGS,column[11],NULL);
    IncrementItemListCounter(ARGS,column[11]);

    for (i = 0; column[i] != NULL; i++)
       {
       free(column[i]);
       }
    }

 Banner("Autodetect active users");

 ActiveUsers(fp,hub);

 for (ip = USERS; ip != NULL; ip=ip->next)
    {
    UsernameConcepts(fp,ip->name);
    Gr(fp,hub,a_contains,ip->name,"host process table");
    Log(LOG_LEVEL_VERBOSE,"  active user - %20s in %s\n",SUser(ip->name),hub);
    }

 UpdateRealQResourceImpact(ctx,"defuncts",defuncts);
 UpdateRealQResourceImpact(ctx,"processgroup0count",process_group_0);
 UpdateRealQResourceImpact(ctx,"processgroup1count",process_group_1);
 UpdateRealQResourceImpact(ctx,"processgroup2count",process_group_2);
 UpdateRealQResourceImpact(ctx,"processgroupUSERcount",process_group_user);
     
 ClassifyListChanges(ctx,ARGS, "JOB command change");
 ClassifyListChanges(ctx,USERS,"USERname change");
 
 DeleteItemList(USERS);
 DeleteItemList(ARGS);
 USERS = ARGS = NULL;
 
}

/***********************************************************************************************/

static void UpdateProcessGroup(char *value,int *process_group_0, int *process_group_1,int *process_group_2,int *process_group_user)
{
 if (strcmp(value,"0") == 0)
    {
    *process_group_0++;
    }
 else if (strcmp(value,"1") == 0)
    {
    *process_group_1++;
    }
 else if (strcmp(value,"2") == 0)
    {
    *process_group_2++;
    }
 else
    {
    *process_group_user++;
    }
}

/***********************************************************************************************/

static void CountDefunctProcesses(FILE *fp,char *command, int *defunct)
{
 if (strstr(command,"<defunct>"))
    {
    *defunct++;
    }
 
 char hub[CF_BUFSIZE];
 snprintf(hub,CF_BUFSIZE,"defunct process %s",command);
 RoleGr(fp,hub,"defunct process",command,"host process table");
}

/***********************************************************************************************/

static void CommandConcepts(FILE *fp,char *user, char *command)
{
 char *sp,cmd[CF_BUFSIZE],attr[CF_BUFSIZE];
 char *where = HereGr(fp,MY_LOCATION);

 snprintf(cmd,CF_BUFSIZE,"%s process group entry %s %s",user,command,where);
 snprintf(attr,CF_BUFSIZE,"user %s,command %s,%s",user,command,where);

 RoleGr(fp,cmd,"process group entry",attr,"host process table");

 snprintf(cmd,CF_BUFSIZE,"command %s",command);
 RoleGr(fp,cmd,"executable program",command,"running process");
 
 if (*command == '/')
    {
    sscanf(command,"%s",cmd); // extract command path

    for (sp = cmd+strlen(cmd); *sp != '/'; sp--)
       {
       }

    RoleGr(fp,command,"server",sp+1,"running process");
    *sp = '\0';

    snprintf(attr,CF_BUFSIZE,"directory %s",cmd);
    RoleGr(fp,attr,"directory",cmd,"executable program");
    }
}

/***********************************************************************************************/

static void UsernameConcepts(FILE *fp,char *user)
{
 char hub[CF_BUFSIZE],attr[CF_BUFSIZE];
 char *where = HereGr(fp,MY_LOCATION);

 RoleGr(fp,SUser(user),"username",user,"host process table");
 
 snprintf(hub,CF_BUFSIZE,"active username %s %s",user,where);
 snprintf(attr,CF_BUFSIZE,"%s,%s,active",SUser(user),where);

 RoleGr(fp,hub,"active username",attr,"host process table");
}

/*********************************************************************/

void ClassifyListChanges(EvalContext *ctx, Item *current_state, char *comment)
{
 Item *prev_state = LoadStateList(comment); // Assume pre-sorted, no tampering
 char name[CF_BUFSIZE];

 // Now separate the lists into (invariant/intesect + delta/NOT-intersect) sets

 Item *ip1 = current_state, *ip2 = prev_state, *match;

 // Learn the current number of each named item, in the usual WAverage way
 
 for (ip1 = current_state; ip1 != NULL; ip1=ip1->next)
    {
    if ((match = ReturnItemIn(prev_state,ip1->name)))
       {   
       }
    else
       {
       snprintf(name,CF_BUFSIZE,"%s %.64s_appeared",comment,ip1->name);
       EvalContextClassPutSoft(ctx,name, CONTEXT_SCOPE_NAMESPACE, "process state");
       }
    
    snprintf(name,CF_BUFSIZE,"%s_%s",comment,ip1->name); 
    UpdateRealQResourceImpact(ctx,name,(double)(ip1->counter));
    DeleteItemLiteral(&prev_state,ip1->name);
    }
 
 for (ip2 = prev_state; ip2 != NULL; ip2=ip2->next)
    {
    snprintf(name,CF_BUFSIZE,"%s %.64s_disappeared",comment,ip2->name);
    EvalContextClassPutSoft(ctx,name, CONTEXT_SCOPE_NAMESPACE, "process state"); 
    snprintf(name,CF_BUFSIZE,"%s_%.64s",comment,ip2->name);
    UpdateRealQResourceImpact(ctx,name,0);
    }
 
 SaveStateList(current_state,comment);
 DeleteItemList(prev_state);
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

static void DiffInvariants(EvalContext *ctx,Item **process_syndrome,Item **performance_syndrome,Item **security_syndrome,Item **invariants)
{
 Item *current_state = NULL;
 Item *prev_state = LoadInvariants(); // Assume pre-sorted, no tampering
 
 ClassTableIterator *iter = EvalContextClassTableIteratorNewGlobal(ctx, NULL, true, true);
 Class *cls = NULL;

 while ((cls = ClassTableIteratorNext(iter)))
    {
    StringSet *tagset = EvalContextClassTags(ctx, cls->ns, cls->name);
    StringSetIterator iter2 = StringSetIteratorInit(tagset);
    char *name = NULL, fqname[CF_BUFSIZE];

    if (strstr(cls->name,"normal") || strstr(cls->name,"dev1"))
       {
       continue;
       }

    if (cls->ns)
       {
       snprintf(fqname,CF_BUFSIZE,"%s_%s",cls->ns,cls->name);
       }
    else
       {
       snprintf(fqname,CF_BUFSIZE,"%s",cls->name);
       }

    while ((name = StringSetIteratorNext(&iter2)))
       {
       if (strcmp(name,"time") == 0) // Skip non-invariant time classes
          {
          break;
          }
       PrependItem(&current_state,fqname,NULL);
       break;
       }
    }

 ClassTableIteratorDestroy(iter);
 SaveInvariants(current_state);

 // Now separate the lists into (invariant/intesect + delta/NOT-intersect) sets

 Item *ip1 = current_state, *ip2 = prev_state;
 
 for (ip1 = current_state; ip1 != NULL; ip1=ip1->next)
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
       
       if (strncmp(ip1->name,"JOB",3) == 0)
          {
          PrependItem(process_syndrome,name,NULL);
          }
       else if (((strncmp(ip1->name,"USER",4) == 0) && strstr(ip1->name,"appeared"))
                || strstr(ip1->name,"6_port") || strstr(ip1->name,"4_port")
                || strstr(ip1->name,"ipv4")|| strstr(ip1->name,"ipv6"))
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

    if (strncmp(ip2->name,"JOB",3) == 0)
       {
       PrependItem(process_syndrome,name,NULL);
       }
    else if (strncmp(ip2->name,"USER",4) == 0 || strstr(ip2->name,"6_port") || strstr(ip2->name,"4_port")
             || strstr(ip2->name,"ipv4")|| strstr(ip2->name,"ipv6"))
       {
       PrependItem(security_syndrome,name,NULL);
       }
    else
       {
       PrependItem(performance_syndrome,name,NULL);
       }
    }
 
 DeleteItemList(current_state);
 DeleteItemList(prev_state);
}

/*********************************************************************/
    
static void SaveInvariants(Item *list)
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

static Item *LoadInvariants()
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

static Item *LoadStateList(char *comment)
{
 FILE *fp;
 Item *list = NULL;
 char class[CF_BUFSIZE],name[CF_BUFSIZE];
 int counter = 0;

 snprintf(name,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,comment);
 
 if ((fp = fopen(name, "r")) == NULL)
    {
    return NULL;
    }
 
 while(!feof(fp))
    {
    class[0] = '\0';
    counter = 0;
    
    fscanf(fp,"%s %d",class,&counter);

    if (class[0] != '\0')
       {
       PrependItem(&list,class,NULL);
       SetItemListCounter(list,class,counter);
       }
    }
 
 fclose(fp);
 return list;
}

/*********************************************************************/

static void SaveStateList(Item *list,char *name)
{
 FILE *fp;
 Item *ip;
 char fname[CF_BUFSIZE];

 snprintf(fname,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,name);
 
 if ((fp = fopen(fname, "w")) == NULL)
    {
    return;
    }
 
 for(ip = list; ip != NULL; ip=ip->next)
    {
    fprintf(fp,"%s %d\n",ip->name,ip->counter);
    }
 
 fclose(fp);
}

/**********************************************************************/

char *HereGr(FILE *fp, char *address)
{
 char identity[CGN_BUFSIZE], identitymain[CGN_BUFSIZE];

 // alias different indentities
 snprintf(identitymain,CGN_BUFSIZE,"host identity %s",VFQNAME);
 snprintf(identity,CGN_BUFSIZE,"host identity %s",VIPADDRESS); 
 Gr(fp,identitymain,a_alias,identity,"host location identification");

 return WhereGr(fp,address,VUQNAME,VDOMAIN,VIPADDRESS,NULL);
}

/*********************************************************************/

char *MakeAnomalyGrName(FILE *fp,char *title,Item *list)

// take 1_2_3, e.g. wwws_in_state - four letters from first, then 1 letter from 2 and 3
    
{
 static char result[CF_BUFSIZE] = {0};
 char *digest = MakeFlatList(fp,list);
 snprintf(result, CF_BUFSIZE, "%s %s",title,digest);
 RoleGr(fp,result,title,digest,"anomaly cluster");
 return result;
}

/*********************************************************************/

char *MakeFlatList(FILE *fp,Item *list)

// create a cluster for the list under a short signature heading
    
{
 static char result[64];
 Item *ip;
 int i,len = 0;
 memset(result,0,64);

 // make signature
 for (i = 0; i < 62; i++)
    {
    for (ip = list; ip != NULL; ip=ip->next)
       {
       if (ip->name[i] == '_' || (len > 0 && ip->name[i] == result[len-1]))
          {
          continue;
          }
       
       result[len++] = ip->name[i];

       if (len >= 63)
          {
          goto members;
          }
       }
    }

 members:

 for (ip = list; ip != NULL; ip=ip->next)
    {
    Gr(fp,result,a_contains,ip->name,"system monitoring measurement");
    }

 return result;
}

/***********************************************************************************************/

void UpdateStrQResourceImpact(EvalContext *ctx, char *name,char *value,char *user,char *args)
{
 double newq = 0.1;
 char qname[CF_BUFSIZE];
 
 if (strcmp(value,"-") == 0 ||strcmp(value,"0.0") == 0)
    {
    return; // uninteresting
    }

  sscanf(value,"%lf",&newq);
 
  if (fabs(newq) > 99.0)
     {
     // parsing error?
     return;
     }
  
  // The reporting is already coarse grained by ps
  
  snprintf(qname,CF_BUFSIZE,"%s_%s_%.64s",user,name,CanonifyName(args));
  UpdateRealQResourceImpact(ctx,qname,newq);
      
}

/***********************************************************************************************/

void UpdateRealQResourceImpact(EvalContext *ctx, char *qname,double newq)
{
 double oldav = 0, oldvar = 0.1;
 char cname[CF_BUFSIZE];
 
 if (LoadSpecialQ(qname,&oldav,&oldvar))
    {
    // Because the coarse resolution is only 0.1
    
    if (oldvar == 0) // desensitize
       {
       oldvar = 0.5;
       }
    
    double nextav = WAverage(newq,oldav,WAGE);
    double newvar = (newq-oldav)*(newq-oldav);
    double nextvar = WAverage(newvar,oldvar,WAGE);
    double devq = sqrt(oldvar);

    if (devq < 0.1)
       {
       devq = 0.1; // Minimum sensitivity
       }
    
    if (newq > oldav + 3*devq)
       {
       snprintf(cname,CF_BUFSIZE,"%s_high_anomaly",qname);
       EvalContextClassPutSoft(ctx, cname, CONTEXT_SCOPE_NAMESPACE, "process state");
       Log(LOG_LEVEL_VERBOSE," [pr] Process anomaly %s (%lf > %lf)\n",cname,newq,oldav+3*devq);
       }
    else if (newq < oldav - 3*devq)
       {
       snprintf(cname,CF_BUFSIZE,"%s_low_anomaly",qname);
       EvalContextClassPutSoft(ctx, cname, CONTEXT_SCOPE_NAMESPACE, "process state");
       Log(LOG_LEVEL_VERBOSE," [pr] Process anomaly %s (%lf < %lf)\n",cname,newq,oldav+3*devq);
       }

    SaveSpecialQ(qname,nextav,nextvar);
    }
 else
    {
    double nextav = WAverage(newq,0,WAGE);
    double nextvar = WAverage(newq/2,0,WAGE);
    
    SaveSpecialQ(qname,nextav,nextvar);
    }

 // if more 30% of resource flag this specially
  
}

/*********************************************************************/

int LoadSpecialQ(char *name,double *oldq, double *oldvar)
{
 FILE *fp;
 char file[CF_BUFSIZE];

 snprintf(file,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,name);

 if ((fp = fopen(file,"r")) == NULL)
    {
    return false;
    }

 fscanf(fp,"%lf %lf",oldq,oldvar);
 fclose(fp);
 return true;
}

/*********************************************************************/

int SaveSpecialQ(char *name,double av,double var)
{
 FILE *fp;
 char file[CF_BUFSIZE];

 snprintf(file,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,name);
  
 if ((fp = fopen(file,"w")) == NULL)
    {
    return false;
    }

 fprintf(fp,"%lf %lf",av,var);
 fclose(fp);
 return true;
}

/*********************************************************************/

void ActiveUsers(FILE *fp,char *hub)
{
 snprintf(hub,CF_BUFSIZE,"active users %s",HereGr(fp,MY_LOCATION));
 RoleGr(fp,hub,"active users","users,username","host process table");
}

/*********************************************************************/

void CheckExpectedSpacetime(EvalContext *ctx)
{
 FILE *fp;
 char name[CF_BUFSIZE];
 char lastip[CF_BUFSIZE],lastname[CF_BUFSIZE],lastdomain[CF_BUFSIZE];
 int normal = true;
 
 Banner(" * Checking spacetime location");

 if (strlen(VDOMAIN) == 0)
    {
    strcpy(VDOMAIN,"unknown_domain");
    }

 snprintf(name,CF_BUFSIZE,"%s/state/location",CFWORKDIR);

 lastip[0] = lastname[0] = lastdomain[0] = '\0';

 if ((fp = fopen(name,"r")) != NULL)
    {
    fscanf(fp,"%[^, ],%[^, ],%s",lastname,lastdomain,lastip);
    fclose(fp);
    }

 if (strcmp(lastname,VUQNAME) || strcmp(lastdomain,VDOMAIN) || strcmp(lastip,VIPADDRESS))
    {
    EvalContextClassPutSoft(ctx,"teleportation_event", CONTEXT_SCOPE_NAMESPACE, "check location");

    Log(LOG_LEVEL_VERBOSE," * teleportation event - sensors moved to a new IP location - from (%s,%s,%s) to (%s,%s,%s)\n",lastname,lastdomain,lastip,VUQNAME,VDOMAIN,VIPADDRESS);
    
    if ((fp = fopen(name,"w")) != NULL)
       {
       fprintf(fp,"%s,%s,%s\n",VUQNAME,VDOMAIN,VIPADDRESS);
       fclose(fp);
       }

    normal = false;
    }

 snprintf(name,CF_BUFSIZE,"%s/state/clock",CFWORKDIR);

 time_t then = 0, now = time(NULL);

 if ((fp = fopen(name,"r")) != NULL)
    {
    fscanf(fp,"%ld",&then);
    fclose(fp);
    }

 if (now < then)
    {
    // System blacked out
    EvalContextClassPutSoft(ctx,"clock_jumped_backwards_event", CONTEXT_SCOPE_NAMESPACE, "check clock time");
    Log(LOG_LEVEL_VERBOSE," * clock jump event - possible clock reset - jump %d secs\n",(int)(now-then));
    normal = false;
    }

 if (now > then + 2*SLEEPTIME)
    {
    // System blacked out
    EvalContextClassPutSoft(ctx,"clock_jumped_forwards_event", CONTEXT_SCOPE_NAMESPACE, "check clock time");
    Log(LOG_LEVEL_VERBOSE," * clock jump event - possible system blackout - jump %d secs\n",(int)(now-then));
    normal = false;
    }

 if ((fp = fopen(name,"w")) != NULL)
    {
    fprintf(fp,"%ld",now);
    fclose(fp);
    }

 if (normal)
    {
    Log(LOG_LEVEL_VERBOSE," * no changes\n");
    }
}














